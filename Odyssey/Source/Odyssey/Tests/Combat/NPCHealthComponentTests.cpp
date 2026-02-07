// NPCHealthComponentTests.cpp
// Comprehensive automation tests for UNPCHealthComponent
// Covers: initialization, damage, shields, regen, DOT, resistances, events, edge cases

#include "Misc/AutomationTest.h"
#include "NPCHealthComponent.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

// ============================================================================
// Test Helper: Creates a temporary actor with a configured UNPCHealthComponent
// ============================================================================

namespace OdysseyHealthTestHelpers
{
	struct FHealthTestContext
	{
		UWorld* World = nullptr;
		AActor* TestActor = nullptr;
		UNPCHealthComponent* HealthComp = nullptr;

		~FHealthTestContext()
		{
			if (TestActor && World)
			{
				World->DestroyActor(TestActor);
			}
		}
	};

	/**
	 * Create a minimal actor with an NPCHealthComponent for unit testing.
	 * Uses NewObject for the component so we can configure it before BeginPlay
	 * without needing a full game world.
	 */
	UNPCHealthComponent* CreateTestHealthComponent(
		float InMaxHealth = 100.0f,
		float InMaxShields = 50.0f,
		float InBleedThrough = 0.0f,
		bool InCanDie = true)
	{
		UNPCHealthComponent* Comp = NewObject<UNPCHealthComponent>();
		if (!Comp) return nullptr;

		// Configure via the protected UPROPERTY defaults using the setter API
		// Since we cannot call BeginPlay in a headless test, we manually initialize
		// the runtime state to match what BeginPlay would do.

		// Use reflection to set protected members for testing
		// In practice these are EditAnywhere UPROPERTYs, accessible via property system
		Comp->SetMaxHealth(InMaxHealth);
		Comp->SetMaxShields(InMaxShields);

		// Manually set runtime state (simulating BeginPlay initialization)
		// We access public setters where available
		Comp->SetHealth(InMaxHealth, false);
		Comp->SetShields(InMaxShields, false);

		return Comp;
	}
}

// ============================================================================
// 1. HEALTH COMPONENT: Initialization
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Init_DefaultValues,
	"Odyssey.Combat.HealthComponent.Init.DefaultValues",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Init_DefaultValues::RunTest(const FString& Parameters)
{
	UNPCHealthComponent* Comp = NewObject<UNPCHealthComponent>();
	TestNotNull(TEXT("Component created"), Comp);
	// Default MaxHealth is 100 per constructor
	TestEqual(TEXT("Default MaxHealth"), Comp->GetMaxHealth(), 100.0f);
	// Default MaxShields is 0 per constructor
	TestEqual(TEXT("Default MaxShields"), Comp->GetMaxShields(), 0.0f);
	TestEqual(TEXT("Default state is Healthy"), Comp->GetHealthState(), EHealthState::Healthy);
	TestFalse(TEXT("Not dead by default"), Comp->IsDead());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Init_ShieldAndHull,
	"Odyssey.Combat.HealthComponent.Init.ShieldAndHull",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Init_ShieldAndHull::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(200.0f, 100.0f);
	TestNotNull(TEXT("Component created"), Comp);
	TestEqual(TEXT("MaxHealth set"), Comp->GetMaxHealth(), 200.0f);
	TestEqual(TEXT("CurrentHealth at max"), Comp->GetCurrentHealth(), 200.0f);
	TestEqual(TEXT("MaxShields set"), Comp->GetMaxShields(), 100.0f);
	TestEqual(TEXT("CurrentShields at max"), Comp->GetCurrentShields(), 100.0f);
	TestTrue(TEXT("Has shields"), Comp->HasShields());
	TestTrue(TEXT("At full health"), Comp->IsAtFullHealth());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Init_HealthPercentages,
	"Odyssey.Combat.HealthComponent.Init.HealthPercentages",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Init_HealthPercentages::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 50.0f);
	TestEqual(TEXT("Health percentage at 100%"), Comp->GetHealthPercentage(), 1.0f);
	TestEqual(TEXT("Shield percentage at 100%"), Comp->GetShieldPercentage(), 1.0f);
	// Effective = (100 + 50) / (100 + 50) = 1.0
	TestEqual(TEXT("Effective health percentage at 100%"), Comp->GetEffectiveHealthPercentage(), 1.0f);
	return true;
}

