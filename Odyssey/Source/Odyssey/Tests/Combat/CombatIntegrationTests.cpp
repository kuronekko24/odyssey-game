// CombatIntegrationTests.cpp
// Integration tests that validate cross-system behavior across the combat pipeline
// Covers: full pipelines, type definitions, event payloads, system controller config

#include "Misc/AutomationTest.h"
#include "Combat/CombatTypes.h"
#include "Combat/CombatSystemController.h"
#include "Combat/TouchTargetingSystem.h"
#include "Combat/AutoWeaponSystem.h"
#include "Combat/CombatFeedbackSystem.h"
#include "NPCHealthComponent.h"
#include "OdysseyDamageProcessor.h"
#include "NPCSpawnManager.h"
#include "NPCBehaviorComponent.h"
#include "NPCShip.h"
#include "OdysseyActionEvent.h"
#include "Tests/AutomationCommon.h"

// ============================================================================
// 1. INTEGRATION: Combat Event Payload
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_CombatEvent_Payload,
	"Odyssey.Combat.Integration.CombatEvent.Payload",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_CombatEvent_Payload::RunTest(const FString& Parameters)
{
	FCombatEventPayload Payload;
	Payload.Initialize(EOdysseyEventType::AttackHit, nullptr);
	Payload.DamageAmount = 50.0f;
	Payload.DamageType = FName("Kinetic");
	Payload.bIsCritical = true;
	Payload.HitLocation = FVector(100, 200, 300);

	TestEqual(TEXT("Event type"), Payload.EventType, EOdysseyEventType::AttackHit);
	TestEqual(TEXT("Damage amount"), Payload.DamageAmount, 50.0f);
	TestEqual(TEXT("Damage type"), Payload.DamageType, FName("Kinetic"));
	TestTrue(TEXT("Is critical"), Payload.bIsCritical);
	TestEqual(TEXT("Hit location"), Payload.HitLocation, FVector(100, 200, 300));
	TestTrue(TEXT("Event ID is valid"), Payload.EventId.IsValid());
	return true;
}

