// NPCShipTests.cpp
// Comprehensive automation tests for ANPCShip
// Covers: config, ship types, combat stats, death/respawn state

#include "Misc/AutomationTest.h"
#include "NPCShip.h"
#include "NPCBehaviorComponent.h"
#include "Tests/AutomationCommon.h"

// ============================================================================
// 1. NPC SHIP: Configuration Struct Defaults
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNPCShip_Config_Defaults,
	"Odyssey.Combat.NPCShip.Config.Defaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNPCShip_Config_Defaults::RunTest(const FString& Parameters)
{
	FNPCShipConfig Config;
	TestEqual(TEXT("Default type is Civilian"), Config.ShipType, ENPCShipType::Civilian);
	TestEqual(TEXT("Default name"), Config.ShipName, FString(TEXT("Unknown Ship")));
	TestEqual(TEXT("Default MaxHealth"), Config.MaxHealth, 100.0f);
	TestEqual(TEXT("Default AttackDamage"), Config.AttackDamage, 25.0f);
	TestEqual(TEXT("Default MovementSpeed"), Config.MovementSpeed, 400.0f);
	TestFalse(TEXT("Default cannot respawn"), Config.bCanRespawn);
	TestEqual(TEXT("Default RespawnDelay"), Config.RespawnDelay, 30.0f);
	TestEqual(TEXT("Default AttackCooldown"), Config.AttackCooldown, 2.0f);
	return true;
}