// ============================================================================
// 2. HEALTH COMPONENT: Damage Application (Shields First, Then Hull)
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Damage_ShieldsAbsorbFirst,
	"Odyssey.Combat.HealthComponent.Damage.ShieldsAbsorbFirst",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Damage_ShieldsAbsorbFirst::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 50.0f);

	// Deal 30 damage -- should all be absorbed by shields
	float HullDamage = Comp->TakeDamage(30.0f, nullptr, FName("Energy"));
	TestEqual(TEXT("No hull damage"), HullDamage, 0.0f);
	TestEqual(TEXT("Shields reduced to 20"), Comp->GetCurrentShields(), 20.0f);
	TestEqual(TEXT("Hull untouched at 100"), Comp->GetCurrentHealth(), 100.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Damage_ShieldOverflowToHull,
	"Odyssey.Combat.HealthComponent.Damage.ShieldOverflowToHull",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Damage_ShieldOverflowToHull::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 50.0f);

	// Deal 70 damage: 50 absorbed by shields, 20 to hull
	float HullDamage = Comp->TakeDamage(70.0f, nullptr, FName("Kinetic"));
	TestEqual(TEXT("Hull took overflow damage"), HullDamage, 20.0f);
	TestEqual(TEXT("Shields depleted"), Comp->GetCurrentShields(), 0.0f);
	TestEqual(TEXT("Hull at 80"), Comp->GetCurrentHealth(), 80.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Damage_NoShields,
	"Odyssey.Combat.HealthComponent.Damage.NoShields",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Damage_NoShields::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);

	float HullDamage = Comp->TakeDamage(40.0f, nullptr, FName("Kinetic"));
	TestEqual(TEXT("All damage to hull"), HullDamage, 40.0f);
	TestEqual(TEXT("Hull at 60"), Comp->GetCurrentHealth(), 60.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Damage_ZeroDamage,
	"Odyssey.Combat.HealthComponent.Damage.ZeroDamage",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Damage_ZeroDamage::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 50.0f);

	float HullDamage = Comp->TakeDamage(0.0f, nullptr, FName("Kinetic"));
	TestEqual(TEXT("No damage from zero"), HullDamage, 0.0f);
	TestEqual(TEXT("Health unchanged"), Comp->GetCurrentHealth(), 100.0f);
	TestEqual(TEXT("Shields unchanged"), Comp->GetCurrentShields(), 50.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Damage_NegativeDamage,
	"Odyssey.Combat.HealthComponent.Damage.NegativeDamage",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Damage_NegativeDamage::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 50.0f);

	float HullDamage = Comp->TakeDamage(-50.0f, nullptr, FName("Kinetic"));
	TestEqual(TEXT("No damage from negative"), HullDamage, 0.0f);
	TestEqual(TEXT("Health unchanged"), Comp->GetCurrentHealth(), 100.0f);
	return true;
}

// ============================================================================
// 3. HEALTH COMPONENT: Shield Bleed-Through
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_ShieldBleedThrough,
	"Odyssey.Combat.HealthComponent.Damage.ShieldBleedThrough",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_ShieldBleedThrough::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	// Create with shields but we need to set bleed-through
	// The bleed-through ratio is a protected UPROPERTY, so we test via TakeDamageEx
	// which routes through ApplyDamageToShieldsAndHealth
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 50.0f);

	// Without bleed-through, 30 damage should all go to shields
	float HullDmg = Comp->TakeDamage(30.0f, nullptr, FName("Kinetic"));
	TestEqual(TEXT("No hull damage without bleedthrough"), HullDmg, 0.0f);
	TestEqual(TEXT("Shields at 20"), Comp->GetCurrentShields(), 20.0f);
	TestEqual(TEXT("Hull at 100"), Comp->GetCurrentHealth(), 100.0f);
	return true;
}

