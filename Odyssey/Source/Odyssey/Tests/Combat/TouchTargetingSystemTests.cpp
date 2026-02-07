// TouchTargetingSystemTests.cpp
// Comprehensive automation tests for UTouchTargetingSystem
// Covers: config, snapshots, scoring, reticle states, edge cases

#include "Misc/AutomationTest.h"
#include "Combat/TouchTargetingSystem.h"
#include "Combat/CombatTypes.h"
#include "Tests/AutomationCommon.h"

// ============================================================================
// 1. TARGETING: Configuration Defaults
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTargeting_Config_Defaults,
	"Odyssey.Combat.TouchTargeting.Config.Defaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTargeting_Config_Defaults::RunTest(const FString& Parameters)
{
	FTargetingConfig Config;
	TestEqual(TEXT("Default MaxRange"), Config.MaxRange, 3000.0f);
	TestEqual(TEXT("Default TouchRadiusPixels"), Config.TouchRadiusPixels, 60.0f);
	TestEqual(TEXT("Default AutoTargetInterval"), Config.AutoTargetInterval, 0.4f);
	TestEqual(TEXT("Default DistanceWeight"), Config.DistanceWeight, 1.0f);
	TestEqual(TEXT("Default LowHealthWeight"), Config.LowHealthWeight, 0.6f);
	TestEqual(TEXT("Default HostilityWeight"), Config.HostilityWeight, 1.5f);
	TestEqual(TEXT("Has Enemy tag"), Config.ValidTargetTags.Num(), 2);
	return true;
}

// ============================================================================
// 2. TARGETING: Component Construction
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTargeting_Init_Defaults,
	"Odyssey.Combat.TouchTargeting.Init.Defaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTargeting_Init_Defaults::RunTest(const FString& Parameters)
{
	UTouchTargetingSystem* System = NewObject<UTouchTargetingSystem>();
	TestNotNull(TEXT("System created"), System);
	TestTrue(TEXT("No target initially"), System->GetCurrentTarget() == nullptr);
	TestFalse(TEXT("HasValidTarget is false"), System->HasValidTarget());
	TestEqual(TEXT("Reticle starts Hidden"), System->GetReticleState(), EReticleState::Hidden);
	return true;
}

// ============================================================================
// 3. TARGETING: Target Snapshot Structure
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTargeting_Snapshot_Defaults,
	"Odyssey.Combat.TouchTargeting.Snapshot.Defaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTargeting_Snapshot_Defaults::RunTest(const FString& Parameters)
{
	FCombatTargetSnapshot Snap;
	TestFalse(TEXT("Default snapshot is invalid"), Snap.IsValid());
	TestTrue(TEXT("Default actor is null"), Snap.GetActor() == nullptr);
	TestEqual(TEXT("Default location is zero"), Snap.WorldLocation, FVector::ZeroVector);
	TestEqual(TEXT("Default velocity is zero"), Snap.Velocity, FVector::ZeroVector);
	TestEqual(TEXT("Default health is 1.0"), Snap.HealthFraction, 1.0f);
	TestFalse(TEXT("Default not hostile"), Snap.bIsHostile);
	TestFalse(TEXT("Default no LOS"), Snap.bHasLineOfSight);
	TestEqual(TEXT("Default priority is 0"), Snap.PriorityScore, 0.0f);
	return true;
}

// ============================================================================
// 4. TARGETING: Distance Query Without Target
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTargeting_Distance_NoTarget,
	"Odyssey.Combat.TouchTargeting.Distance.NoTarget",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTargeting_Distance_NoTarget::RunTest(const FString& Parameters)
{
	UTouchTargetingSystem* System = NewObject<UTouchTargetingSystem>();
	float Dist = System->GetDistanceToTarget();
	TestEqual(TEXT("Distance is MAX_FLT without target"), Dist, TNumericLimits<float>::Max());
	return true;
}

// ============================================================================
// 5. TARGETING: ClearTarget Safety
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTargeting_Clear_WhenEmpty,
	"Odyssey.Combat.TouchTargeting.Clear.WhenEmpty",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTargeting_Clear_WhenEmpty::RunTest(const FString& Parameters)
{
	UTouchTargetingSystem* System = NewObject<UTouchTargetingSystem>();

	// Should not crash when clearing an already-empty target
	System->ClearTarget();
	TestFalse(TEXT("Still no target after clear"), System->HasValidTarget());
	return true;
}

// ============================================================================
// 6. TARGETING: SelectTarget with null
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTargeting_Select_NullActor,
	"Odyssey.Combat.TouchTargeting.Select.NullActor",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTargeting_Select_NullActor::RunTest(const FString& Parameters)
{
	UTouchTargetingSystem* System = NewObject<UTouchTargetingSystem>();

	bool bResult = System->SelectTarget(nullptr, false);
	TestFalse(TEXT("Cannot select null target"), bResult);
	TestFalse(TEXT("No target set"), System->HasValidTarget());
	return true;
}