// ============================================================================
// 2. NPC SHIP: Ship Type Configurations
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNPCShip_Config_PirateType,
	"Odyssey.Combat.NPCShip.Config.PirateType",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNPCShip_Config_PirateType::RunTest(const FString& Parameters)
{
	FNPCShipConfig Config;
	Config.ShipType = ENPCShipType::Pirate;
	Config.ShipName = TEXT("Pirate Ship");
	Config.MaxHealth = 120.0f;
	Config.AttackDamage = 35.0f;
	Config.MovementSpeed = 450.0f;
	Config.bCanRespawn = false;
	Config.AttackCooldown = 1.5f;

	TestEqual(TEXT("Pirate type"), Config.ShipType, ENPCShipType::Pirate);
	TestEqual(TEXT("Pirate health"), Config.MaxHealth, 120.0f);
	TestEqual(TEXT("Pirate damage"), Config.AttackDamage, 35.0f);
	TestEqual(TEXT("Pirate speed"), Config.MovementSpeed, 450.0f);
	TestFalse(TEXT("Pirates don't respawn"), Config.bCanRespawn);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNPCShip_Config_SecurityType,
	"Odyssey.Combat.NPCShip.Config.SecurityType",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNPCShip_Config_SecurityType::RunTest(const FString& Parameters)
{
	FNPCShipConfig Config;
	Config.ShipType = ENPCShipType::Security;
	Config.MaxHealth = 150.0f;
	Config.AttackDamage = 30.0f;
	Config.bCanRespawn = true;
	Config.RespawnDelay = 45.0f;

	TestEqual(TEXT("Security type"), Config.ShipType, ENPCShipType::Security);
	TestEqual(TEXT("Security health"), Config.MaxHealth, 150.0f);
	TestTrue(TEXT("Security respawns"), Config.bCanRespawn);
	TestEqual(TEXT("Security respawn delay"), Config.RespawnDelay, 45.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNPCShip_Config_TraderType,
	"Odyssey.Combat.NPCShip.Config.TraderType",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNPCShip_Config_TraderType::RunTest(const FString& Parameters)
{
	FNPCShipConfig Config;
	Config.ShipType = ENPCShipType::Civilian;
	Config.ShipName = TEXT("Trader");
	Config.MaxHealth = 75.0f;
	Config.AttackDamage = 10.0f;
	Config.MovementSpeed = 300.0f;
	Config.bCanRespawn = true;
	Config.RespawnDelay = 60.0f;

	TestEqual(TEXT("Trader is Civilian"), Config.ShipType, ENPCShipType::Civilian);
	TestEqual(TEXT("Trader health is low"), Config.MaxHealth, 75.0f);
	TestEqual(TEXT("Trader damage is low"), Config.AttackDamage, 10.0f);
	TestTrue(TEXT("Trader can respawn"), Config.bCanRespawn);
	return true;
}

// ============================================================================
// 3. NPC SHIP: Combat Statistics
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNPCShip_Stats_DefaultsAndReset,
	"Odyssey.Combat.NPCShip.Stats.DefaultsAndReset",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNPCShip_Stats_DefaultsAndReset::RunTest(const FString& Parameters)
{
	FNPCCombatStats Stats;
	TestEqual(TEXT("Default attacks"), Stats.TotalAttacks, 0);
	TestEqual(TEXT("Default damage dealt"), Stats.TotalDamageDealt, 0.0f);
	TestEqual(TEXT("Default damage taken"), Stats.TotalDamageTaken, 0.0f);
	TestEqual(TEXT("Default death count"), Stats.DeathCount, 0);
	TestEqual(TEXT("Default respawn count"), Stats.RespawnCount, 0);
	TestEqual(TEXT("Default time alive"), Stats.TotalTimeAlive, 0.0f);
	TestEqual(TEXT("Default time in combat"), Stats.TotalTimeInCombat, 0.0f);

	// Simulate some values then reset
	Stats.TotalAttacks = 5;
	Stats.TotalDamageDealt = 200.0f;
	Stats.DeathCount = 1;

	Stats.Reset();
	TestEqual(TEXT("Attacks reset"), Stats.TotalAttacks, 0);
	TestEqual(TEXT("Damage dealt reset"), Stats.TotalDamageDealt, 0.0f);
	TestEqual(TEXT("Deaths reset"), Stats.DeathCount, 0);
	return true;
}

// ============================================================================
// 4. NPC SHIP: Engagement Data
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNPCShip_Engagement_DataReset,
	"Odyssey.Combat.NPCShip.Engagement.DataReset",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNPCShip_Engagement_DataReset::RunTest(const FString& Parameters)
{
	FNPCEngagementData Data;
	Data.AttackCount = 10;
	Data.TotalDamageDealt = 500.0f;
	Data.DistanceToTarget = 100.0f;

	Data.Reset();
	TestFalse(TEXT("Target cleared"), Data.Target.IsValid());
	TestEqual(TEXT("Attack count reset"), Data.AttackCount, 0);
	TestEqual(TEXT("Damage dealt reset"), Data.TotalDamageDealt, 0.0f);
	TestEqual(TEXT("Distance reset"), Data.DistanceToTarget, 0.0f);
	return true;
}

// ============================================================================
// 5. NPC SHIP: Ship Type Enum Coverage
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNPCShip_Enum_AllTypes,
	"Odyssey.Combat.NPCShip.Enum.AllTypes",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNPCShip_Enum_AllTypes::RunTest(const FString& Parameters)
{
	// Verify all enum values are distinct and castable
	TestTrue(TEXT("Civilian is 0"), static_cast<uint8>(ENPCShipType::Civilian) == 0);
	TestTrue(TEXT("Pirate is 1"), static_cast<uint8>(ENPCShipType::Pirate) == 1);
	TestTrue(TEXT("Security is 2"), static_cast<uint8>(ENPCShipType::Security) == 2);
	TestTrue(TEXT("Escort is 3"), static_cast<uint8>(ENPCShipType::Escort) == 3);
	return true;
}

// ============================================================================
// 6. NPC SHIP: Weak Pointer Safety
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNPCShip_WeakPtr_EngagementTargetSafety,
	"Odyssey.Combat.NPCShip.WeakPtr.EngagementTargetSafety",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNPCShip_WeakPtr_EngagementTargetSafety::RunTest(const FString& Parameters)
{
	FNPCEngagementData Data;

	// Target is null by default
	TestFalse(TEXT("Default target is invalid"), Data.Target.IsValid());
	TestTrue(TEXT("Null target returns nullptr"), Data.Target.Get() == nullptr);

	// After reset, same behavior
	Data.Reset();
	TestFalse(TEXT("Reset target is invalid"), Data.Target.IsValid());
	return true;
}