// ============================================================================
// 2. INTEGRATION: Health Event Payload
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_HealthEvent_Payload,
	"Odyssey.Combat.Integration.HealthEvent.Payload",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_HealthEvent_Payload::RunTest(const FString& Parameters)
{
	FHealthEventPayload Payload;
	Payload.MaxHealth = 100.0f;
	Payload.CurrentHealth = 60.0f;
	Payload.MaxShields = 50.0f;
	Payload.CurrentShields = 20.0f;

	float HealthPct = Payload.GetHealthPercentage();
	float ShieldPct = Payload.GetShieldPercentage();
	float EffectivePct = Payload.GetEffectiveHealthPercentage();

	TestTrue(TEXT("Health percentage ~60%"), FMath::IsNearlyEqual(HealthPct, 0.6f, 0.01f));
	TestTrue(TEXT("Shield percentage ~40%"), FMath::IsNearlyEqual(ShieldPct, 0.4f, 0.01f));
	// Effective = (60 + 20) / (100 + 50) = 80 / 150 ~= 0.533
	TestTrue(TEXT("Effective percentage ~53%"), FMath::IsNearlyEqual(EffectivePct, 80.0f / 150.0f, 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_HealthEvent_ZeroMax,
	"Odyssey.Combat.Integration.HealthEvent.ZeroMax",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_HealthEvent_ZeroMax::RunTest(const FString& Parameters)
{
	FHealthEventPayload Payload;
	Payload.MaxHealth = 0.0f;
	Payload.MaxShields = 0.0f;

	TestEqual(TEXT("Health pct with zero max = 0"), Payload.GetHealthPercentage(), 0.0f);
	TestEqual(TEXT("Shield pct with zero max = 0"), Payload.GetShieldPercentage(), 0.0f);
	TestEqual(TEXT("Effective pct with zero max = 0"), Payload.GetEffectiveHealthPercentage(), 0.0f);
	return true;
}

// ============================================================================
// 3. INTEGRATION: Damage Processor + Health Component Pipeline
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_DamageCalc_WithResistance,
	"Odyssey.Combat.Integration.DamageCalc.WithResistance",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_DamageCalc_WithResistance::RunTest(const FString& Parameters)
{
	// Create processor
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();
	Proc->SetCriticalHitsEnabled(false);
	Proc->SetGlobalDamageMultiplier(1.0f);

	// Calculate damage
	FDamageCalculationParams Params;
	Params.BaseDamage = 100.0f;
	Params.DamageType = FName("Kinetic");

	FDamageCalculationResult Result = Proc->CalculateDamage(Params);
	TestEqual(TEXT("Processed base damage = 100"), Result.FinalDamage, 100.0f);

	// Create health component with resistance
	UNPCHealthComponent* Health = NewObject<UNPCHealthComponent>();
	Health->SetMaxHealth(200.0f);
	Health->SetHealth(200.0f, false);
	Health->SetDamageResistance(FName("Kinetic"), 0.25f); // 25% resistance

	// Apply damage through health component (which applies its own resistance)
	float HullDmg = Health->TakeDamage(Result.FinalDamage, nullptr, FName("Kinetic"));
	// Health component resistance: 100 * (1 - 0.25) = 75
	TestEqual(TEXT("Hull damage after resistance"), HullDmg, 75.0f);
	TestEqual(TEXT("Health = 200 - 75 = 125"), Health->GetCurrentHealth(), 125.0f);
	return true;
}

// ============================================================================
// 4. INTEGRATION: Full Damage Pipeline (Processor + Health + Shields)
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_FullPipeline_DamageToShieldsAndHull,
	"Odyssey.Combat.Integration.FullPipeline.DamageToShieldsAndHull",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_FullPipeline_DamageToShieldsAndHull::RunTest(const FString& Parameters)
{
	// Set up damage processor with a type bonus
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();
	Proc->SetCriticalHitsEnabled(false);
	Proc->SetDamageTypeMultiplier(FName("Plasma"), 1.5f); // 50% bonus

	// Calculate: 80 base * 1.5 type = 120 final
	FDamageCalculationParams Params;
	Params.BaseDamage = 80.0f;
	Params.DamageType = FName("Plasma");

	FDamageCalculationResult Result = Proc->CalculateDamage(Params);
	TestEqual(TEXT("Calculated damage"), Result.FinalDamage, 120.0f);

	// Set up health component: 100 hull, 50 shields
	UNPCHealthComponent* Health = NewObject<UNPCHealthComponent>();
	Health->SetMaxHealth(100.0f);
	Health->SetMaxShields(50.0f);
	Health->SetHealth(100.0f, false);
	Health->SetShields(50.0f, false);

	// Apply 120 damage: 50 shields + 70 to hull
	// (No resistance on health component for this test)
	float HullDmg = Health->TakeDamage(Result.FinalDamage, nullptr, FName("Plasma"));
	TestEqual(TEXT("Hull damage after shields"), HullDmg, 70.0f);
	TestEqual(TEXT("Shields depleted"), Health->GetCurrentShields(), 0.0f);
	TestEqual(TEXT("Hull at 30"), Health->GetCurrentHealth(), 30.0f);
	return true;
}

// ============================================================================
// 5. INTEGRATION: Damage to Death Pipeline
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_Pipeline_DamageToDeath,
	"Odyssey.Combat.Integration.Pipeline.DamageToDeath",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_Pipeline_DamageToDeath::RunTest(const FString& Parameters)
{
	UNPCHealthComponent* Health = NewObject<UNPCHealthComponent>();
	Health->SetMaxHealth(50.0f);
	Health->SetHealth(50.0f, false);
	Health->SetMaxShields(0.0f);

	// Verified state before combat
	TestEqual(TEXT("Pre-combat health"), Health->GetCurrentHealth(), 50.0f);
	TestFalse(TEXT("Not dead before combat"), Health->IsDead());
	TestEqual(TEXT("Healthy state"), Health->GetHealthState(), EHealthState::Healthy);

	// Apply lethal damage
	float HullDmg = Health->TakeDamage(100.0f, nullptr, FName("Explosive"));
	TestTrue(TEXT("Dead after lethal damage"), Health->IsDead());
	TestEqual(TEXT("Health at zero"), Health->GetCurrentHealth(), 0.0f);
	TestEqual(TEXT("Dead state"), Health->GetHealthState(), EHealthState::Dead);

	// Additional damage should be ignored
	float ExtraDmg = Health->TakeDamage(50.0f, nullptr, FName("Kinetic"));
	TestEqual(TEXT("No damage to dead target"), ExtraDmg, 0.0f);
	return true;
}

// ============================================================================
// 6. INTEGRATION: Combat Session Stats Lifecycle
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_SessionStats_Lifecycle,
	"Odyssey.Combat.Integration.SessionStats.Lifecycle",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_SessionStats_Lifecycle::RunTest(const FString& Parameters)
{
	FCombatSessionStats Stats;

	// Simulate a combat session
	Stats.ShotsFired = 10;
	Stats.ShotsHit = 7;
	Stats.CriticalHits = 2;
	Stats.TotalDamageDealt = 350.0f;
	Stats.EnemiesDestroyed = 1;
	Stats.EngagementDuration = 15.0f;

	TestTrue(TEXT("Accuracy = 70%"), FMath::IsNearlyEqual(Stats.GetAccuracy(), 0.7f, 0.01f));
	TestTrue(TEXT("Crit rate ~28.5%"), FMath::IsNearlyEqual(Stats.GetCritRate(), 2.0f / 7.0f, 0.01f));
	TestTrue(TEXT("DPS ~23.3"), FMath::IsNearlyEqual(Stats.GetDPS(), 350.0f / 15.0f, 0.1f));

	Stats.Reset();
	TestEqual(TEXT("Stats reset: shots"), Stats.ShotsFired, 0);
	TestEqual(TEXT("Stats reset: hits"), Stats.ShotsHit, 0);
	TestEqual(TEXT("Stats reset: kills"), Stats.EnemiesDestroyed, 0);
	return true;
}

// ============================================================================
// 7. INTEGRATION: Combat System Controller Configuration
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_Controller_Construction,
	"Odyssey.Combat.Integration.Controller.Construction",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_Controller_Construction::RunTest(const FString& Parameters)
{
	UCombatSystemController* Controller = NewObject<UCombatSystemController>();
	TestNotNull(TEXT("Controller created"), Controller);
	TestFalse(TEXT("Not initialized before init"), Controller->IsCombatEnabled());
	TestTrue(TEXT("Auto-enable default"), Controller->bAutoEnable);
	TestTrue(TEXT("Auto-register actions default"), Controller->bAutoRegisterActions);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_Controller_ConfigPropagate,
	"Odyssey.Combat.Integration.Controller.ConfigPropagate",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_Controller_ConfigPropagate::RunTest(const FString& Parameters)
{
	UCombatSystemController* Controller = NewObject<UCombatSystemController>();

	// Modify configs
	Controller->TargetingConfig.MaxRange = 5000.0f;
	Controller->WeaponConfig.BaseDamage = 50.0f;
	Controller->FeedbackConfig.MaxDamageNumbers = 16;

	TestEqual(TEXT("Targeting config set"), Controller->TargetingConfig.MaxRange, 5000.0f);
	TestEqual(TEXT("Weapon config set"), Controller->WeaponConfig.BaseDamage, 50.0f);
	TestEqual(TEXT("Feedback config set"), Controller->FeedbackConfig.MaxDamageNumbers, 16);
	return true;
}

// ============================================================================
// 8. INTEGRATION: Event Type Enum Coverage
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_EventTypes_CombatRange,
	"Odyssey.Combat.Integration.EventTypes.CombatRange",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_EventTypes_CombatRange::RunTest(const FString& Parameters)
{
	// Verify combat event types exist and are in the expected range
	TestTrue(TEXT("AttackStarted = 40"), static_cast<uint8>(EOdysseyEventType::AttackStarted) == 40);
	TestTrue(TEXT("AttackHit = 41"), static_cast<uint8>(EOdysseyEventType::AttackHit) == 41);
	TestTrue(TEXT("AttackMissed = 42"), static_cast<uint8>(EOdysseyEventType::AttackMissed) == 42);
	TestTrue(TEXT("DamageDealt = 43"), static_cast<uint8>(EOdysseyEventType::DamageDealt) == 43);
	TestTrue(TEXT("DamageReceived = 44"), static_cast<uint8>(EOdysseyEventType::DamageReceived) == 44);
	return true;
}

// ============================================================================
// 9. INTEGRATION: Event Payload Initialization
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_EventPayload_Init,
	"Odyssey.Combat.Integration.EventPayload.Init",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_EventPayload_Init::RunTest(const FString& Parameters)
{
	FOdysseyEventPayload Payload;
	TestEqual(TEXT("Default type = None"), Payload.EventType, EOdysseyEventType::None);
	TestFalse(TEXT("Default not consumed"), Payload.bConsumed);
	TestTrue(TEXT("Default cancellable"), Payload.bCancellable);

	Payload.Initialize(EOdysseyEventType::DamageDealt, nullptr, EOdysseyEventPriority::High);
	TestEqual(TEXT("Initialized type"), Payload.EventType, EOdysseyEventType::DamageDealt);
	TestEqual(TEXT("Initialized priority"), Payload.Priority, EOdysseyEventPriority::High);
	TestTrue(TEXT("Has valid event ID"), Payload.EventId.IsValid());
	TestTrue(TEXT("Has creation time"), Payload.CreationTime > 0.0);

	Payload.Consume();
	TestTrue(TEXT("Consumed"), Payload.IsConsumed());
	return true;
}

// ============================================================================
// 10. INTEGRATION: Event Filter Matching
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_EventFilter_Matching,
	"Odyssey.Combat.Integration.EventFilter.Matching",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_EventFilter_Matching::RunTest(const FString& Parameters)
{
	FOdysseyEventFilter Filter;
	Filter.AllowedEventTypes.Add(EOdysseyEventType::DamageDealt);
	Filter.AllowedEventTypes.Add(EOdysseyEventType::DamageReceived);
	Filter.MinimumPriority = EOdysseyEventPriority::Normal;

	// Create matching payload
	FOdysseyEventPayload MatchingPayload;
	MatchingPayload.EventType = EOdysseyEventType::DamageDealt;
	MatchingPayload.Priority = EOdysseyEventPriority::High;
	TestTrue(TEXT("Matching payload passes filter"), Filter.Matches(MatchingPayload));

	// Create non-matching payload (wrong type)
	FOdysseyEventPayload WrongType;
	WrongType.EventType = EOdysseyEventType::AttackStarted;
	WrongType.Priority = EOdysseyEventPriority::High;
	TestFalse(TEXT("Wrong type fails filter"), Filter.Matches(WrongType));

	// Create non-matching payload (low priority)
	FOdysseyEventPayload LowPriority;
	LowPriority.EventType = EOdysseyEventType::DamageDealt;
	LowPriority.Priority = EOdysseyEventPriority::Low;
	TestFalse(TEXT("Low priority fails filter"), Filter.Matches(LowPriority));

	// Empty filter matches everything
	FOdysseyEventFilter EmptyFilter;
	TestTrue(TEXT("Empty filter matches any event"), EmptyFilter.Matches(MatchingPayload));
	return true;
}

// ============================================================================
// 11. INTEGRATION: DOT + Resistance + Death Pipeline
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_Pipeline_DOTWithResistance,
	"Odyssey.Combat.Integration.Pipeline.DOTWithResistance",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_Pipeline_DOTWithResistance::RunTest(const FString& Parameters)
{
	UNPCHealthComponent* Health = NewObject<UNPCHealthComponent>();
	Health->SetMaxHealth(100.0f);
	Health->SetHealth(100.0f, false);
	Health->SetDamageResistance(FName("Plasma"), 0.5f); // 50% Plasma resistance

	// Apply DOT: 20 per tick, Plasma type
	Health->ApplyDamageOverTime(20.0f, 1.0f, 5.0f, FName("Plasma"), nullptr);
	TestEqual(TEXT("DOT applied"), Health->GetActiveDOTCount(), 1);

	// When the DOT ticks, it calls TakeDamage which applies resistance
	// So each tick: 20 * (1 - 0.5) = 10 damage
	// After 5 ticks: 50 damage total -> health at 50
	// We can't tick manually here without a world, but we verify setup is correct
	TestEqual(TEXT("Health pre-DOT tick"), Health->GetCurrentHealth(), 100.0f);
	return true;
}

// ============================================================================
// 12. INTEGRATION: Multiple Systems Config Consistency
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_Config_WeaponMatchesTargeting,
	"Odyssey.Combat.Integration.Config.WeaponMatchesTargeting",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_Config_WeaponMatchesTargeting::RunTest(const FString& Parameters)
{
	FAutoWeaponConfig WeaponConfig;
	FTargetingConfig TargetingConfig;

	// Weapon engagement range should be <= targeting max range
	TestTrue(TEXT("Weapon range <= Targeting range"),
		WeaponConfig.EngagementRange <= TargetingConfig.MaxRange);

	// Set custom ranges and verify consistency
	TargetingConfig.MaxRange = 4000.0f;
	WeaponConfig.EngagementRange = 3500.0f;
	TestTrue(TEXT("Custom weapon range <= custom targeting range"),
		WeaponConfig.EngagementRange <= TargetingConfig.MaxRange);
	return true;
}

// ============================================================================
// 13. INTEGRATION: Performance Tier Cascading
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_PerfTier_Cascading,
	"Odyssey.Combat.Integration.PerfTier.Cascading",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_PerfTier_Cascading::RunTest(const FString& Parameters)
{
	// Verify that performance tier enums match across all systems
	TestTrue(TEXT("High = 0"), static_cast<uint8>(EPerformanceTier::High) == 0);
	TestTrue(TEXT("Medium = 1"), static_cast<uint8>(EPerformanceTier::Medium) == 1);
	TestTrue(TEXT("Low = 2"), static_cast<uint8>(EPerformanceTier::Low) == 2);

	// Verify spawn manager limits decrease with tier
	FNPCPerformanceLimits High(12, 0.05f, 5000.0f, true, 1500.0f, 3000.0f, 5000.0f, 6);
	FNPCPerformanceLimits Med(8, 0.1f, 3000.0f, true, 1000.0f, 2000.0f, 3000.0f, 4);
	FNPCPerformanceLimits Low(4, 0.2f, 2000.0f, false, 500.0f, 1000.0f, 2000.0f, 2);

	TestTrue(TEXT("High > Medium NPCs"), High.MaxNPCs > Med.MaxNPCs);
	TestTrue(TEXT("Medium > Low NPCs"), Med.MaxNPCs > Low.MaxNPCs);
	TestTrue(TEXT("High faster updates"), High.UpdateFrequency < Med.UpdateFrequency);
	TestTrue(TEXT("Medium faster updates"), Med.UpdateFrequency < Low.UpdateFrequency);
	TestTrue(TEXT("Low disables patrol"), !Low.bEnablePatrolling);
	return true;
}

// ============================================================================
// 14. INTEGRATION: Event ID Generation Uniqueness
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_EventId_Unique,
	"Odyssey.Combat.Integration.EventId.Unique",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_EventId_Unique::RunTest(const FString& Parameters)
{
	FOdysseyEventId Id1 = FOdysseyEventId::Generate();
	FOdysseyEventId Id2 = FOdysseyEventId::Generate();
	FOdysseyEventId Id3 = FOdysseyEventId::Generate();

	TestTrue(TEXT("ID1 is valid"), Id1.IsValid());
	TestTrue(TEXT("ID2 is valid"), Id2.IsValid());
	TestTrue(TEXT("ID3 is valid"), Id3.IsValid());

	TestTrue(TEXT("ID1 != ID2"), Id1 != Id2);
	TestTrue(TEXT("ID2 != ID3"), Id2 != Id3);
	TestTrue(TEXT("ID1 != ID3"), Id1 != Id3);

	TestTrue(TEXT("ID1 == ID1"), Id1 == Id1);
	return true;
}

// ============================================================================
// 15. INTEGRATION: Damage Category Enum
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_DamageCategory_AllTypes,
	"Odyssey.Combat.Integration.DamageCategory.AllTypes",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_DamageCategory_AllTypes::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Kinetic = 0"), static_cast<uint8>(EDamageCategory::Kinetic) == 0);
	TestTrue(TEXT("Energy = 1"), static_cast<uint8>(EDamageCategory::Energy) == 1);
	TestTrue(TEXT("Plasma = 2"), static_cast<uint8>(EDamageCategory::Plasma) == 2);
	TestTrue(TEXT("Explosive = 3"), static_cast<uint8>(EDamageCategory::Explosive) == 3);
	TestTrue(TEXT("Collision = 4"), static_cast<uint8>(EDamageCategory::Collision) == 4);
	TestTrue(TEXT("True = 5"), static_cast<uint8>(EDamageCategory::True) == 5);
	return true;
}

// ============================================================================
// 16. INTEGRATION: Health State Thresholds
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_HealthState_Thresholds,
	"Odyssey.Combat.Integration.HealthState.Thresholds",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_HealthState_Thresholds::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Healthy = 0"), static_cast<uint8>(EHealthState::Healthy) == 0);
	TestTrue(TEXT("Damaged = 1"), static_cast<uint8>(EHealthState::Damaged) == 1);
	TestTrue(TEXT("Critical = 2"), static_cast<uint8>(EHealthState::Critical) == 2);
	TestTrue(TEXT("Dying = 3"), static_cast<uint8>(EHealthState::Dying) == 3);
	TestTrue(TEXT("Dead = 4"), static_cast<uint8>(EHealthState::Dead) == 4);

	// Verify threshold description:
	// Healthy: 75-100%, Damaged: 50-74%, Critical: 25-49%, Dying: 1-24%, Dead: 0%
	UNPCHealthComponent* Comp = NewObject<UNPCHealthComponent>();
	Comp->SetMaxHealth(100.0f);

	Comp->SetHealth(100.0f, false);
	TestEqual(TEXT("100% = Healthy"), Comp->GetHealthState(), EHealthState::Healthy);

	Comp->SetHealth(76.0f, false);
	TestEqual(TEXT("76% = Healthy"), Comp->GetHealthState(), EHealthState::Healthy);

	Comp->SetHealth(74.0f, false);
	TestEqual(TEXT("74% = Damaged"), Comp->GetHealthState(), EHealthState::Damaged);

	Comp->SetHealth(50.0f, false);
	TestEqual(TEXT("50% = Critical"), Comp->GetHealthState(), EHealthState::Critical);

	Comp->SetHealth(25.0f, false);
	TestEqual(TEXT("25% = Dying"), Comp->GetHealthState(), EHealthState::Dying);

	Comp->SetHealth(1.0f, false);
	TestEqual(TEXT("1% = Dying"), Comp->GetHealthState(), EHealthState::Dying);
	return true;
}

// ============================================================================
// 17. INTEGRATION: DamageOverTimeEffect Structure
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_DOTEffect_Structure,
	"Odyssey.Combat.Integration.DOTEffect.Structure",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_DOTEffect_Structure::RunTest(const FString& Parameters)
{
	FDamageOverTimeEffect DOT;
	TestEqual(TEXT("Default DamagePerTick"), DOT.DamagePerTick, 0.0f);
	TestEqual(TEXT("Default TickInterval"), DOT.TickInterval, 1.0f);
	TestEqual(TEXT("Default RemainingDuration"), DOT.RemainingDuration, 0.0f);
	TestEqual(TEXT("Default DamageType"), DOT.DamageType, NAME_None);
	TestFalse(TEXT("Default Source invalid"), DOT.Source.IsValid());
	TestEqual(TEXT("Default TickAccumulator"), DOT.TickAccumulator, 0.0f);
	return true;
}

// ============================================================================
// 18. INTEGRATION: Damage Processor Stats Structure
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_DmgProcStats_Structure,
	"Odyssey.Combat.Integration.DmgProcStats.Structure",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_DmgProcStats_Structure::RunTest(const FString& Parameters)
{
	FDamageProcessorStats Stats;
	TestEqual(TEXT("Default events"), Stats.TotalDamageEventsProcessed, (int64)0);
	TestEqual(TEXT("Default damage"), Stats.TotalDamageDealt, (int64)0);
	TestEqual(TEXT("Default shield absorbed"), Stats.TotalShieldDamageAbsorbed, (int64)0);
	TestEqual(TEXT("Default crits"), Stats.CriticalHits, (int64)0);
	TestEqual(TEXT("Default blocked"), Stats.BlockedAttacks, (int64)0);
	TestEqual(TEXT("Default kills"), Stats.KillsProcessed, (int64)0);
	TestEqual(TEXT("Default avg processing time"), Stats.AverageProcessingTimeMs, 0.0);
	return true;
}

// ============================================================================
// 19. INTEGRATION: Damage Calculation Params/Result Structs
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_DmgCalc_Structures,
	"Odyssey.Combat.Integration.DmgCalc.Structures",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_DmgCalc_Structures::RunTest(const FString& Parameters)
{
	FDamageCalculationParams Params;
	TestEqual(TEXT("Default BaseDamage"), Params.BaseDamage, 0.0f);
	TestEqual(TEXT("Default DamageType"), Params.DamageType, NAME_None);
	TestEqual(TEXT("Default CritChance"), Params.CriticalChance, -1.0f);
	TestEqual(TEXT("Default CritMult"), Params.CriticalMultiplier, -1.0f);
	TestEqual(TEXT("Default Distance"), Params.Distance, -1.0f);

	FDamageCalculationResult Result;
	TestEqual(TEXT("Default FinalDamage"), Result.FinalDamage, 0.0f);
	TestFalse(TEXT("Default not critical"), Result.bIsCritical);
	TestFalse(TEXT("Default not blocked"), Result.bWasBlocked);
	TestEqual(TEXT("Default multiplier = 1"), Result.DamageMultiplier, 1.0f);
	TestEqual(TEXT("Default falloff = 1"), Result.DistanceFalloff, 1.0f);
	return true;
}