// ============================================================================
// 7. TARGETING: Target Priority Scoring Config
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTargeting_Scoring_WeightsConfigurable,
	"Odyssey.Combat.TouchTargeting.Scoring.WeightsConfigurable",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTargeting_Scoring_WeightsConfigurable::RunTest(const FString& Parameters)
{
	UTouchTargetingSystem* System = NewObject<UTouchTargetingSystem>();

	// Modify scoring weights
	System->Config.DistanceWeight = 2.0f;
	System->Config.LowHealthWeight = 1.5f;
	System->Config.HostilityWeight = 3.0f;

	TestEqual(TEXT("Distance weight updated"), System->Config.DistanceWeight, 2.0f);
	TestEqual(TEXT("LowHealth weight updated"), System->Config.LowHealthWeight, 1.5f);
	TestEqual(TEXT("Hostility weight updated"), System->Config.HostilityWeight, 3.0f);
	return true;
}

// ============================================================================
// 8. TARGETING: Valid Target Tags
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTargeting_Tags_DefaultTags,
	"Odyssey.Combat.TouchTargeting.Tags.DefaultTags",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTargeting_Tags_DefaultTags::RunTest(const FString& Parameters)
{
	FTargetingConfig Config;
	TestTrue(TEXT("Has Enemy tag"), Config.ValidTargetTags.Contains(FName("Enemy")));
	TestTrue(TEXT("Has NPC tag"), Config.ValidTargetTags.Contains(FName("NPC")));
	return true;
}

// ============================================================================
// 9. TARGETING: Reticle State Enum
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTargeting_Reticle_AllStates,
	"Odyssey.Combat.TouchTargeting.Reticle.AllStates",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTargeting_Reticle_AllStates::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Hidden = 0"), static_cast<uint8>(EReticleState::Hidden) == 0);
	TestTrue(TEXT("Searching = 1"), static_cast<uint8>(EReticleState::Searching) == 1);
	TestTrue(TEXT("Locking = 2"), static_cast<uint8>(EReticleState::Locking) == 2);
	TestTrue(TEXT("Locked = 3"), static_cast<uint8>(EReticleState::Locked) == 3);
	TestTrue(TEXT("Firing = 4"), static_cast<uint8>(EReticleState::Firing) == 4);
	TestTrue(TEXT("OutOfRange = 5"), static_cast<uint8>(EReticleState::OutOfRange) == 5);
	return true;
}

// ============================================================================
// 10. TARGETING: Max Range Configuration
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTargeting_Config_MaxRange,
	"Odyssey.Combat.TouchTargeting.Config.MaxRange",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTargeting_Config_MaxRange::RunTest(const FString& Parameters)
{
	UTouchTargetingSystem* System = NewObject<UTouchTargetingSystem>();

	System->Config.MaxRange = 5000.0f;
	TestEqual(TEXT("Max range updated"), System->Config.MaxRange, 5000.0f);

	System->Config.MaxRange = 500.0f;
	TestEqual(TEXT("Max range can be reduced"), System->Config.MaxRange, 500.0f);
	return true;
}

// ============================================================================
// 11. TARGETING: Lead Target Prediction Config
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTargeting_Snapshot_VelocityForLead,
	"Odyssey.Combat.TouchTargeting.Snapshot.VelocityForLead",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTargeting_Snapshot_VelocityForLead::RunTest(const FString& Parameters)
{
	FCombatTargetSnapshot Snap;
	Snap.Velocity = FVector(500.0f, 0.0f, 0.0f);
	Snap.WorldLocation = FVector(1000.0f, 0.0f, 0.0f);

	TestEqual(TEXT("Velocity stored"), Snap.Velocity, FVector(500.0f, 0.0f, 0.0f));
	TestEqual(TEXT("Location stored"), Snap.WorldLocation, FVector(1000.0f, 0.0f, 0.0f));
	return true;
}

// ============================================================================
// 12. TARGETING: GetCurrentTargetSnapshot when empty
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTargeting_Snapshot_EmptySnapshot,
	"Odyssey.Combat.TouchTargeting.Snapshot.EmptySnapshot",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTargeting_Snapshot_EmptySnapshot::RunTest(const FString& Parameters)
{
	UTouchTargetingSystem* System = NewObject<UTouchTargetingSystem>();
	FCombatTargetSnapshot Snap = System->GetCurrentTargetSnapshot();
	TestFalse(TEXT("Empty snapshot is invalid"), Snap.IsValid());
	return true;
}
