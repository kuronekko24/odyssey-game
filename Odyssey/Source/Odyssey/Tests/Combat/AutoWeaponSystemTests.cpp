// AutoWeaponSystemTests.cpp
// Comprehensive automation tests for UAutoWeaponSystem
// Covers: config, weapon types, energy, fire rate, cooldown, engagement states

#include "Misc/AutomationTest.h"
#include "Combat/AutoWeaponSystem.h"
#include "Combat/CombatTypes.h"
#include "Tests/AutomationCommon.h"

// ============================================================================
// 1. WEAPON: Default Configuration
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeapon_Config_Defaults,
	"Odyssey.Combat.AutoWeapon.Config.Defaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeapon_Config_Defaults::RunTest(const FString& Parameters)
{
	FAutoWeaponConfig Config;
	TestEqual(TEXT("Default BaseDamage"), Config.BaseDamage, 20.0f);
	TestEqual(TEXT("Default FireRate"), Config.FireRate, 3.0f);
	TestEqual(TEXT("Default EngagementRange"), Config.EngagementRange, 2000.0f);
	TestEqual(TEXT("Default Accuracy"), Config.Accuracy, 0.92f);
	TestEqual(TEXT("Default CritChance"), Config.CritChance, 0.08f);
	TestEqual(TEXT("Default CritMultiplier"), Config.CritMultiplier, 2.0f);
	TestEqual(TEXT("Default EnergyCost"), Config.EnergyCost, 5);
	TestEqual(TEXT("Default ProjectileSpeed"), Config.ProjectileSpeed, 0.0f);
	return true;
}

// ============================================================================
// 2. WEAPON: Component Construction
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeapon_Init_Defaults,
	"Odyssey.Combat.AutoWeapon.Init.Defaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeapon_Init_Defaults::RunTest(const FString& Parameters)
{
	UAutoWeaponSystem* Weapon = NewObject<UAutoWeaponSystem>();
	TestNotNull(TEXT("Weapon created"), Weapon);
	TestEqual(TEXT("Default engagement state is Idle"), Weapon->GetEngagementState(), ECombatEngagementState::Idle);
	TestTrue(TEXT("Auto-fire enabled by default"), Weapon->IsAutoFireEnabled());
	TestTrue(TEXT("Weapon enabled by default"), Weapon->IsWeaponEnabled());
	return true;
}

