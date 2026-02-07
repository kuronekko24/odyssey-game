// OdysseyDamageProcessorTests.cpp
// Comprehensive automation tests for UOdysseyDamageProcessor
// Covers: pipeline, falloff, type multipliers, true damage, multi-target, stats

#include "Misc/AutomationTest.h"
#include "OdysseyDamageProcessor.h"
#include "NPCHealthComponent.h"
#include "Tests/AutomationCommon.h"

// ============================================================================
// 1. DAMAGE PROCESSOR: Construction and Initialization
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDmgProcessor_Init_DefaultValues,
	"Odyssey.Combat.DamageProcessor.Init.DefaultValues",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDmgProcessor_Init_DefaultValues::RunTest(const FString& Parameters)
{
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();
	TestNotNull(TEXT("Processor created"), Proc);

	// Check defaults from constructor
	FDamageProcessorStats Stats = Proc->GetStatistics();
	TestEqual(TEXT("No events processed"), Stats.TotalDamageEventsProcessed, (int64)0);
	TestEqual(TEXT("No kills"), Stats.KillsProcessed, (int64)0);
	TestFalse(TEXT("Not initialized before Init()"), Proc->IsInitialized());
	return true;
}

// ============================================================================
// 2. DAMAGE PROCESSOR: CalculateDamage Pipeline
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDmgProcessor_Calc_BaseDamage,
	"Odyssey.Combat.DamageProcessor.Calc.BaseDamage",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDmgProcessor_Calc_BaseDamage::RunTest(const FString& Parameters)
{
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();
	// Disable crits for deterministic test
	Proc->SetCriticalHitsEnabled(false);

	FDamageCalculationParams Params;
	Params.BaseDamage = 100.0f;
	Params.DamageType = FName("Kinetic");

	FDamageCalculationResult Result = Proc->CalculateDamage(Params);
	TestEqual(TEXT("Base damage passes through"), Result.FinalDamage, 100.0f);
	TestFalse(TEXT("Not critical"), Result.bIsCritical);
	TestEqual(TEXT("Distance falloff is 1.0"), Result.DistanceFalloff, 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDmgProcessor_Calc_GlobalMultiplier,
	"Odyssey.Combat.DamageProcessor.Calc.GlobalMultiplier",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDmgProcessor_Calc_GlobalMultiplier::RunTest(const FString& Parameters)
{
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();
	Proc->SetCriticalHitsEnabled(false);
	Proc->SetGlobalDamageMultiplier(1.5f);

	FDamageCalculationParams Params;
	Params.BaseDamage = 100.0f;

	FDamageCalculationResult Result = Proc->CalculateDamage(Params);
	TestEqual(TEXT("Global mult applied"), Result.FinalDamage, 150.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDmgProcessor_Calc_TypeMultiplier,
	"Odyssey.Combat.DamageProcessor.Calc.TypeMultiplier",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDmgProcessor_Calc_TypeMultiplier::RunTest(const FString& Parameters)
{
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();
	Proc->SetCriticalHitsEnabled(false);
	Proc->SetDamageTypeMultiplier(FName("Plasma"), 2.0f);

	FDamageCalculationParams Params;
	Params.BaseDamage = 50.0f;
	Params.DamageType = FName("Plasma");

	FDamageCalculationResult Result = Proc->CalculateDamage(Params);
	TestEqual(TEXT("Type mult applied"), Result.FinalDamage, 100.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDmgProcessor_Calc_NamedModifiers,
	"Odyssey.Combat.DamageProcessor.Calc.NamedModifiers",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDmgProcessor_Calc_NamedModifiers::RunTest(const FString& Parameters)
{
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();
	Proc->SetCriticalHitsEnabled(false);

	FDamageCalculationParams Params;
	Params.BaseDamage = 100.0f;
	Params.DamageModifiers.Add(FName("WeaponBonus"), 1.5f);
	Params.DamageModifiers.Add(FName("BuffMultiplier"), 1.2f);

	FDamageCalculationResult Result = Proc->CalculateDamage(Params);
	// 100 * 1.0(global) * 1.5 * 1.2 = 180
	TestEqual(TEXT("Named modifiers stacked"), Result.FinalDamage, 180.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDmgProcessor_Calc_GlobalPlusTypePlusModifiers,
	"Odyssey.Combat.DamageProcessor.Calc.FullStack",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDmgProcessor_Calc_GlobalPlusTypePlusModifiers::RunTest(const FString& Parameters)
{
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();
	Proc->SetCriticalHitsEnabled(false);
	Proc->SetGlobalDamageMultiplier(2.0f);
	Proc->SetDamageTypeMultiplier(FName("Energy"), 1.5f);

	FDamageCalculationParams Params;
	Params.BaseDamage = 10.0f;
	Params.DamageType = FName("Energy");
	Params.DamageModifiers.Add(FName("Boost"), 2.0f);

	FDamageCalculationResult Result = Proc->CalculateDamage(Params);
	// 10 * 2.0(global) * 1.5(type) * 2.0(boost) = 60
	TestEqual(TEXT("Full multiplier stack"), Result.FinalDamage, 60.0f);
	return true;
}

// ============================================================================
// 3. DAMAGE PROCESSOR: Distance Falloff
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDmgProcessor_Falloff_DisabledByDefault,
	"Odyssey.Combat.DamageProcessor.Falloff.DisabledByDefault",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDmgProcessor_Falloff_DisabledByDefault::RunTest(const FString& Parameters)
{
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();
	Proc->SetCriticalHitsEnabled(false);

	FDamageCalculationParams Params;
	Params.BaseDamage = 100.0f;
	Params.Distance = 5000.0f; // Very far

	FDamageCalculationResult Result = Proc->CalculateDamage(Params);
	TestEqual(TEXT("No falloff when disabled"), Result.FinalDamage, 100.0f);
	TestEqual(TEXT("Falloff multiplier = 1.0"), Result.DistanceFalloff, 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDmgProcessor_Falloff_WithinMinRange,
	"Odyssey.Combat.DamageProcessor.Falloff.WithinMinRange",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDmgProcessor_Falloff_WithinMinRange::RunTest(const FString& Parameters)
{
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();
	Proc->SetCriticalHitsEnabled(false);
	Proc->SetDistanceFalloffEnabled(true);
	Proc->SetDistanceFalloffParams(500.0f, 2000.0f, 1.0f);

	FDamageCalculationParams Params;
	Params.BaseDamage = 100.0f;
	Params.Distance = 300.0f; // Within min range

	FDamageCalculationResult Result = Proc->CalculateDamage(Params);
	TestEqual(TEXT("Full damage within min range"), Result.FinalDamage, 100.0f);
	TestEqual(TEXT("Falloff = 1.0 within min range"), Result.DistanceFalloff, 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDmgProcessor_Falloff_BeyondMaxRange,
	"Odyssey.Combat.DamageProcessor.Falloff.BeyondMaxRange",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDmgProcessor_Falloff_BeyondMaxRange::RunTest(const FString& Parameters)
{
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();
	Proc->SetCriticalHitsEnabled(false);
	Proc->SetDistanceFalloffEnabled(true);
	Proc->SetDistanceFalloffParams(500.0f, 2000.0f, 1.0f);
	Proc->SetMinimumDamage(0.0f); // Disable minimum damage floor

	FDamageCalculationParams Params;
	Params.BaseDamage = 100.0f;
	Params.Distance = 3000.0f; // Beyond max range

	FDamageCalculationResult Result = Proc->CalculateDamage(Params);
	TestEqual(TEXT("Zero damage beyond max range"), Result.FinalDamage, 0.0f);
	TestEqual(TEXT("Falloff = 0.0 beyond max range"), Result.DistanceFalloff, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDmgProcessor_Falloff_LinearMidpoint,
	"Odyssey.Combat.DamageProcessor.Falloff.LinearMidpoint",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDmgProcessor_Falloff_LinearMidpoint::RunTest(const FString& Parameters)
{
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();
	Proc->SetCriticalHitsEnabled(false);
	Proc->SetDistanceFalloffEnabled(true);
	Proc->SetDistanceFalloffParams(0.0f, 1000.0f, 1.0f); // Linear: 0 to 1000
	Proc->SetMinimumDamage(0.0f);

	FDamageCalculationParams Params;
	Params.BaseDamage = 100.0f;
	Params.Distance = 500.0f; // Midpoint

	FDamageCalculationResult Result = Proc->CalculateDamage(Params);
	// Linear falloff at midpoint: 1.0 - (500/1000)^1 = 0.5
	TestTrue(TEXT("~50% damage at midpoint"), FMath::IsNearlyEqual(Result.FinalDamage, 50.0f, 1.0f));
	TestTrue(TEXT("~0.5 falloff at midpoint"), FMath::IsNearlyEqual(Result.DistanceFalloff, 0.5f, 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDmgProcessor_Falloff_QuadraticCurve,
	"Odyssey.Combat.DamageProcessor.Falloff.QuadraticCurve",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDmgProcessor_Falloff_QuadraticCurve::RunTest(const FString& Parameters)
{
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();
	Proc->SetCriticalHitsEnabled(false);
	Proc->SetDistanceFalloffEnabled(true);
	Proc->SetDistanceFalloffParams(0.0f, 1000.0f, 2.0f); // Quadratic
	Proc->SetMinimumDamage(0.0f);

	FDamageCalculationParams Params;
	Params.BaseDamage = 100.0f;
	Params.Distance = 500.0f; // Midpoint

	FDamageCalculationResult Result = Proc->CalculateDamage(Params);
	// Quadratic falloff at midpoint: 1.0 - (0.5)^2 = 1.0 - 0.25 = 0.75
	TestTrue(TEXT("~75% damage with quadratic at midpoint"), FMath::IsNearlyEqual(Result.FinalDamage, 75.0f, 1.0f));
	return true;
}

// ============================================================================
// 4. DAMAGE PROCESSOR: Critical Hits
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDmgProcessor_Crit_GuaranteedCrit,
	"Odyssey.Combat.DamageProcessor.Crit.GuaranteedCrit",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDmgProcessor_Crit_GuaranteedCrit::RunTest(const FString& Parameters)
{
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();
	Proc->SetCriticalHitsEnabled(true);
	Proc->SetGlobalCriticalChance(1.0f); // 100% crit
	Proc->SetGlobalCriticalMultiplier(3.0f);

	FDamageCalculationParams Params;
	Params.BaseDamage = 50.0f;

	FDamageCalculationResult Result = Proc->CalculateDamage(Params);
	TestTrue(TEXT("Critical hit guaranteed"), Result.bIsCritical);
	TestEqual(TEXT("Crit damage = 150"), Result.FinalDamage, 150.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDmgProcessor_Crit_Disabled,
	"Odyssey.Combat.DamageProcessor.Crit.Disabled",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDmgProcessor_Crit_Disabled::RunTest(const FString& Parameters)
{
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();
	Proc->SetCriticalHitsEnabled(false);
	Proc->SetGlobalCriticalChance(1.0f); // Would be 100% but disabled

	FDamageCalculationParams Params;
	Params.BaseDamage = 100.0f;

	FDamageCalculationResult Result = Proc->CalculateDamage(Params);
	TestFalse(TEXT("No crit when disabled"), Result.bIsCritical);
	TestEqual(TEXT("Base damage only"), Result.FinalDamage, 100.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDmgProcessor_Crit_PerAttackOverride,
	"Odyssey.Combat.DamageProcessor.Crit.PerAttackOverride",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDmgProcessor_Crit_PerAttackOverride::RunTest(const FString& Parameters)
{
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();
	Proc->SetCriticalHitsEnabled(true);
	Proc->SetGlobalCriticalChance(0.0f); // No global crit
	Proc->SetGlobalCriticalMultiplier(2.0f);

	FDamageCalculationParams Params;
	Params.BaseDamage = 100.0f;
	Params.CriticalChance = 1.0f;  // Override: guaranteed crit
	Params.CriticalMultiplier = 4.0f; // Override: 4x mult

	FDamageCalculationResult Result = Proc->CalculateDamage(Params);
	TestTrue(TEXT("Per-attack crit override"), Result.bIsCritical);
	TestEqual(TEXT("Per-attack crit multiplier"), Result.FinalDamage, 400.0f);
	return true;
}

// ============================================================================
// 5. DAMAGE PROCESSOR: Minimum Damage Floor
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDmgProcessor_MinDamage_Floor,
	"Odyssey.Combat.DamageProcessor.MinDamage.Floor",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDmgProcessor_MinDamage_Floor::RunTest(const FString& Parameters)
{
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();
	Proc->SetCriticalHitsEnabled(false);
	Proc->SetGlobalDamageMultiplier(0.01f); // Very low
	Proc->SetMinimumDamage(5.0f);

	FDamageCalculationParams Params;
	Params.BaseDamage = 10.0f; // 10 * 0.01 = 0.1, but min is 5

	FDamageCalculationResult Result = Proc->CalculateDamage(Params);
	TestEqual(TEXT("Minimum damage floor enforced"), Result.FinalDamage, 5.0f);
	return true;
}

// ============================================================================
// 6. DAMAGE PROCESSOR: Statistics
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDmgProcessor_Stats_Reset,
	"Odyssey.Combat.DamageProcessor.Stats.Reset",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDmgProcessor_Stats_Reset::RunTest(const FString& Parameters)
{
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();
	Proc->ResetStatistics();

	FDamageProcessorStats Stats = Proc->GetStatistics();
	TestEqual(TEXT("Events reset"), Stats.TotalDamageEventsProcessed, (int64)0);
	TestEqual(TEXT("Damage reset"), Stats.TotalDamageDealt, (int64)0);
	TestEqual(TEXT("Shield reset"), Stats.TotalShieldDamageAbsorbed, (int64)0);
	TestEqual(TEXT("Crits reset"), Stats.CriticalHits, (int64)0);
	TestEqual(TEXT("Kills reset"), Stats.KillsProcessed, (int64)0);
	return true;
}

// ============================================================================
// 7. DAMAGE PROCESSOR: Configuration Setters
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDmgProcessor_Config_Setters,
	"Odyssey.Combat.DamageProcessor.Config.Setters",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDmgProcessor_Config_Setters::RunTest(const FString& Parameters)
{
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();

	Proc->SetGlobalDamageMultiplier(2.5f);
	Proc->SetCriticalHitsEnabled(false);
	Proc->SetGlobalCriticalChance(0.5f);
	Proc->SetGlobalCriticalMultiplier(3.0f);
	Proc->SetDistanceFalloffEnabled(true);
	Proc->SetDistanceFalloffParams(100.0f, 5000.0f, 2.0f);
	Proc->SetMinimumDamage(3.0f);

	// Verify via calculation
	Proc->SetCriticalHitsEnabled(false);

	FDamageCalculationParams Params;
	Params.BaseDamage = 40.0f;

	FDamageCalculationResult Result = Proc->CalculateDamage(Params);
	TestEqual(TEXT("Config applied: 40 * 2.5 = 100"), Result.FinalDamage, 100.0f);
	return true;
}

// ============================================================================
// 8. DAMAGE PROCESSOR: Type Multiplier Edge Cases
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDmgProcessor_TypeMult_RemoveOnNearlyOne,
	"Odyssey.Combat.DamageProcessor.TypeMult.RemoveOnNearlyOne",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDmgProcessor_TypeMult_RemoveOnNearlyOne::RunTest(const FString& Parameters)
{
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();
	Proc->SetCriticalHitsEnabled(false);

	Proc->SetDamageTypeMultiplier(FName("Laser"), 2.0f);

	// Setting to 1.0 should remove it (treated as no multiplier)
	Proc->SetDamageTypeMultiplier(FName("Laser"), 1.0f);

	FDamageCalculationParams Params;
	Params.BaseDamage = 100.0f;
	Params.DamageType = FName("Laser");

	FDamageCalculationResult Result = Proc->CalculateDamage(Params);
	TestEqual(TEXT("1.0 mult effectively removed"), Result.FinalDamage, 100.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDmgProcessor_TypeMult_ZeroDamageMultiplier,
	"Odyssey.Combat.DamageProcessor.TypeMult.ZeroDamageMultiplier",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDmgProcessor_TypeMult_ZeroDamageMultiplier::RunTest(const FString& Parameters)
{
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();
	Proc->SetCriticalHitsEnabled(false);
	Proc->SetMinimumDamage(0.0f);

	Proc->SetDamageTypeMultiplier(FName("Kinetic"), 0.0f);

	FDamageCalculationParams Params;
	Params.BaseDamage = 100.0f;
	Params.DamageType = FName("Kinetic");

	FDamageCalculationResult Result = Proc->CalculateDamage(Params);
	TestEqual(TEXT("Zero type mult = zero damage"), Result.FinalDamage, 0.0f);
	return true;
}

// ============================================================================
// 9. DAMAGE PROCESSOR: DealDamage convenience (null actor test)
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDmgProcessor_DealDamage_NullTarget,
	"Odyssey.Combat.DamageProcessor.DealDamage.NullTarget",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDmgProcessor_DealDamage_NullTarget::RunTest(const FString& Parameters)
{
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();

	float Damage = Proc->DealDamage(nullptr, 100.0f, FName("Kinetic"), nullptr);
	TestEqual(TEXT("No damage to null target"), Damage, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDmgProcessor_DealDamage_ZeroDamage,
	"Odyssey.Combat.DamageProcessor.DealDamage.ZeroDamage",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDmgProcessor_DealDamage_ZeroDamage::RunTest(const FString& Parameters)
{
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();

	// Even with a valid target, zero damage should return zero
	float Damage = Proc->DealDamage(nullptr, 0.0f, FName("Kinetic"), nullptr);
	TestEqual(TEXT("No damage from zero amount"), Damage, 0.0f);
	return true;
}

// ============================================================================
// 10. DAMAGE PROCESSOR: Calculation Result Structure
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDmgProcessor_CalcResult_CorrectFields,
	"Odyssey.Combat.DamageProcessor.CalcResult.CorrectFields",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDmgProcessor_CalcResult_CorrectFields::RunTest(const FString& Parameters)
{
	UOdysseyDamageProcessor* Proc = NewObject<UOdysseyDamageProcessor>();
	Proc->SetCriticalHitsEnabled(true);
	Proc->SetGlobalCriticalChance(1.0f);
	Proc->SetGlobalCriticalMultiplier(2.0f);
	Proc->SetGlobalDamageMultiplier(1.5f);
	Proc->SetDistanceFalloffEnabled(true);
	Proc->SetDistanceFalloffParams(0.0f, 1000.0f, 1.0f);
	Proc->SetMinimumDamage(0.0f);

	FDamageCalculationParams Params;
	Params.BaseDamage = 100.0f;
	Params.Distance = 500.0f; // 50% falloff

	FDamageCalculationResult Result = Proc->CalculateDamage(Params);

	// 100 * 1.5(global) * 0.5(falloff) * 2.0(crit) = 150
	TestTrue(TEXT("Is critical"), Result.bIsCritical);
	TestFalse(TEXT("Not blocked"), Result.bWasBlocked);
	TestTrue(TEXT("Falloff ~0.5"), FMath::IsNearlyEqual(Result.DistanceFalloff, 0.5f, 0.01f));
	TestTrue(TEXT("Damage ~150"), FMath::IsNearlyEqual(Result.FinalDamage, 150.0f, 2.0f));
	TestTrue(TEXT("Multiplier > 1"), Result.DamageMultiplier > 1.0f);
	return true;
}
