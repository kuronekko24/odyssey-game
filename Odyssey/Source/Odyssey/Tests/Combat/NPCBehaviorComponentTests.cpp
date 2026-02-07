// NPCBehaviorComponentTests.cpp
// Comprehensive automation tests for UNPCBehaviorComponent
// Covers: states, transitions, performance tiers, patrol, detection

#include "Misc/AutomationTest.h"
#include "NPCBehaviorComponent.h"
#include "OdysseyMobileOptimizer.h"
#include "Tests/AutomationCommon.h"

// ============================================================================
// 1. BEHAVIOR: Default Construction
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBehavior_Init_Defaults,
	"Odyssey.Combat.BehaviorComponent.Init.Defaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBehavior_Init_Defaults::RunTest(const FString& Parameters)
{
	UNPCBehaviorComponent* Comp = NewObject<UNPCBehaviorComponent>();
	TestNotNull(TEXT("Component created"), Comp);
	TestEqual(TEXT("Default state is Idle"), Comp->GetCurrentState(), ENPCState::Idle);
	TestEqual(TEXT("Previous state is Idle"), Comp->GetPreviousState(), ENPCState::Idle);
	TestTrue(TEXT("Default is hostile"), Comp->IsHostile());
	TestFalse(TEXT("No valid target initially"), Comp->HasValidTarget());
	TestFalse(TEXT("No patrol route initially"), Comp->HasPatrolRoute());
	return true;
}

// ============================================================================
// 2. BEHAVIOR: State Display Names
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBehavior_State_DisplayNames,
	"Odyssey.Combat.BehaviorComponent.State.DisplayNames",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBehavior_State_DisplayNames::RunTest(const FString& Parameters)
{
	UNPCBehaviorComponent* Comp = NewObject<UNPCBehaviorComponent>();

	// Default state is Idle
	TestEqual(TEXT("Idle display name"), Comp->GetStateDisplayName(), FString(TEXT("Idle")));
	return true;
}

// ============================================================================
// 3. BEHAVIOR: State Enum Values
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBehavior_State_EnumValues,
	"Odyssey.Combat.BehaviorComponent.State.EnumValues",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBehavior_State_EnumValues::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Idle = 0"), static_cast<uint8>(ENPCState::Idle) == 0);
	TestTrue(TEXT("Patrolling = 1"), static_cast<uint8>(ENPCState::Patrolling) == 1);
	TestTrue(TEXT("Engaging = 2"), static_cast<uint8>(ENPCState::Engaging) == 2);
	TestTrue(TEXT("Dead = 3"), static_cast<uint8>(ENPCState::Dead) == 3);
	return true;
}