// ============================================================================
// 3. WEAPON: Enable/Disable
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeapon_Enable_Toggle,
	"Odyssey.Combat.AutoWeapon.Enable.Toggle",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeapon_Enable_Toggle::RunTest(const FString& Parameters)
{
	UAutoWeaponSystem* Weapon = NewObject<UAutoWeaponSystem>();

	Weapon->SetWeaponEnabled(false);
	TestFalse(TEXT("Weapon disabled"), Weapon->IsWeaponEnabled());

	Weapon->SetWeaponEnabled(true);
	TestTrue(TEXT("Weapon re-enabled"), Weapon->IsWeaponEnabled());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeapon_AutoFire_Toggle,
	"Odyssey.Combat.AutoWeapon.AutoFire.Toggle",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeapon_AutoFire_Toggle::RunTest(const FString& Parameters)
{
	UAutoWeaponSystem* Weapon = NewObject<UAutoWeaponSystem>();

	Weapon->SetAutoFireEnabled(false);
	TestFalse(TEXT("Auto-fire disabled"), Weapon->IsAutoFireEnabled());

	Weapon->SetAutoFireEnabled(true);
	TestTrue(TEXT("Auto-fire re-enabled"), Weapon->IsAutoFireEnabled());
	return true;
}

// ============================================================================
// 4. WEAPON: CanFire Requirements
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeapon_CanFire_DisabledWeapon,
	"Odyssey.Combat.AutoWeapon.CanFire.DisabledWeapon",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeapon_CanFire_DisabledWeapon::RunTest(const FString& Parameters)
{
	UAutoWeaponSystem* Weapon = NewObject<UAutoWeaponSystem>();
	Weapon->SetWeaponEnabled(false);

	TestFalse(TEXT("Cannot fire when disabled"), Weapon->CanFire());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeapon_CanFire_NoTargetingSystem,
	"Odyssey.Combat.AutoWeapon.CanFire.NoTargetingSystem",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeapon_CanFire_NoTargetingSystem::RunTest(const FString& Parameters)
{
	UAutoWeaponSystem* Weapon = NewObject<UAutoWeaponSystem>();

	// No targeting system linked
	TestFalse(TEXT("Cannot fire without targeting system"), Weapon->CanFire());
	return true;
}

// ============================================================================
// 5. WEAPON: FireOnce Without Target
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeapon_Fire_NoTarget,
	"Odyssey.Combat.AutoWeapon.Fire.NoTarget",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeapon_Fire_NoTarget::RunTest(const FString& Parameters)
{
	UAutoWeaponSystem* Weapon = NewObject<UAutoWeaponSystem>();

	FCombatFireResult Result = Weapon->FireOnce();
	TestFalse(TEXT("Cannot fire without target"), Result.bFired);
	TestEqual(TEXT("Failure reason = CannotFire"), Result.FailReason, FName("CannotFire"));
	return true;
}

// ============================================================================
// 6. WEAPON: Session Stats
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeapon_Stats_Reset,
	"Odyssey.Combat.AutoWeapon.Stats.Reset",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeapon_Stats_Reset::RunTest(const FString& Parameters)
{
	UAutoWeaponSystem* Weapon = NewObject<UAutoWeaponSystem>();

	FCombatSessionStats Stats = Weapon->GetSessionStats();
	TestEqual(TEXT("Default shots fired"), Stats.ShotsFired, 0);
	TestEqual(TEXT("Default shots hit"), Stats.ShotsHit, 0);
	TestEqual(TEXT("Default crits"), Stats.CriticalHits, 0);
	TestEqual(TEXT("Default damage dealt"), Stats.TotalDamageDealt, 0.0f);
	TestEqual(TEXT("Default kills"), Stats.EnemiesDestroyed, 0);

	Weapon->ResetSessionStats();
	Stats = Weapon->GetSessionStats();
	TestEqual(TEXT("Stats reset"), Stats.ShotsFired, 0);
	return true;
}

// ============================================================================
// 7. WEAPON: Cooldown Progress
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeapon_Cooldown_ProgressInitial,
	"Odyssey.Combat.AutoWeapon.Cooldown.ProgressInitial",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeapon_Cooldown_ProgressInitial::RunTest(const FString& Parameters)
{
	UAutoWeaponSystem* Weapon = NewObject<UAutoWeaponSystem>();

	// Initially TimeSinceLastShot is 0, so cooldown progress = 0 / (1/3) = 0
	float Progress = Weapon->GetCooldownProgress();
	TestTrue(TEXT("Initial cooldown progress is 0"), FMath::IsNearlyEqual(Progress, 0.0f, 0.01f));
	return true;
}

// ============================================================================
// 8. WEAPON: Multiple Weapon Type Configs
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeapon_Types_LaserConfig,
	"Odyssey.Combat.AutoWeapon.Types.LaserConfig",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeapon_Types_LaserConfig::RunTest(const FString& Parameters)
{
	FAutoWeaponConfig LaserConfig;
	LaserConfig.BaseDamage = 15.0f;
	LaserConfig.FireRate = 8.0f; // Fast
	LaserConfig.EngagementRange = 1500.0f;
	LaserConfig.CritChance = 0.1f;
	LaserConfig.EnergyCost = 3;
	LaserConfig.ProjectileSpeed = 0.0f; // Hitscan

	TestEqual(TEXT("Laser damage"), LaserConfig.BaseDamage, 15.0f);
	TestEqual(TEXT("Laser fire rate"), LaserConfig.FireRate, 8.0f);
	TestEqual(TEXT("Laser is hitscan"), LaserConfig.ProjectileSpeed, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeapon_Types_PlasmaConfig,
	"Odyssey.Combat.AutoWeapon.Types.PlasmaConfig",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeapon_Types_PlasmaConfig::RunTest(const FString& Parameters)
{
	FAutoWeaponConfig PlasmaConfig;
	PlasmaConfig.BaseDamage = 40.0f;
	PlasmaConfig.FireRate = 1.5f; // Slow
	PlasmaConfig.EngagementRange = 2500.0f;
	PlasmaConfig.CritChance = 0.15f;
	PlasmaConfig.CritMultiplier = 2.5f;
	PlasmaConfig.EnergyCost = 15;
	PlasmaConfig.ProjectileSpeed = 3000.0f; // Projectile

	TestEqual(TEXT("Plasma damage"), PlasmaConfig.BaseDamage, 40.0f);
	TestEqual(TEXT("Plasma fire rate"), PlasmaConfig.FireRate, 1.5f);
	TestTrue(TEXT("Plasma is projectile"), PlasmaConfig.ProjectileSpeed > 0.0f);
	TestEqual(TEXT("Plasma energy cost"), PlasmaConfig.EnergyCost, 15);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeapon_Types_KineticConfig,
	"Odyssey.Combat.AutoWeapon.Types.KineticConfig",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeapon_Types_KineticConfig::RunTest(const FString& Parameters)
{
	FAutoWeaponConfig KineticConfig;
	KineticConfig.BaseDamage = 25.0f;
	KineticConfig.FireRate = 5.0f;
	KineticConfig.EngagementRange = 1800.0f;
	KineticConfig.Accuracy = 0.85f;
	KineticConfig.EnergyCost = 0; // Free firing
	KineticConfig.ProjectileSpeed = 5000.0f; // Fast projectile

	TestEqual(TEXT("Kinetic damage"), KineticConfig.BaseDamage, 25.0f);
	TestEqual(TEXT("Kinetic no energy cost"), KineticConfig.EnergyCost, 0);
	TestTrue(TEXT("Kinetic less accurate"), KineticConfig.Accuracy < 0.9f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeapon_Types_MissileConfig,
	"Odyssey.Combat.AutoWeapon.Types.MissileConfig",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeapon_Types_MissileConfig::RunTest(const FString& Parameters)
{
	FAutoWeaponConfig MissileConfig;
	MissileConfig.BaseDamage = 100.0f;
	MissileConfig.FireRate = 0.3f; // Very slow
	MissileConfig.EngagementRange = 4000.0f; // Long range
	MissileConfig.Accuracy = 0.98f; // Near perfect
	MissileConfig.CritChance = 0.2f;
	MissileConfig.CritMultiplier = 3.0f;
	MissileConfig.EnergyCost = 25;
	MissileConfig.ProjectileSpeed = 2000.0f; // Slow but guided

	TestEqual(TEXT("Missile damage"), MissileConfig.BaseDamage, 100.0f);
	TestEqual(TEXT("Missile fire rate"), MissileConfig.FireRate, 0.3f);
	TestEqual(TEXT("Missile range"), MissileConfig.EngagementRange, 4000.0f);
	TestEqual(TEXT("Missile energy cost"), MissileConfig.EnergyCost, 25);
	return true;
}

// ============================================================================
// 9. WEAPON: Engagement State Enum
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeapon_EngagementState_Enum,
	"Odyssey.Combat.AutoWeapon.EngagementState.Enum",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeapon_EngagementState_Enum::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Idle = 0"), static_cast<uint8>(ECombatEngagementState::Idle) == 0);
	TestTrue(TEXT("Scanning = 1"), static_cast<uint8>(ECombatEngagementState::Scanning) == 1);
	TestTrue(TEXT("Locked = 2"), static_cast<uint8>(ECombatEngagementState::Locked) == 2);
	TestTrue(TEXT("Firing = 3"), static_cast<uint8>(ECombatEngagementState::Firing) == 3);
	TestTrue(TEXT("Cooldown = 4"), static_cast<uint8>(ECombatEngagementState::Cooldown) == 4);
	TestTrue(TEXT("Disengaging = 5"), static_cast<uint8>(ECombatEngagementState::Disengaging) == 5);
	return true;
}

// ============================================================================
// 10. WEAPON: Combat Session Stats Utility Functions
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeapon_SessionStats_Calculations,
	"Odyssey.Combat.AutoWeapon.SessionStats.Calculations",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeapon_SessionStats_Calculations::RunTest(const FString& Parameters)
{
	FCombatSessionStats Stats;
	Stats.ShotsFired = 100;
	Stats.ShotsHit = 75;
	Stats.CriticalHits = 15;
	Stats.TotalDamageDealt = 1500.0f;
	Stats.EngagementDuration = 30.0f;

	TestTrue(TEXT("Accuracy = 75%"), FMath::IsNearlyEqual(Stats.GetAccuracy(), 0.75f, 0.01f));
	TestTrue(TEXT("Crit rate = 20%"), FMath::IsNearlyEqual(Stats.GetCritRate(), 0.2f, 0.01f));
	TestTrue(TEXT("DPS = 50"), FMath::IsNearlyEqual(Stats.GetDPS(), 50.0f, 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeapon_SessionStats_ZeroDivision,
	"Odyssey.Combat.AutoWeapon.SessionStats.ZeroDivision",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeapon_SessionStats_ZeroDivision::RunTest(const FString& Parameters)
{
	FCombatSessionStats Stats; // All zero

	TestEqual(TEXT("Accuracy with zero shots = 0"), Stats.GetAccuracy(), 0.0f);
	TestEqual(TEXT("Crit rate with zero hits = 0"), Stats.GetCritRate(), 0.0f);
	TestEqual(TEXT("DPS with zero time = 0"), Stats.GetDPS(), 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeapon_SessionStats_Reset,
	"Odyssey.Combat.AutoWeapon.SessionStats.Reset",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeapon_SessionStats_Reset::RunTest(const FString& Parameters)
{
	FCombatSessionStats Stats;
	Stats.ShotsFired = 50;
	Stats.ShotsHit = 30;
	Stats.CriticalHits = 5;
	Stats.TotalDamageDealt = 800.0f;
	Stats.EnemiesDestroyed = 3;
	Stats.EngagementDuration = 20.0f;

	Stats.Reset();
	TestEqual(TEXT("ShotsFired reset"), Stats.ShotsFired, 0);
	TestEqual(TEXT("ShotsHit reset"), Stats.ShotsHit, 0);
	TestEqual(TEXT("CriticalHits reset"), Stats.CriticalHits, 0);
	TestEqual(TEXT("TotalDamageDealt reset"), Stats.TotalDamageDealt, 0.0f);
	TestEqual(TEXT("EnemiesDestroyed reset"), Stats.EnemiesDestroyed, 0);
	TestEqual(TEXT("EngagementDuration reset"), Stats.EngagementDuration, 0.0f);
	return true;
}

// ============================================================================
// 11. WEAPON: Fire Result Structure
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeapon_FireResult_Defaults,
	"Odyssey.Combat.AutoWeapon.FireResult.Defaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeapon_FireResult_Defaults::RunTest(const FString& Parameters)
{
	FCombatFireResult Result;
	TestFalse(TEXT("Default not fired"), Result.bFired);
	TestFalse(TEXT("Default not hit"), Result.bHit);
	TestFalse(TEXT("Default not critical"), Result.bCritical);
	TestFalse(TEXT("Default not killing blow"), Result.bKillingBlow);
	TestEqual(TEXT("Default zero damage"), Result.DamageDealt, 0.0f);
	TestEqual(TEXT("Default zero impact"), Result.ImpactLocation, FVector::ZeroVector);
	TestFalse(TEXT("Default no hit actor"), Result.HitActor.IsValid());
	return true;
}

// ============================================================================
// 12. WEAPON: SetTargetingSystem
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeapon_TargetSystem_Set,
	"Odyssey.Combat.AutoWeapon.TargetSystem.Set",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeapon_TargetSystem_Set::RunTest(const FString& Parameters)
{
	UAutoWeaponSystem* Weapon = NewObject<UAutoWeaponSystem>();

	// Setting null should not crash
	Weapon->SetTargetingSystem(nullptr);
	TestFalse(TEXT("Cannot fire without targeting"), Weapon->CanFire());
	return true;
}