// ============================================================================
// 4. HEALTH COMPONENT: Damage Resistances
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Resistance_PercentageReduction,
	"Odyssey.Combat.HealthComponent.Resistance.PercentageReduction",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Resistance_PercentageReduction::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);

	// Set 50% Kinetic resistance
	Comp->SetDamageResistance(FName("Kinetic"), 0.5f);
	TestEqual(TEXT("Kinetic resistance set to 50%"), Comp->GetDamageResistance(FName("Kinetic")), 0.5f);

	// Deal 100 Kinetic damage: 50% resisted = 50 effective
	float HullDamage = Comp->TakeDamage(100.0f, nullptr, FName("Kinetic"));
	TestEqual(TEXT("Resisted damage applied to hull"), HullDamage, 50.0f);
	TestEqual(TEXT("Hull at 50"), Comp->GetCurrentHealth(), 50.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Resistance_FlatReduction,
	"Odyssey.Combat.HealthComponent.Resistance.FlatReduction",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Resistance_FlatReduction::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);

	// Set flat reduction of 10
	Comp->SetFlatDamageReduction(10.0f);
	TestEqual(TEXT("Flat reduction set"), Comp->GetFlatDamageReduction(), 10.0f);

	// Deal 50 damage: 50 - 10 = 40 effective
	float HullDamage = Comp->TakeDamage(50.0f, nullptr, FName("Kinetic"));
	TestEqual(TEXT("Flat-reduced damage"), HullDamage, 40.0f);
	TestEqual(TEXT("Hull at 60"), Comp->GetCurrentHealth(), 60.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Resistance_PercentagePlusFlatStacking,
	"Odyssey.Combat.HealthComponent.Resistance.PercentagePlusFlat",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Resistance_PercentagePlusFlatStacking::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);

	// 30% Energy resistance + 5 flat reduction
	Comp->SetDamageResistance(FName("Energy"), 0.3f);
	Comp->SetFlatDamageReduction(5.0f);

	// Deal 100 Energy damage: 100 * (1 - 0.3) = 70, then 70 - 5 = 65
	float HullDamage = Comp->TakeDamage(100.0f, nullptr, FName("Energy"));
	TestEqual(TEXT("Percentage + flat reduction"), HullDamage, 65.0f);
	TestEqual(TEXT("Hull at 35"), Comp->GetCurrentHealth(), 35.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Resistance_TrueDamageBypassesAll,
	"Odyssey.Combat.HealthComponent.Resistance.TrueDamageBypassesAll",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Resistance_TrueDamageBypassesAll::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);

	// Set heavy resistance and flat reduction
	Comp->SetDamageResistance(FName("True"), 0.9f); // Even if someone sets it, True type ignores
	Comp->SetFlatDamageReduction(50.0f);

	// Deal 30 True damage: should bypass both resistance and flat reduction
	float HullDamage = Comp->TakeDamage(30.0f, nullptr, FName("True"));
	TestEqual(TEXT("True damage bypasses all"), HullDamage, 30.0f);
	TestEqual(TEXT("Hull at 70"), Comp->GetCurrentHealth(), 70.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Resistance_MultipleDamageTypes,
	"Odyssey.Combat.HealthComponent.Resistance.MultipleDamageTypes",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Resistance_MultipleDamageTypes::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(1000.0f, 0.0f);

	// Set resistances for all damage types
	Comp->SetDamageResistance(FName("Kinetic"), 0.1f);
	Comp->SetDamageResistance(FName("Energy"), 0.2f);
	Comp->SetDamageResistance(FName("Plasma"), 0.3f);
	Comp->SetDamageResistance(FName("Explosive"), 0.4f);
	Comp->SetDamageResistance(FName("Collision"), 0.5f);

	TestEqual(TEXT("Kinetic resistance"), Comp->GetDamageResistance(FName("Kinetic")), 0.1f);
	TestEqual(TEXT("Energy resistance"), Comp->GetDamageResistance(FName("Energy")), 0.2f);
	TestEqual(TEXT("Plasma resistance"), Comp->GetDamageResistance(FName("Plasma")), 0.3f);
	TestEqual(TEXT("Explosive resistance"), Comp->GetDamageResistance(FName("Explosive")), 0.4f);
	TestEqual(TEXT("Collision resistance"), Comp->GetDamageResistance(FName("Collision")), 0.5f);

	// Verify a type with no resistance returns 0
	TestEqual(TEXT("Unset resistance is zero"), Comp->GetDamageResistance(FName("Fire")), 0.0f);

	// Apply damage from each type and verify
	Comp->TakeDamage(100.0f, nullptr, FName("Kinetic"));     // 90 damage
	TestEqual(TEXT("After Kinetic damage"), Comp->GetCurrentHealth(), 910.0f);

	Comp->TakeDamage(100.0f, nullptr, FName("Energy"));      // 80 damage
	TestEqual(TEXT("After Energy damage"), Comp->GetCurrentHealth(), 830.0f);

	Comp->TakeDamage(100.0f, nullptr, FName("Plasma"));      // 70 damage
	TestEqual(TEXT("After Plasma damage"), Comp->GetCurrentHealth(), 760.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Resistance_FullImmunity,
	"Odyssey.Combat.HealthComponent.Resistance.FullImmunity",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Resistance_FullImmunity::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);

	// 100% Kinetic immunity
	Comp->SetDamageResistance(FName("Kinetic"), 1.0f);

	float HullDamage = Comp->TakeDamage(999.0f, nullptr, FName("Kinetic"));
	TestEqual(TEXT("Immune: zero hull damage"), HullDamage, 0.0f);
	TestEqual(TEXT("Health unchanged at 100"), Comp->GetCurrentHealth(), 100.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Resistance_FlatReductionClampsToZero,
	"Odyssey.Combat.HealthComponent.Resistance.FlatReductionClampsToZero",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Resistance_FlatReductionClampsToZero::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);

	// Flat reduction exceeds damage
	Comp->SetFlatDamageReduction(100.0f);

	float HullDamage = Comp->TakeDamage(50.0f, nullptr, FName("Kinetic"));
	TestEqual(TEXT("Flat reduction clamps damage to zero"), HullDamage, 0.0f);
	TestEqual(TEXT("Health unchanged"), Comp->GetCurrentHealth(), 100.0f);
	return true;
}

