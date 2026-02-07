// CombatFeedbackSystemTests.cpp
// Comprehensive automation tests for UCombatFeedbackSystem
// Covers: config, damage numbers, health bars, hit markers, widget pooling

#include "Misc/AutomationTest.h"
#include "Combat/CombatFeedbackSystem.h"
#include "Combat/CombatTypes.h"
#include "Tests/AutomationCommon.h"

// ============================================================================
// 1. FEEDBACK: Configuration Defaults
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFeedback_Config_Defaults,
	"Odyssey.Combat.Feedback.Config.Defaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFeedback_Config_Defaults::RunTest(const FString& Parameters)
{
	FCombatFeedbackConfig Config;
	TestEqual(TEXT("Default EffectQuality"), Config.EffectQuality, EEffectQuality::Medium);
	TestEqual(TEXT("Default ReticleSize"), Config.ReticleSize, 72.0f);
	TestEqual(TEXT("Default DamageNumberLifetime"), Config.DamageNumberLifetime, 1.2f);
	TestEqual(TEXT("Default MaxDamageNumbers"), Config.MaxDamageNumbers, 8);
	TestTrue(TEXT("Default show enemy health bars"), Config.bShowEnemyHealthBars);
	TestTrue(TEXT("Default show hit markers"), Config.bShowHitMarkers);
	TestEqual(TEXT("Default HitMarkerDuration"), Config.HitMarkerDuration, 0.25f);
	return true;
}

// ============================================================================
// 2. FEEDBACK: Component Construction
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFeedback_Init_Defaults,
	"Odyssey.Combat.Feedback.Init.Defaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFeedback_Init_Defaults::RunTest(const FString& Parameters)
{
	UCombatFeedbackSystem* Feedback = NewObject<UCombatFeedbackSystem>();
	TestNotNull(TEXT("Feedback system created"), Feedback);

	FReticleDisplayData ReticleData = Feedback->GetReticleData();
	TestEqual(TEXT("Reticle starts hidden"), ReticleData.State, EReticleState::Hidden);

	TestEqual(TEXT("No damage numbers"), Feedback->GetActiveDamageNumbers().Num(), 0);
	TestEqual(TEXT("No health bars"), Feedback->GetTrackedHealthBars().Num(), 0);
	TestEqual(TEXT("No hit markers"), Feedback->GetActiveHitMarkers().Num(), 0);
	return true;
}