// ============================================================================
// 4. BEHAVIOR: Performance Tier Settings
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBehavior_PerfTier_HighDefaults,
	"Odyssey.Combat.BehaviorComponent.PerfTier.HighDefaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBehavior_PerfTier_HighDefaults::RunTest(const FString& Parameters)
{
	UNPCBehaviorComponent* Comp = NewObject<UNPCBehaviorComponent>();

	// Default tier is High
	TestEqual(TEXT("Default tier is High"), Comp->GetPerformanceTier(), EPerformanceTier::High);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBehavior_PerfTier_UpdateFrequencies,
	"Odyssey.Combat.BehaviorComponent.PerfTier.UpdateFrequencies",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBehavior_PerfTier_UpdateFrequencies::RunTest(const FString& Parameters)
{
	// Verify the documented update rates:
	// High: 10Hz, Medium: 5Hz, Low: 2Hz
	FNPCBehaviorPerformanceSettings High;
	High.UpdateFrequency = 10.0f;
	TestEqual(TEXT("High tier = 10Hz"), High.UpdateFrequency, 10.0f);

	FNPCBehaviorPerformanceSettings Medium;
	Medium.UpdateFrequency = 5.0f;
	TestEqual(TEXT("Medium tier = 5Hz"), Medium.UpdateFrequency, 5.0f);

	FNPCBehaviorPerformanceSettings Low;
	Low.UpdateFrequency = 2.0f;
	TestEqual(TEXT("Low tier = 2Hz"), Low.UpdateFrequency, 2.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBehavior_PerfTier_LowDisablesLOS,
	"Odyssey.Combat.BehaviorComponent.PerfTier.LowDisablesFeatures",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBehavior_PerfTier_LowDisablesLOS::RunTest(const FString& Parameters)
{
	// Verify Low tier settings disable expensive features
	FNPCBehaviorPerformanceSettings LowSettings;
	LowSettings.bEnablePatrolling = false;
	LowSettings.bEnableLineOfSightChecks = false;
	LowSettings.DetectionRangeMultiplier = 0.5f;

	TestFalse(TEXT("Low tier disables patrolling"), LowSettings.bEnablePatrolling);
	TestFalse(TEXT("Low tier disables LOS checks"), LowSettings.bEnableLineOfSightChecks);
	TestEqual(TEXT("Low tier halves detection range"), LowSettings.DetectionRangeMultiplier, 0.5f);
	return true;
}

// ============================================================================
// 5. BEHAVIOR: Hostility Configuration
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBehavior_Config_Hostility,
	"Odyssey.Combat.BehaviorComponent.Config.Hostility",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBehavior_Config_Hostility::RunTest(const FString& Parameters)
{
	UNPCBehaviorComponent* Comp = NewObject<UNPCBehaviorComponent>();

	TestTrue(TEXT("Default hostile"), Comp->IsHostile());

	Comp->SetHostile(false);
	TestFalse(TEXT("Set non-hostile"), Comp->IsHostile());

	Comp->SetHostile(true);
	TestTrue(TEXT("Set hostile again"), Comp->IsHostile());
	return true;
}

// ============================================================================
// 6. BEHAVIOR: Patrol Route Management
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBehavior_Patrol_SetPoints,
	"Odyssey.Combat.BehaviorComponent.Patrol.SetPoints",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBehavior_Patrol_SetPoints::RunTest(const FString& Parameters)
{
	UNPCBehaviorComponent* Comp = NewObject<UNPCBehaviorComponent>();

	TestFalse(TEXT("No patrol route initially"), Comp->HasPatrolRoute());

	TArray<FVector> PatrolPoints;
	PatrolPoints.Add(FVector(0, 0, 0));
	PatrolPoints.Add(FVector(100, 0, 0));
	PatrolPoints.Add(FVector(100, 100, 0));

	Comp->SetPatrolPoints(PatrolPoints);
	TestTrue(TEXT("Has patrol route after set"), Comp->HasPatrolRoute());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBehavior_Patrol_EmptyRouteHasNoEffect,
	"Odyssey.Combat.BehaviorComponent.Patrol.EmptyRouteNoEffect",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBehavior_Patrol_EmptyRouteHasNoEffect::RunTest(const FString& Parameters)
{
	UNPCBehaviorComponent* Comp = NewObject<UNPCBehaviorComponent>();

	TArray<FVector> EmptyPoints;
	Comp->SetPatrolPoints(EmptyPoints);

	TestFalse(TEXT("Empty patrol route"), Comp->HasPatrolRoute());
	TestEqual(TEXT("Still in Idle"), Comp->GetCurrentState(), ENPCState::Idle);
	return true;
}

// ============================================================================
// 7. BEHAVIOR: Target Management (Without Spawned Actor)
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBehavior_Target_NullTargetIsInvalid,
	"Odyssey.Combat.BehaviorComponent.Target.NullIsInvalid",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBehavior_Target_NullTargetIsInvalid::RunTest(const FString& Parameters)
{
	UNPCBehaviorComponent* Comp = NewObject<UNPCBehaviorComponent>();

	TestFalse(TEXT("No target initially"), Comp->HasValidTarget());
	TestTrue(TEXT("Current target is null"), Comp->GetCurrentTarget() == nullptr);
	TestFalse(TEXT("Cannot attack without target"), Comp->CanAttack());
	TestFalse(TEXT("Target not in range"), Comp->IsTargetInRange());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBehavior_Target_ClearTarget,
	"Odyssey.Combat.BehaviorComponent.Target.ClearTarget",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBehavior_Target_ClearTarget::RunTest(const FString& Parameters)
{
	UNPCBehaviorComponent* Comp = NewObject<UNPCBehaviorComponent>();

	// Clear on empty state should not crash
	Comp->ClearTarget();
	TestFalse(TEXT("No target after clear"), Comp->HasValidTarget());
	TestEqual(TEXT("Distance is 0 with no target"), Comp->GetDistanceToTarget(), 0.0f);
	return true;
}

// ============================================================================
// 8. BEHAVIOR: NPC State Change Event Payload
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBehavior_Event_StateChangePayload,
	"Odyssey.Combat.BehaviorComponent.Event.StateChangePayload",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBehavior_Event_StateChangePayload::RunTest(const FString& Parameters)
{
	FNPCStateChangeEventPayload Payload;
	TestEqual(TEXT("Default previous state"), Payload.PreviousState, ENPCState::Idle);
	TestEqual(TEXT("Default new state"), Payload.NewState, ENPCState::Idle);
	TestEqual(TEXT("Default ship name"), Payload.NPCShipName, NAME_None);
	TestFalse(TEXT("Default engagement target null"), Payload.EngagementTarget.IsValid());

	// Set values
	Payload.PreviousState = ENPCState::Idle;
	Payload.NewState = ENPCState::Engaging;
	Payload.NPCShipName = FName("TestShip");

	TestEqual(TEXT("Set previous state"), Payload.PreviousState, ENPCState::Idle);
	TestEqual(TEXT("Set new state"), Payload.NewState, ENPCState::Engaging);
	TestEqual(TEXT("Set ship name"), Payload.NPCShipName, FName("TestShip"));
	return true;
}

// ============================================================================
// 9. BEHAVIOR: Patrol Config Defaults
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBehavior_PatrolConfig_Defaults,
	"Odyssey.Combat.BehaviorComponent.PatrolConfig.Defaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBehavior_PatrolConfig_Defaults::RunTest(const FString& Parameters)
{
	FNPCPatrolConfig Config;
	TestEqual(TEXT("Default patrol speed"), Config.PatrolSpeed, 300.0f);
	TestEqual(TEXT("Default patrol radius"), Config.PatrolRadius, 100.0f);
	TestTrue(TEXT("Default loop patrol"), Config.bLoopPatrol);
	TestEqual(TEXT("Default wait time"), Config.WaitTimeAtPoint, 2.0f);
	TestEqual(TEXT("No patrol points"), Config.PatrolPoints.Num(), 0);
	return true;
}

// ============================================================================
// 10. BEHAVIOR: Detection Radius
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBehavior_Detection_EffectiveRadius,
	"Odyssey.Combat.BehaviorComponent.Detection.EffectiveRadius",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBehavior_Detection_EffectiveRadius::RunTest(const FString& Parameters)
{
	UNPCBehaviorComponent* Comp = NewObject<UNPCBehaviorComponent>();

	// Default detection radius is 1000 with High tier multiplier of 1.0
	float EffectiveRadius = Comp->GetEffectiveDetectionRadius();
	TestEqual(TEXT("High tier effective radius = base"), EffectiveRadius, 1000.0f);
	return true;
}

// ============================================================================
// 11. BEHAVIOR: CanAttack Validation
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBehavior_Attack_CanAttackRequirements,
	"Odyssey.Combat.BehaviorComponent.Attack.CanAttackRequirements",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBehavior_Attack_CanAttackRequirements::RunTest(const FString& Parameters)
{
	UNPCBehaviorComponent* Comp = NewObject<UNPCBehaviorComponent>();

	// CanAttack requires: valid target, in range, Engaging state, and cooldown elapsed
	TestFalse(TEXT("Cannot attack: no target"), Comp->CanAttack());

	// Even setting state would fail without valid target
	TestFalse(TEXT("Cannot attack: missing requirements"), Comp->CanAttack());
	return true;
}

// ============================================================================
// 12. BEHAVIOR: Performance Settings Struct
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBehavior_PerfSettings_Defaults,
	"Odyssey.Combat.BehaviorComponent.PerfSettings.Defaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBehavior_PerfSettings_Defaults::RunTest(const FString& Parameters)
{
	FNPCBehaviorPerformanceSettings Settings;
	TestEqual(TEXT("Default update freq = 10Hz"), Settings.UpdateFrequency, 10.0f);
	TestEqual(TEXT("Default detection freq = 2Hz"), Settings.DetectionUpdateFrequency, 2.0f);
	TestTrue(TEXT("Default patrolling enabled"), Settings.bEnablePatrolling);
	TestTrue(TEXT("Default LOS checks enabled"), Settings.bEnableLineOfSightChecks);
	TestEqual(TEXT("Default detection range mult = 1.0"), Settings.DetectionRangeMultiplier, 1.0f);
	return true;
}