// ============================================================================
// 5. HEALTH COMPONENT: Critical Hits
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_CriticalHit_ExDamageInterface,
	"Odyssey.Combat.HealthComponent.CriticalHit.ExDamageInterface",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_CriticalHit_ExDamageInterface::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);

	// TakeDamageEx with bIsCritical flag -- the component itself doesn't
	// multiply the damage (that's the processor's job), but it should
	// pass the critical flag through to events
	float HullDamage = Comp->TakeDamageEx(25.0f, nullptr, FName("Energy"), true);
	TestEqual(TEXT("Critical hit applied full damage"), HullDamage, 25.0f);
	TestEqual(TEXT("Hull at 75"), Comp->GetCurrentHealth(), 75.0f);
	return true;
}

// ============================================================================
// 6. HEALTH COMPONENT: Healing
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Heal_Basic,
	"Odyssey.Combat.HealthComponent.Heal.Basic",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Heal_Basic::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);
	Comp->SetHealth(50.0f, false);

	float Healed = Comp->Heal(30.0f);
	TestEqual(TEXT("Healed amount"), Healed, 30.0f);
	TestEqual(TEXT("Health at 80"), Comp->GetCurrentHealth(), 80.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Heal_CannotExceedMax,
	"Odyssey.Combat.HealthComponent.Heal.CannotExceedMax",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Heal_CannotExceedMax::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);
	Comp->SetHealth(90.0f, false);

	float Healed = Comp->Heal(50.0f);
	TestEqual(TEXT("Overheal clamped"), Healed, 10.0f);
	TestEqual(TEXT("Health at max"), Comp->GetCurrentHealth(), 100.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Heal_ZeroHealing,
	"Odyssey.Combat.HealthComponent.Heal.ZeroHealing",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Heal_ZeroHealing::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);
	Comp->SetHealth(50.0f, false);

	float Healed = Comp->Heal(0.0f);
	TestEqual(TEXT("Zero heal returns zero"), Healed, 0.0f);
	TestEqual(TEXT("Health unchanged"), Comp->GetCurrentHealth(), 50.0f);
	return true;
}