// ============================================================================
// 3. FEEDBACK: Floating Damage Number Structure
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFeedback_DmgNumber_Defaults,
	"Odyssey.Combat.Feedback.DmgNumber.Defaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFeedback_DmgNumber_Defaults::RunTest(const FString& Parameters)
{
	FFloatingDamageNumber DN;
	TestEqual(TEXT("Default WorldOrigin"), DN.WorldOrigin, FVector::ZeroVector);
	TestEqual(TEXT("Default DamageAmount"), DN.DamageAmount, 0.0f);
	TestFalse(TEXT("Default not critical"), DN.bIsCritical);
	TestEqual(TEXT("Default Age"), DN.Age, 0.0f);
	TestEqual(TEXT("Default Lifetime"), DN.Lifetime, 1.2f);
	TestEqual(TEXT("Default NormalizedAge"), DN.NormalizedAge, 0.0f);
	TestFalse(TEXT("Not expired at birth"), DN.IsExpired());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFeedback_DmgNumber_Expiry,
	"Odyssey.Combat.Feedback.DmgNumber.Expiry",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFeedback_DmgNumber_Expiry::RunTest(const FString& Parameters)
{
	FFloatingDamageNumber DN;
	DN.Lifetime = 1.0f;

	DN.Age = 0.5f;
	TestFalse(TEXT("Not expired at half life"), DN.IsExpired());

	DN.Age = 1.0f;
	TestTrue(TEXT("Expired at full lifetime"), DN.IsExpired());

	DN.Age = 2.0f;
	TestTrue(TEXT("Expired past lifetime"), DN.IsExpired());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFeedback_DmgNumber_CriticalVisual,
	"Odyssey.Combat.Feedback.DmgNumber.CriticalVisual",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFeedback_DmgNumber_CriticalVisual::RunTest(const FString& Parameters)
{
	FFloatingDamageNumber NormalDN;
	NormalDN.DamageAmount = 50.0f;
	NormalDN.bIsCritical = false;

	FFloatingDamageNumber CritDN;
	CritDN.DamageAmount = 100.0f;
	CritDN.bIsCritical = true;

	TestFalse(TEXT("Normal is not critical"), NormalDN.bIsCritical);
	TestTrue(TEXT("Crit is critical"), CritDN.bIsCritical);
	TestTrue(TEXT("Crit deals more damage"), CritDN.DamageAmount > NormalDN.DamageAmount);
	return true;
}

// ============================================================================
// 4. FEEDBACK: Tracked Health Bar Structure
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFeedback_HealthBar_Defaults,
	"Odyssey.Combat.Feedback.HealthBar.Defaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFeedback_HealthBar_Defaults::RunTest(const FString& Parameters)
{
	FTrackedHealthBar HB;
	TestFalse(TEXT("Default actor invalid"), HB.IsValid());
	TestEqual(TEXT("Default HealthFraction"), HB.HealthFraction, 1.0f);
	TestFalse(TEXT("Default not targeted"), HB.bIsTargeted);
	TestFalse(TEXT("Default not on screen"), HB.bOnScreen);
	return true;
}

// ============================================================================
// 5. FEEDBACK: Reticle Display Data Structure
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFeedback_Reticle_Defaults,
	"Odyssey.Combat.Feedback.Reticle.Defaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFeedback_Reticle_Defaults::RunTest(const FString& Parameters)
{
	FReticleDisplayData Reticle;
	TestEqual(TEXT("Default state hidden"), Reticle.State, EReticleState::Hidden);
	TestEqual(TEXT("Default size"), Reticle.Size, 72.0f);
	TestEqual(TEXT("Default pulse phase"), Reticle.PulsePhase, 0.0f);
	TestEqual(TEXT("Default distance"), Reticle.DistanceToTarget, 0.0f);
	TestEqual(TEXT("Default health fraction"), Reticle.TargetHealthFraction, 1.0f);
	TestFalse(TEXT("Default not on screen"), Reticle.bOnScreen);
	return true;
}

// ============================================================================
// 6. FEEDBACK: Hit Marker Structure
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFeedback_HitMarker_Defaults,
	"Odyssey.Combat.Feedback.HitMarker.Defaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFeedback_HitMarker_Defaults::RunTest(const FString& Parameters)
{
	FHitMarkerData HM;
	TestFalse(TEXT("Default not critical"), HM.bIsCritical);
	TestEqual(TEXT("Default Age"), HM.Age, 0.0f);
	TestEqual(TEXT("Default Lifetime"), HM.Lifetime, 0.25f);
	TestFalse(TEXT("Not expired at birth"), HM.IsExpired());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFeedback_HitMarker_Expiry,
	"Odyssey.Combat.Feedback.HitMarker.Expiry",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFeedback_HitMarker_Expiry::RunTest(const FString& Parameters)
{
	FHitMarkerData HM;
	HM.Lifetime = 0.25f;

	HM.Age = 0.1f;
	TestFalse(TEXT("Not expired at 0.1"), HM.IsExpired());

	HM.Age = 0.25f;
	TestTrue(TEXT("Expired at lifetime"), HM.IsExpired());

	HM.Age = 0.5f;
	TestTrue(TEXT("Expired past lifetime"), HM.IsExpired());
	return true;
}

// ============================================================================
// 7. FEEDBACK: Effect Quality Enum
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFeedback_EffectQuality_Enum,
	"Odyssey.Combat.Feedback.EffectQuality.Enum",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFeedback_EffectQuality_Enum::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Minimal = 0"), static_cast<uint8>(EEffectQuality::Minimal) == 0);
	TestTrue(TEXT("Low = 1"), static_cast<uint8>(EEffectQuality::Low) == 1);
	TestTrue(TEXT("Medium = 2"), static_cast<uint8>(EEffectQuality::Medium) == 2);
	TestTrue(TEXT("High = 3"), static_cast<uint8>(EEffectQuality::High) == 3);
	return true;
}

// ============================================================================
// 8. FEEDBACK: Color Configuration
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFeedback_Colors_Defaults,
	"Odyssey.Combat.Feedback.Colors.Defaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFeedback_Colors_Defaults::RunTest(const FString& Parameters)
{
	FCombatFeedbackConfig Config;

	// Normal damage = white
	TestEqual(TEXT("Normal damage color is white"), Config.NormalDamageColor, FLinearColor::White);

	// Crit damage = red-ish
	TestTrue(TEXT("Crit color red component high"), Config.CritDamageColor.R > 0.5f);

	// Reticle locked = red
	TestEqual(TEXT("Reticle locked is red"), Config.ReticleLockedColor, FLinearColor::Red);

	// Reticle out of range = grey
	TestTrue(TEXT("OOR reticle is greyish"), Config.ReticleOutOfRangeColor.A < 1.0f);
	return true;
}

// ============================================================================
// 9. FEEDBACK: Max Damage Numbers Pool
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFeedback_Pool_MaxDamageNumbers,
	"Odyssey.Combat.Feedback.Pool.MaxDamageNumbers",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFeedback_Pool_MaxDamageNumbers::RunTest(const FString& Parameters)
{
	FCombatFeedbackConfig Config;
	TestEqual(TEXT("Default max damage numbers = 8"), Config.MaxDamageNumbers, 8);

	// Ensure it can be configured
	Config.MaxDamageNumbers = 16;
	TestEqual(TEXT("Updated max = 16"), Config.MaxDamageNumbers, 16);

	Config.MaxDamageNumbers = 4;
	TestEqual(TEXT("Reduced max = 4"), Config.MaxDamageNumbers, 4);
	return true;
}

// ============================================================================
// 10. FEEDBACK: SetTargetingSystem null safety
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFeedback_SetSystems_NullSafe,
	"Odyssey.Combat.Feedback.SetSystems.NullSafe",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFeedback_SetSystems_NullSafe::RunTest(const FString& Parameters)
{
	UCombatFeedbackSystem* Feedback = NewObject<UCombatFeedbackSystem>();

	// Setting null should not crash
	Feedback->SetTargetingSystem(nullptr);
	Feedback->SetWeaponSystem(nullptr);

	FReticleDisplayData ReticleData = Feedback->GetReticleData();
	TestEqual(TEXT("Reticle hidden with null targeting"), ReticleData.State, EReticleState::Hidden);
	return true;
}