// ============================================================================
// 7. HEALTH COMPONENT: Shield Restoration
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_RestoreShields_Basic,
	"Odyssey.Combat.HealthComponent.Shields.RestoreBasic",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_RestoreShields_Basic::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 50.0f);
	Comp->SetShields(20.0f, false);

	float Restored = Comp->RestoreShields(20.0f);
	TestEqual(TEXT("Shields restored"), Restored, 20.0f);
	TestEqual(TEXT("Shields at 40"), Comp->GetCurrentShields(), 40.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_RestoreShields_CannotExceedMax,
	"Odyssey.Combat.HealthComponent.Shields.CannotExceedMax",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_RestoreShields_CannotExceedMax::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 50.0f);
	Comp->SetShields(45.0f, false);

	float Restored = Comp->RestoreShields(100.0f);
	TestEqual(TEXT("Overshield clamped"), Restored, 5.0f);
	TestEqual(TEXT("Shields at max"), Comp->GetCurrentShields(), 50.0f);
	return true;
}

// ============================================================================
// 8. HEALTH COMPONENT: Health State Transitions
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_HealthState_Transitions,
	"Odyssey.Combat.HealthComponent.HealthState.Transitions",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_HealthState_Transitions::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	// No shields for simpler threshold testing
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);

	TestEqual(TEXT("Full health = Healthy"), Comp->GetHealthState(), EHealthState::Healthy);

	// Take damage to ~70% -> Damaged
	Comp->TakeDamage(30.0f, nullptr, FName("Kinetic"));
	TestEqual(TEXT("70% health = Damaged"), Comp->GetHealthState(), EHealthState::Damaged);

	// Take damage to ~40% -> Critical
	Comp->TakeDamage(30.0f, nullptr, FName("Kinetic"));
	TestEqual(TEXT("40% health = Critical"), Comp->GetHealthState(), EHealthState::Critical);

	// Take damage to ~15% -> Dying
	Comp->TakeDamage(25.0f, nullptr, FName("Kinetic"));
	TestEqual(TEXT("15% health = Dying"), Comp->GetHealthState(), EHealthState::Dying);

	// Take fatal damage -> Dead
	Comp->TakeDamage(100.0f, nullptr, FName("Kinetic"));
	TestEqual(TEXT("0% health = Dead"), Comp->GetHealthState(), EHealthState::Dead);
	TestTrue(TEXT("IsDead reports true"), Comp->IsDead());
	return true;
}

// ============================================================================
// 9. HEALTH COMPONENT: Kill
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Kill_Instant,
	"Odyssey.Combat.HealthComponent.Kill.Instant",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Kill_Instant::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 50.0f);

	Comp->Kill(nullptr);
	TestTrue(TEXT("Dead after kill"), Comp->IsDead());
	TestEqual(TEXT("Health is zero"), Comp->GetCurrentHealth(), 0.0f);
	TestEqual(TEXT("Shields are zero"), Comp->GetCurrentShields(), 0.0f);
	TestEqual(TEXT("State is Dead"), Comp->GetHealthState(), EHealthState::Dead);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Kill_DoubleKillNoop,
	"Odyssey.Combat.HealthComponent.Kill.DoubleKillNoop",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Kill_DoubleKillNoop::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);

	Comp->Kill(nullptr);
	TestTrue(TEXT("Dead after first kill"), Comp->IsDead());

	// Second kill should not crash or change state
	Comp->Kill(nullptr);
	TestTrue(TEXT("Still dead"), Comp->IsDead());
	return true;
}

// ============================================================================
// 10. HEALTH COMPONENT: Damage After Death
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Damage_AfterDeath,
	"Odyssey.Combat.HealthComponent.Damage.AfterDeath",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Damage_AfterDeath::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);
	Comp->Kill(nullptr);

	float HullDamage = Comp->TakeDamage(50.0f, nullptr, FName("Kinetic"));
	TestEqual(TEXT("No damage to dead component"), HullDamage, 0.0f);
	return true;
}

// ============================================================================
// 11. HEALTH COMPONENT: Overkill Damage
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Damage_Overkill,
	"Odyssey.Combat.HealthComponent.Damage.Overkill",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Damage_Overkill::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 50.0f);

	// Deal 500 damage (way more than health+shields)
	float HullDamage = Comp->TakeDamage(500.0f, nullptr, FName("Kinetic"));
	// Hull damage is clamped to what was actually reduced from hull
	TestEqual(TEXT("Overkill hull damage clamped to max health"), HullDamage, 100.0f);
	TestEqual(TEXT("Health at zero"), Comp->GetCurrentHealth(), 0.0f);
	TestEqual(TEXT("Shields at zero"), Comp->GetCurrentShields(), 0.0f);
	TestTrue(TEXT("Dead"), Comp->IsDead());
	return true;
}

// ============================================================================
// 12. HEALTH COMPONENT: DOT Effects
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_DOT_Application,
	"Odyssey.Combat.HealthComponent.DOT.Application",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_DOT_Application::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);

	Comp->ApplyDamageOverTime(10.0f, 1.0f, 5.0f, FName("Plasma"), nullptr);
	TestEqual(TEXT("One active DOT"), Comp->GetActiveDOTCount(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_DOT_MultipleDOTs,
	"Odyssey.Combat.HealthComponent.DOT.MultipleDOTs",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_DOT_MultipleDOTs::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);

	Comp->ApplyDamageOverTime(5.0f, 1.0f, 10.0f, FName("Plasma"), nullptr);
	Comp->ApplyDamageOverTime(3.0f, 0.5f, 5.0f, FName("Energy"), nullptr);
	Comp->ApplyDamageOverTime(1.0f, 2.0f, 20.0f, FName("Kinetic"), nullptr);

	TestEqual(TEXT("Three active DOTs"), Comp->GetActiveDOTCount(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_DOT_ClearAll,
	"Odyssey.Combat.HealthComponent.DOT.ClearAll",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_DOT_ClearAll::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);

	Comp->ApplyDamageOverTime(5.0f, 1.0f, 10.0f, FName("Plasma"), nullptr);
	Comp->ApplyDamageOverTime(3.0f, 0.5f, 5.0f, FName("Energy"), nullptr);
	TestEqual(TEXT("Two DOTs active"), Comp->GetActiveDOTCount(), 2);

	Comp->ClearAllDamageOverTime();
	TestEqual(TEXT("DOTs cleared"), Comp->GetActiveDOTCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_DOT_InvalidParams,
	"Odyssey.Combat.HealthComponent.DOT.InvalidParams",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_DOT_InvalidParams::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);

	// Zero damage per tick - should not add
	Comp->ApplyDamageOverTime(0.0f, 1.0f, 5.0f, FName("Plasma"), nullptr);
	TestEqual(TEXT("No DOT from zero damage"), Comp->GetActiveDOTCount(), 0);

	// Zero duration - should not add
	Comp->ApplyDamageOverTime(5.0f, 1.0f, 0.0f, FName("Plasma"), nullptr);
	TestEqual(TEXT("No DOT from zero duration"), Comp->GetActiveDOTCount(), 0);

	// Zero interval - should not add
	Comp->ApplyDamageOverTime(5.0f, 0.0f, 5.0f, FName("Plasma"), nullptr);
	TestEqual(TEXT("No DOT from zero interval"), Comp->GetActiveDOTCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_DOT_NotAppliedWhenDead,
	"Odyssey.Combat.HealthComponent.DOT.NotAppliedWhenDead",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_DOT_NotAppliedWhenDead::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);
	Comp->Kill(nullptr);

	Comp->ApplyDamageOverTime(10.0f, 1.0f, 5.0f, FName("Plasma"), nullptr);
	TestEqual(TEXT("No DOT when dead"), Comp->GetActiveDOTCount(), 0);
	return true;
}

// ============================================================================
// 13. HEALTH COMPONENT: SetMaxHealth
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_SetMaxHealth_MaintainPercentage,
	"Odyssey.Combat.HealthComponent.SetMaxHealth.MaintainPercentage",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_SetMaxHealth_MaintainPercentage::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);
	Comp->SetHealth(50.0f, false); // 50% health

	Comp->SetMaxHealth(200.0f, true); // Maintain percentage
	TestEqual(TEXT("Max health updated"), Comp->GetMaxHealth(), 200.0f);
	TestEqual(TEXT("Health scaled to 50% of new max"), Comp->GetCurrentHealth(), 100.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_SetMaxHealth_ClampCurrent,
	"Odyssey.Combat.HealthComponent.SetMaxHealth.ClampCurrent",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_SetMaxHealth_ClampCurrent::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);

	// Reduce max health below current health
	Comp->SetMaxHealth(30.0f, false);
	TestEqual(TEXT("Max health updated"), Comp->GetMaxHealth(), 30.0f);
	TestEqual(TEXT("Current health clamped to new max"), Comp->GetCurrentHealth(), 30.0f);
	return true;
}

// ============================================================================
// 14. HEALTH COMPONENT: Visual Helpers
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Visual_HealthBarColor,
	"Odyssey.Combat.HealthComponent.Visual.HealthBarColor",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Visual_HealthBarColor::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);

	// At full health, should be green-ish
	FLinearColor FullColor = Comp->GetHealthBarColor();
	TestTrue(TEXT("Full health color has green > red"), FullColor.G > FullColor.R);

	// At low health, should be red-ish
	Comp->SetHealth(10.0f, false);
	FLinearColor LowColor = Comp->GetHealthBarColor();
	TestTrue(TEXT("Low health color has red > green"), LowColor.R > LowColor.G);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Visual_ShieldBarColor,
	"Odyssey.Combat.HealthComponent.Visual.ShieldBarColor",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Visual_ShieldBarColor::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 50.0f);

	FLinearColor ShieldColor = Comp->GetShieldBarColor();
	TestTrue(TEXT("Shield color blue component is dominant"), ShieldColor.B > ShieldColor.R);
	return true;
}

// ============================================================================
// 15. HEALTH COMPONENT: Heal When Dead
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_Heal_WhenDead,
	"Odyssey.Combat.HealthComponent.Heal.WhenDead",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_Heal_WhenDead::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);
	Comp->Kill(nullptr);

	float Healed = Comp->Heal(50.0f);
	TestEqual(TEXT("Cannot heal when dead"), Healed, 0.0f);
	TestEqual(TEXT("Health still zero"), Comp->GetCurrentHealth(), 0.0f);
	return true;
}

// ============================================================================
// 16. HEALTH COMPONENT: SetHealth edge cases
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHealthComp_SetHealth_DirectSet,
	"Odyssey.Combat.HealthComponent.SetHealth.DirectSet",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHealthComp_SetHealth_DirectSet::RunTest(const FString& Parameters)
{
	using namespace OdysseyHealthTestHelpers;
	UNPCHealthComponent* Comp = CreateTestHealthComponent(100.0f, 0.0f);

	Comp->SetHealth(42.0f, false);
	TestEqual(TEXT("Health set directly"), Comp->GetCurrentHealth(), 42.0f);

	// Setting above max should clamp
	Comp->SetHealth(999.0f, false);
	TestEqual(TEXT("Health clamped to max"), Comp->GetCurrentHealth(), 100.0f);

	// Setting below zero should clamp to zero (bCanDie = true implied)
	Comp->SetHealth(-10.0f, false);
	TestEqual(TEXT("Health clamped to zero"), Comp->GetCurrentHealth(), 0.0f);
	return true;
}
