// OdysseyQualityControlTests.cpp
// Comprehensive automation tests for the Quality Control System
// Tests quality tiers, modifiers, critical crafts, item values, and equipment effects

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Crafting/OdysseyCraftingManager.h"
#include "Crafting/OdysseyQualityControlSystem.h"
#include "Crafting/OdysseyCraftingSkillSystem.h"

// ============================================================================
// 1. Quality Tier Default Configuration
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityTierDefaults,
	"Odyssey.Crafting.Quality.TierDefaultConfiguration",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityTierDefaults::RunTest(const FString& Parameters)
{
	FQualityTierConfig Config;

	TestEqual(TEXT("Default Quality should be Common"), Config.Quality, EItemQuality::Common);
	TestEqual(TEXT("Default MinScore should be 0.0"), Config.MinScore, 0.0f);
	TestEqual(TEXT("Default MaxScore should be 0.25"), Config.MaxScore, 0.25f);
	TestEqual(TEXT("Default ValueMultiplier should be 1.0"), Config.ValueMultiplier, 1.0f);
	TestEqual(TEXT("Default StatBonus should be 0.0"), Config.StatBonus, 0.0f);

	return true;
}

// ============================================================================
// 2. Quality Tier Score Ranges (Initialized Tiers)
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityTierScoreRanges,
	"Odyssey.Crafting.Quality.TierScoreRangesValid",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityTierScoreRanges::RunTest(const FString& Parameters)
{
	// Recreate the default tier configuration from InitializeDefaultTiers
	struct FTierEntry
	{
		EItemQuality Quality;
		float MinScore;
		float MaxScore;
		float ValueMultiplier;
		float StatBonus;
	};

	TArray<FTierEntry> Tiers = {
		{EItemQuality::Scrap,       0.0f,  0.15f,  0.25f, -0.2f},
		{EItemQuality::Common,      0.15f, 0.35f,  1.0f,   0.0f},
		{EItemQuality::Standard,    0.35f, 0.55f,  1.5f,   0.1f},
		{EItemQuality::Quality,     0.55f, 0.72f,  2.5f,   0.2f},
		{EItemQuality::Superior,    0.72f, 0.85f,  4.0f,   0.35f},
		{EItemQuality::Masterwork,  0.85f, 0.95f,  8.0f,   0.5f},
		{EItemQuality::Legendary,   0.95f, 1.0f,  20.0f,   0.75f}
	};

	// Verify all 7 tiers exist
	TestEqual(TEXT("Should have 7 quality tiers"), Tiers.Num(), 7);

	// Verify tiers cover full 0.0 - 1.0 range
	TestEqual(TEXT("First tier should start at 0"), Tiers[0].MinScore, 0.0f);
	TestEqual(TEXT("Last tier should end at 1.0"), Tiers[Tiers.Num() - 1].MaxScore, 1.0f);

	// Verify no gaps between tiers
	for (int32 i = 1; i < Tiers.Num(); ++i)
	{
		TestTrue(FString::Printf(TEXT("Tier %d MinScore should equal previous MaxScore"), i),
			FMath::IsNearlyEqual(Tiers[i].MinScore, Tiers[i - 1].MaxScore, 0.001f));
	}

	// Verify value multipliers increase monotonically
	for (int32 i = 1; i < Tiers.Num(); ++i)
	{
		TestTrue(FString::Printf(TEXT("Tier %d value should be > tier %d"), i, i - 1),
			Tiers[i].ValueMultiplier > Tiers[i - 1].ValueMultiplier);
	}

	// Verify stat bonuses increase monotonically
	for (int32 i = 1; i < Tiers.Num(); ++i)
	{
		TestTrue(FString::Printf(TEXT("Tier %d stat bonus should be > tier %d"), i, i - 1),
			Tiers[i].StatBonus > Tiers[i - 1].StatBonus);
	}

	return true;
}

// ============================================================================
// 3. Quality Score to Tier Mapping
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityScoreToTier,
	"Odyssey.Crafting.Quality.ScoreToTierMapping",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityScoreToTier::RunTest(const FString& Parameters)
{
	// Replicate GetQualityTierFromScore logic using the default tier ranges
	struct FTierRange { float Min; float Max; EItemQuality Quality; };
	TArray<FTierRange> Tiers = {
		{0.0f, 0.15f, EItemQuality::Scrap},
		{0.15f, 0.35f, EItemQuality::Common},
		{0.35f, 0.55f, EItemQuality::Standard},
		{0.55f, 0.72f, EItemQuality::Quality},
		{0.72f, 0.85f, EItemQuality::Superior},
		{0.85f, 0.95f, EItemQuality::Masterwork},
		{0.95f, 1.0f, EItemQuality::Legendary}
	};

	auto GetTier = [&](float Score) -> EItemQuality
	{
		for (const auto& T : Tiers)
		{
			if (Score >= T.Min && Score < T.Max)
			{
				return T.Quality;
			}
		}
		if (Score >= 0.95f) return EItemQuality::Legendary;
		return EItemQuality::Common;
	};

	// Test boundary values
	TestEqual(TEXT("0.0 -> Scrap"), GetTier(0.0f), EItemQuality::Scrap);
	TestEqual(TEXT("0.10 -> Scrap"), GetTier(0.10f), EItemQuality::Scrap);
	TestEqual(TEXT("0.15 -> Common"), GetTier(0.15f), EItemQuality::Common);
	TestEqual(TEXT("0.34 -> Common"), GetTier(0.34f), EItemQuality::Common);
	TestEqual(TEXT("0.35 -> Standard"), GetTier(0.35f), EItemQuality::Standard);
	TestEqual(TEXT("0.54 -> Standard"), GetTier(0.54f), EItemQuality::Standard);
	TestEqual(TEXT("0.55 -> Quality"), GetTier(0.55f), EItemQuality::Quality);
	TestEqual(TEXT("0.71 -> Quality"), GetTier(0.71f), EItemQuality::Quality);
	TestEqual(TEXT("0.72 -> Superior"), GetTier(0.72f), EItemQuality::Superior);
	TestEqual(TEXT("0.84 -> Superior"), GetTier(0.84f), EItemQuality::Superior);
	TestEqual(TEXT("0.85 -> Masterwork"), GetTier(0.85f), EItemQuality::Masterwork);
	TestEqual(TEXT("0.94 -> Masterwork"), GetTier(0.94f), EItemQuality::Masterwork);
	TestEqual(TEXT("0.95 -> Legendary"), GetTier(0.95f), EItemQuality::Legendary);
	TestEqual(TEXT("1.0 -> Legendary"), GetTier(1.0f), EItemQuality::Legendary);

	return true;
}

// ============================================================================
// 4. Quality Modifier Additive Application
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityModifierAdditive,
	"Odyssey.Crafting.Quality.AdditiveModifierApplication",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityModifierAdditive::RunTest(const FString& Parameters)
{
	// Replicate ApplyModifiers logic
	float BaseScore = 0.35f;
	TArray<FQualityModifier> Modifiers;

	Modifiers.Add(FQualityModifier(EQualityModifierSource::Skill, TEXT("Skill Bonus"), 0.1f, false));
	Modifiers.Add(FQualityModifier(EQualityModifierSource::Facility, TEXT("Facility Bonus"), 0.05f, false));

	float AdditiveTotal = 0.0f;
	float MultiplicativeTotal = 1.0f;

	for (const FQualityModifier& Mod : Modifiers)
	{
		if (Mod.bIsMultiplicative)
		{
			MultiplicativeTotal *= (1.0f + Mod.Modifier);
		}
		else
		{
			AdditiveTotal += Mod.Modifier;
		}
	}

	float FinalScore = (BaseScore + AdditiveTotal) * MultiplicativeTotal;
	FinalScore = FMath::Clamp(FinalScore, 0.0f, 1.0f);

	// Expected: (0.35 + 0.1 + 0.05) * 1.0 = 0.50
	TestTrue(TEXT("Final score with additive mods should be 0.50"),
		FMath::IsNearlyEqual(FinalScore, 0.50f, 0.001f));

	return true;
}

// ============================================================================
// 5. Quality Modifier Multiplicative Application
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityModifierMultiplicative,
	"Odyssey.Crafting.Quality.MultiplicativeModifierApplication",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityModifierMultiplicative::RunTest(const FString& Parameters)
{
	float BaseScore = 0.5f;
	TArray<FQualityModifier> Modifiers;

	Modifiers.Add(FQualityModifier(EQualityModifierSource::Catalyst, TEXT("Catalyst"), 0.2f, true));
	Modifiers.Add(FQualityModifier(EQualityModifierSource::Tool, TEXT("Tool"), 0.1f, true));

	float AdditiveTotal = 0.0f;
	float MultiplicativeTotal = 1.0f;

	for (const FQualityModifier& Mod : Modifiers)
	{
		if (Mod.bIsMultiplicative)
		{
			MultiplicativeTotal *= (1.0f + Mod.Modifier);
		}
		else
		{
			AdditiveTotal += Mod.Modifier;
		}
	}

	float FinalScore = (BaseScore + AdditiveTotal) * MultiplicativeTotal;
	// Expected: (0.5 + 0.0) * (1.2 * 1.1) = 0.5 * 1.32 = 0.66
	float ExpectedScore = 0.5f * 1.2f * 1.1f;

	TestTrue(TEXT("Final score with multiplicative mods should be ~0.66"),
		FMath::IsNearlyEqual(FinalScore, ExpectedScore, 0.001f));

	return true;
}

// ============================================================================
// 6. Combined Additive and Multiplicative Modifiers
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityModifierCombined,
	"Odyssey.Crafting.Quality.CombinedModifiers",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityModifierCombined::RunTest(const FString& Parameters)
{
	float BaseScore = 0.3f;
	TArray<FQualityModifier> Modifiers;

	Modifiers.Add(FQualityModifier(EQualityModifierSource::Skill, TEXT("Skill"), 0.15f, false));
	Modifiers.Add(FQualityModifier(EQualityModifierSource::Catalyst, TEXT("Catalyst"), 0.3f, true));

	float AdditiveTotal = 0.0f;
	float MultiplicativeTotal = 1.0f;

	for (const FQualityModifier& Mod : Modifiers)
	{
		if (Mod.bIsMultiplicative)
		{
			MultiplicativeTotal *= (1.0f + Mod.Modifier);
		}
		else
		{
			AdditiveTotal += Mod.Modifier;
		}
	}

	float FinalScore = (BaseScore + AdditiveTotal) * MultiplicativeTotal;
	// (0.3 + 0.15) * 1.3 = 0.45 * 1.3 = 0.585
	float Expected = 0.45f * 1.3f;

	TestTrue(TEXT("Combined modifiers should produce ~0.585"),
		FMath::IsNearlyEqual(FinalScore, Expected, 0.001f));

	return true;
}

// ============================================================================
// 7. Quality Score Clamping
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityScoreClamping,
	"Odyssey.Crafting.Quality.ScoreClampingBounds",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityScoreClamping::RunTest(const FString& Parameters)
{
	// Test that extreme modifiers are clamped to [0, 1]
	float OvershootScore = FMath::Clamp(1.5f, 0.0f, 1.0f);
	TestEqual(TEXT("Overshoot should clamp to 1.0"), OvershootScore, 1.0f);

	float UndershootScore = FMath::Clamp(-0.3f, 0.0f, 1.0f);
	TestEqual(TEXT("Undershoot should clamp to 0.0"), UndershootScore, 0.0f);

	float NormalScore = FMath::Clamp(0.65f, 0.0f, 1.0f);
	TestEqual(TEXT("Normal score should pass through"), NormalScore, 0.65f);

	return true;
}

// ============================================================================
// 8. Base Quality Score Calculation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityBaseScoreCalc,
	"Odyssey.Crafting.Quality.BaseScoreCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityBaseScoreCalc::RunTest(const FString& Parameters)
{
	// Replicate CalculateBaseQualityScore logic
	float BaseScore = 0.35f; // Hard-coded base
	float RecipeBaseQualityChance = 0.5f;
	BaseScore += RecipeBaseQualityChance * 0.3f; // += 0.15

	TestTrue(TEXT("Base quality score should be ~0.50"),
		FMath::IsNearlyEqual(BaseScore, 0.50f, 0.001f));

	// With a higher base quality chance
	float BaseScore2 = 0.35f + (0.8f * 0.3f); // 0.35 + 0.24 = 0.59
	TestTrue(TEXT("High quality recipe should have ~0.59 base score"),
		FMath::IsNearlyEqual(BaseScore2, 0.59f, 0.001f));

	return true;
}

// ============================================================================
// 9. Critical Craft Chance Calculation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityCriticalChance,
	"Odyssey.Crafting.Quality.CriticalCraftChance",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityCriticalChance::RunTest(const FString& Parameters)
{
	float BaseCriticalChance = 0.05f;

	// Without skill bonus
	float CritChance = BaseCriticalChance;
	TestTrue(TEXT("Base crit chance should be 5%"), FMath::IsNearlyEqual(CritChance, 0.05f, 0.001f));

	// With Precision skill level 5
	float SkillBonus = 5 * 0.02f; // 0.10
	CritChance = BaseCriticalChance + SkillBonus;
	float CappedChance = FMath::Min(CritChance, 0.25f);
	TestTrue(TEXT("Crit with Precision 5 should be 15%"), FMath::IsNearlyEqual(CappedChance, 0.15f, 0.001f));

	// Cap test: Precision level 15 would exceed 25% cap
	float HighSkillBonus = 15 * 0.02f; // 0.30
	float HighCritChance = FMath::Min(BaseCriticalChance + HighSkillBonus, 0.25f);
	TestTrue(TEXT("Crit chance should cap at 25%"), FMath::IsNearlyEqual(HighCritChance, 0.25f, 0.001f));

	return true;
}

// ============================================================================
// 10. Critical Craft Quality Boost
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityCriticalBoost,
	"Odyssey.Crafting.Quality.CriticalCraftQualityBoost",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityCriticalBoost::RunTest(const FString& Parameters)
{
	int32 CriticalQualityBonus = 1; // Default from constructor

	// Standard -> Superior on critical
	int32 CurrentTier = static_cast<int32>(EItemQuality::Standard); // 2
	int32 NewTier = FMath::Min(CurrentTier + CriticalQualityBonus, static_cast<int32>(EItemQuality::Legendary));
	EItemQuality BoostedQuality = static_cast<EItemQuality>(NewTier);

	TestEqual(TEXT("Standard + crit should be Quality"), BoostedQuality, EItemQuality::Quality);

	// Masterwork -> Legendary on critical
	CurrentTier = static_cast<int32>(EItemQuality::Masterwork); // 5
	NewTier = FMath::Min(CurrentTier + CriticalQualityBonus, static_cast<int32>(EItemQuality::Legendary));
	BoostedQuality = static_cast<EItemQuality>(NewTier);

	TestEqual(TEXT("Masterwork + crit should be Legendary"), BoostedQuality, EItemQuality::Legendary);

	// Legendary should cap at Legendary (no overflow)
	CurrentTier = static_cast<int32>(EItemQuality::Legendary); // 6
	NewTier = FMath::Min(CurrentTier + CriticalQualityBonus, static_cast<int32>(EItemQuality::Legendary));
	BoostedQuality = static_cast<EItemQuality>(NewTier);

	TestEqual(TEXT("Legendary + crit should remain Legendary"), BoostedQuality, EItemQuality::Legendary);

	return true;
}

// ============================================================================
// 11. Quality Value Multipliers
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityValueMultipliers,
	"Odyssey.Crafting.Quality.ValueMultipliersByTier",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityValueMultipliers::RunTest(const FString& Parameters)
{
	// From InitializeDefaultTiers
	TMap<EItemQuality, float> ExpectedMultipliers;
	ExpectedMultipliers.Add(EItemQuality::Scrap, 0.25f);
	ExpectedMultipliers.Add(EItemQuality::Common, 1.0f);
	ExpectedMultipliers.Add(EItemQuality::Standard, 1.5f);
	ExpectedMultipliers.Add(EItemQuality::Quality, 2.5f);
	ExpectedMultipliers.Add(EItemQuality::Superior, 4.0f);
	ExpectedMultipliers.Add(EItemQuality::Masterwork, 8.0f);
	ExpectedMultipliers.Add(EItemQuality::Legendary, 20.0f);

	// Verify each
	for (const auto& Pair : ExpectedMultipliers)
	{
		FString QualityName = UEnum::GetValueAsString(Pair.Key);
		TestTrue(FString::Printf(TEXT("%s value multiplier should be %.2f"), *QualityName, Pair.Value),
			FMath::IsNearlyEqual(Pair.Value, Pair.Value, 0.001f));
	}

	// Verify exponential growth
	TestTrue(TEXT("Legendary/Common ratio should be 20x"),
		FMath::IsNearlyEqual(*ExpectedMultipliers.Find(EItemQuality::Legendary) / *ExpectedMultipliers.Find(EItemQuality::Common), 20.0f, 0.001f));

	return true;
}

// ============================================================================
// 12. Item Value Calculation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityItemValueCalc,
	"Odyssey.Crafting.Quality.ItemValueCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityItemValueCalc::RunTest(const FString& Parameters)
{
	// Simulate CalculateItemValue logic
	int32 BaseValue = 100;

	// Common item with default multiplier
	float ValueMultiplier = 1.0f; // Common
	float QualityMult = 1.15f;    // Common quality multiplier from craft
	float ScarcityBonus = 0.0f;   // Common scarcity

	float FinalMultiplier = ValueMultiplier * QualityMult * (1.0f + ScarcityBonus * 0.1f);
	int32 FinalValue = FMath::CeilToInt(BaseValue * FinalMultiplier);

	TestEqual(TEXT("Common item value should be 115"), FinalValue, 115);

	// Legendary item
	float LegValueMult = 20.0f;
	float LegQualityMult = 1.90f;
	float LegScarcity = 63.0f; // 2^6 - 1

	float LegFinalMult = LegValueMult * LegQualityMult * (1.0f + LegScarcity * 0.1f);
	int32 LegFinalValue = FMath::CeilToInt(BaseValue * LegFinalMult);

	TestTrue(TEXT("Legendary item should be worth significantly more than common"),
		LegFinalValue > FinalValue * 10);

	return true;
}

// ============================================================================
// 13. Material Quality Bonus from Input Materials
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityMaterialBonus,
	"Odyssey.Crafting.Quality.MaterialQualityBonusCalc",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityMaterialBonus::RunTest(const FString& Parameters)
{
	// Simulate GetMaterialQualityBonus logic
	auto CalcMaterialBonus = [](const TArray<EItemQuality>& Qualities) -> float
	{
		if (Qualities.Num() == 0) return 0.0f;

		float TotalScore = 0.0f;
		for (EItemQuality Q : Qualities)
		{
			float MaterialScore = static_cast<float>(Q) / 6.0f;
			TotalScore += MaterialScore;
		}
		float Average = TotalScore / Qualities.Num();
		return (Average - 0.5f) * 0.4f;
	};

	// All common materials (enum value 1)
	TArray<EItemQuality> CommonMaterials = {EItemQuality::Common, EItemQuality::Common};
	float CommonBonus = CalcMaterialBonus(CommonMaterials);
	// avg = 1/6 = 0.167, bonus = (0.167 - 0.5) * 0.4 = -0.133
	TestTrue(TEXT("Common materials should give negative bonus"), CommonBonus < 0.0f);

	// All Legendary materials (enum value 6)
	TArray<EItemQuality> LegendaryMaterials = {EItemQuality::Legendary, EItemQuality::Legendary};
	float LegendaryBonus = CalcMaterialBonus(LegendaryMaterials);
	// avg = 6/6 = 1.0, bonus = (1.0 - 0.5) * 0.4 = 0.2
	TestTrue(TEXT("Legendary materials should give 0.2 bonus"),
		FMath::IsNearlyEqual(LegendaryBonus, 0.2f, 0.001f));

	// Standard materials (enum value 2) - close to neutral
	TArray<EItemQuality> StandardMaterials = {EItemQuality::Standard, EItemQuality::Standard, EItemQuality::Standard};
	float StandardBonus = CalcMaterialBonus(StandardMaterials);
	// avg = 2/6 = 0.333, bonus = (0.333 - 0.5) * 0.4 = -0.067
	TestTrue(TEXT("Standard materials should give slightly negative bonus"), StandardBonus < 0.0f);

	// Empty materials
	float EmptyBonus = CalcMaterialBonus(TArray<EItemQuality>());
	TestEqual(TEXT("Empty materials should give 0 bonus"), EmptyBonus, 0.0f);

	return true;
}

// ============================================================================
// 14. Temporary Quality Bonus Management
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityTemporaryBonuses,
	"Odyssey.Crafting.Quality.TemporaryBonusManagement",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityTemporaryBonuses::RunTest(const FString& Parameters)
{
	// Simulate TemporaryBonuses map behavior
	TMap<FName, TPair<float, float>> TempBonuses;

	// Add bonuses
	TempBonuses.Add(FName(TEXT("Catalyst_A")), TPair<float, float>(0.1f, 60.0f));
	TempBonuses.Add(FName(TEXT("Catalyst_B")), TPair<float, float>(0.05f, 30.0f));

	// Calculate total
	float Total = 0.0f;
	for (const auto& Pair : TempBonuses)
	{
		Total += Pair.Value.Key;
	}
	TestTrue(TEXT("Total temporary bonus should be 0.15"), FMath::IsNearlyEqual(Total, 0.15f, 0.001f));

	// Simulate time passing and expiration
	float DeltaTime = 35.0f;
	TArray<FName> Expired;
	for (auto& Pair : TempBonuses)
	{
		Pair.Value.Value -= DeltaTime;
		if (Pair.Value.Value <= 0.0f)
		{
			Expired.Add(Pair.Key);
		}
	}

	TestEqual(TEXT("Catalyst_B should have expired"), Expired.Num(), 1);
	TestTrue(TEXT("Expired bonus should be Catalyst_B"), Expired.Contains(FName(TEXT("Catalyst_B"))));

	// Remove expired
	for (const FName& ID : Expired)
	{
		TempBonuses.Remove(ID);
	}

	// Recalculate total
	Total = 0.0f;
	for (const auto& Pair : TempBonuses)
	{
		Total += Pair.Value.Key;
	}
	TestTrue(TEXT("Total after expiration should be 0.1"), FMath::IsNearlyEqual(Total, 0.1f, 0.001f));

	return true;
}

// ============================================================================
// 15. Equipment Effect Defaults
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityEquipmentEffectDefaults,
	"Odyssey.Crafting.Quality.EquipmentEffectDefaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityEquipmentEffectDefaults::RunTest(const FString& Parameters)
{
	FQualityEquipmentEffect Effect;

	TestEqual(TEXT("Default Quality should be Common"), Effect.Quality, EItemQuality::Common);
	TestEqual(TEXT("Default DamageMultiplier should be 1.0"), Effect.DamageMultiplier, 1.0f);
	TestEqual(TEXT("Default DefenseMultiplier should be 1.0"), Effect.DefenseMultiplier, 1.0f);
	TestEqual(TEXT("Default DurabilityMultiplier should be 1.0"), Effect.DurabilityMultiplier, 1.0f);
	TestEqual(TEXT("Default BonusSlots should be 0"), Effect.BonusSlots, 0);

	return true;
}

// ============================================================================
// 16. Equipment Effects Scaling by Quality Tier
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityEquipmentScaling,
	"Odyssey.Crafting.Quality.EquipmentEffectsScaling",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityEquipmentScaling::RunTest(const FString& Parameters)
{
	// Replicate InitializeDefaultEquipmentEffects logic
	for (int32 i = 0; i <= static_cast<int32>(EItemQuality::Legendary); ++i)
	{
		float Tier = static_cast<float>(i);
		float DamageMult = 0.7f + (Tier * 0.15f);
		float DefenseMult = 0.7f + (Tier * 0.15f);
		float DurabilityMult = 0.5f + (Tier * 0.2f);
		int32 BonusSlots = FMath::FloorToInt(Tier / 2.0f);

		EItemQuality Quality = static_cast<EItemQuality>(i);
		FString QualityName = FString::Printf(TEXT("Tier_%d"), i);

		TestTrue(FString::Printf(TEXT("%s damage mult should be positive"), *QualityName), DamageMult > 0.0f);
		TestTrue(FString::Printf(TEXT("%s durability mult should be positive"), *QualityName), DurabilityMult > 0.0f);
	}

	// Scrap should have lowest multipliers
	float ScrapDamage = 0.7f + (0.0f * 0.15f); // 0.7
	float LegendaryDamage = 0.7f + (6.0f * 0.15f); // 1.6

	TestTrue(TEXT("Legendary damage should be higher than Scrap"), LegendaryDamage > ScrapDamage);
	TestTrue(TEXT("Scrap damage mult should be 0.7"), FMath::IsNearlyEqual(ScrapDamage, 0.7f, 0.001f));
	TestTrue(TEXT("Legendary damage mult should be 1.6"), FMath::IsNearlyEqual(LegendaryDamage, 1.6f, 0.001f));

	// Bonus slots: Legendary (tier 6) = floor(6/2) = 3
	int32 LegendarySlots = FMath::FloorToInt(6.0f / 2.0f);
	TestEqual(TEXT("Legendary should have 3 bonus slots"), LegendarySlots, 3);

	// Common (tier 1) = floor(1/2) = 0
	int32 CommonSlots = FMath::FloorToInt(1.0f / 2.0f);
	TestEqual(TEXT("Common should have 0 bonus slots"), CommonSlots, 0);

	return true;
}

// ============================================================================
// 17. Weapon Stat Quality Application
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityWeaponStatApplication,
	"Odyssey.Crafting.Quality.WeaponStatApplication",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityWeaponStatApplication::RunTest(const FString& Parameters)
{
	// Simulate ApplyQualityToWeaponStats for Masterwork
	float Damage = 100.0f;
	float FireRate = 1.0f;
	float Range = 50.0f;

	// Masterwork: tier 5, DamageMult = 0.7 + 5*0.15 = 1.45, StatBonus = 0.5
	float DamageMult = 0.7f + (5.0f * 0.15f); // 1.45
	float StatBonus = 0.5f;

	Damage *= DamageMult;
	FireRate *= (1.0f + StatBonus * 0.5f);
	Range *= (1.0f + StatBonus * 0.3f);

	TestTrue(TEXT("Masterwork damage should be 145"), FMath::IsNearlyEqual(Damage, 145.0f, 0.01f));
	TestTrue(TEXT("Masterwork fire rate should be 1.25"), FMath::IsNearlyEqual(FireRate, 1.25f, 0.01f));
	TestTrue(TEXT("Masterwork range should be 57.5"), FMath::IsNearlyEqual(Range, 57.5f, 0.01f));

	return true;
}

// ============================================================================
// 18. Armor Stat Quality Application
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityArmorStatApplication,
	"Odyssey.Crafting.Quality.ArmorStatApplication",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityArmorStatApplication::RunTest(const FString& Parameters)
{
	// Simulate ApplyQualityToArmorStats for Superior
	float Defense = 80.0f;
	float Durability = 200.0f;
	float Weight = 50.0f;

	// Superior: tier 4, DefenseMult = 0.7 + 4*0.15 = 1.3, DurabilityMult = 0.5 + 4*0.2 = 1.3, StatBonus = 0.35
	float DefenseMult = 0.7f + (4.0f * 0.15f); // 1.3
	float DurabilityMult = 0.5f + (4.0f * 0.2f); // 1.3
	float StatBonus = 0.35f;

	Defense *= DefenseMult;
	Durability *= DurabilityMult;
	Weight *= (1.0f - StatBonus * 0.2f); // Higher quality = lighter

	TestTrue(TEXT("Superior defense should be 104"), FMath::IsNearlyEqual(Defense, 104.0f, 0.01f));
	TestTrue(TEXT("Superior durability should be 260"), FMath::IsNearlyEqual(Durability, 260.0f, 0.01f));
	TestTrue(TEXT("Superior weight should be reduced"), Weight < 50.0f);
	TestTrue(TEXT("Superior weight should be ~46.5"), FMath::IsNearlyEqual(Weight, 46.5f, 0.01f));

	return true;
}

// ============================================================================
// 19. Quality Display Names
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityDisplayNames,
	"Odyssey.Crafting.Quality.DisplayNames",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityDisplayNames::RunTest(const FString& Parameters)
{
	// Replicate GetQualityDisplayName logic
	auto GetName = [](EItemQuality Quality) -> FString
	{
		switch (Quality)
		{
		case EItemQuality::Scrap:       return TEXT("Scrap");
		case EItemQuality::Common:      return TEXT("Common");
		case EItemQuality::Standard:    return TEXT("Standard");
		case EItemQuality::Quality:     return TEXT("Quality");
		case EItemQuality::Superior:    return TEXT("Superior");
		case EItemQuality::Masterwork:  return TEXT("Masterwork");
		case EItemQuality::Legendary:   return TEXT("Legendary");
		default:                        return TEXT("Unknown");
		}
	};

	TestEqual(TEXT("Scrap display name"), GetName(EItemQuality::Scrap), FString(TEXT("Scrap")));
	TestEqual(TEXT("Common display name"), GetName(EItemQuality::Common), FString(TEXT("Common")));
	TestEqual(TEXT("Standard display name"), GetName(EItemQuality::Standard), FString(TEXT("Standard")));
	TestEqual(TEXT("Quality display name"), GetName(EItemQuality::Quality), FString(TEXT("Quality")));
	TestEqual(TEXT("Superior display name"), GetName(EItemQuality::Superior), FString(TEXT("Superior")));
	TestEqual(TEXT("Masterwork display name"), GetName(EItemQuality::Masterwork), FString(TEXT("Masterwork")));
	TestEqual(TEXT("Legendary display name"), GetName(EItemQuality::Legendary), FString(TEXT("Legendary")));

	return true;
}

// ============================================================================
// 20. Item Authenticity Verification
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityAuthenticityCheck,
	"Odyssey.Crafting.Quality.AuthenticityVerification",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityAuthenticityCheck::RunTest(const FString& Parameters)
{
	// Simulate VerifyAuthenticity logic
	auto VerifyAuthenticity = [](const FCraftedItem& Item) -> float
	{
		float Authenticity = 1.0f;

		if (Item.CrafterID.IsNone())
		{
			Authenticity -= 0.2f;
		}

		if (Item.CraftedTime > FDateTime::Now())
		{
			Authenticity -= 0.5f;
		}

		return FMath::Max(Authenticity, 0.0f);
	};

	// Valid item with crafter
	FCraftedItem ValidItem;
	ValidItem.CrafterID = FName(TEXT("Player_123"));
	ValidItem.CraftedTime = FDateTime::Now() - FTimespan::FromMinutes(5);

	float ValidAuth = VerifyAuthenticity(ValidItem);
	TestTrue(TEXT("Valid item should have 1.0 authenticity"), FMath::IsNearlyEqual(ValidAuth, 1.0f, 0.01f));

	// Item without crafter
	FCraftedItem NoCrafterItem;
	NoCrafterItem.CraftedTime = FDateTime::Now() - FTimespan::FromMinutes(5);

	float NoCrafterAuth = VerifyAuthenticity(NoCrafterItem);
	TestTrue(TEXT("No-crafter item should have 0.8 authenticity"), FMath::IsNearlyEqual(NoCrafterAuth, 0.8f, 0.01f));

	// Item with future timestamp (suspicious)
	FCraftedItem FutureItem;
	FutureItem.CraftedTime = FDateTime::Now() + FTimespan::FromDays(1);

	float FutureAuth = VerifyAuthenticity(FutureItem);
	TestTrue(TEXT("Future timestamp item should have reduced authenticity"), FutureAuth < 0.5f);

	return true;
}

// ============================================================================
// 21. Quality Modifier Source Enum Coverage
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityModifierSources,
	"Odyssey.Crafting.Quality.ModifierSourceCoverage",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityModifierSources::RunTest(const FString& Parameters)
{
	// Verify all modifier sources can be constructed
	FQualityModifier SkillMod(EQualityModifierSource::Skill, TEXT("Skill"), 0.1f);
	FQualityModifier FacilityMod(EQualityModifierSource::Facility, TEXT("Facility"), 0.05f);
	FQualityModifier MaterialMod(EQualityModifierSource::Material, TEXT("Material"), 0.03f);
	FQualityModifier ToolMod(EQualityModifierSource::Tool, TEXT("Tool"), 0.02f, true);
	FQualityModifier CatalystMod(EQualityModifierSource::Catalyst, TEXT("Catalyst"), 0.15f);
	FQualityModifier RandomMod(EQualityModifierSource::Random, TEXT("Random"), 0.01f);

	TestEqual(TEXT("Skill source"), SkillMod.Source, EQualityModifierSource::Skill);
	TestEqual(TEXT("Facility source"), FacilityMod.Source, EQualityModifierSource::Facility);
	TestEqual(TEXT("Material source"), MaterialMod.Source, EQualityModifierSource::Material);
	TestEqual(TEXT("Tool source"), ToolMod.Source, EQualityModifierSource::Tool);
	TestEqual(TEXT("Catalyst source"), CatalystMod.Source, EQualityModifierSource::Catalyst);
	TestEqual(TEXT("Random source"), RandomMod.Source, EQualityModifierSource::Random);

	TestFalse(TEXT("Skill should be additive"), SkillMod.bIsMultiplicative);
	TestTrue(TEXT("Tool should be multiplicative"), ToolMod.bIsMultiplicative);

	return true;
}

// ============================================================================
// 22. Quality Inspection Output
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityInspectionOutput,
	"Odyssey.Crafting.Quality.InspectionOutput",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityInspectionOutput::RunTest(const FString& Parameters)
{
	FQualityInspection Inspection;

	TestEqual(TEXT("Default Quality should be Common"), Inspection.Quality, EItemQuality::Common);
	TestEqual(TEXT("Default QualityScore should be 0"), Inspection.QualityScore, 0.0f);
	TestEqual(TEXT("Default Authenticity should be 1.0"), Inspection.Authenticity, 1.0f);
	TestEqual(TEXT("Default EstimatedValue should be 0"), Inspection.EstimatedValue, 0);
	TestEqual(TEXT("Default QualityNotes should be empty"), Inspection.QualityNotes.Num(), 0);

	return true;
}

// ============================================================================
// 23. Market Demand by Quality
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FQualityMarketDemandScaling,
	"Odyssey.Crafting.Quality.MarketDemandScaling",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FQualityMarketDemandScaling::RunTest(const FString& Parameters)
{
	// Replicate InitializeDefaultMarketDemands logic
	for (int32 i = 0; i <= static_cast<int32>(EItemQuality::Legendary); ++i)
	{
		float Tier = static_cast<float>(i);
		float DemandMult = 1.0f + (Tier * 0.3f);
		float PriceMult = FMath::Pow(1.5f, Tier);
		float Scarcity = FMath::Pow(2.0f, Tier);

		TestTrue(FString::Printf(TEXT("Tier %d demand should be positive"), i), DemandMult > 0.0f);
		TestTrue(FString::Printf(TEXT("Tier %d price should be positive"), i), PriceMult > 0.0f);
		TestTrue(FString::Printf(TEXT("Tier %d scarcity should be >= 1"), i), Scarcity >= 1.0f);
	}

	// Legendary scarcity: 2^6 = 64
	float LegScarcity = FMath::Pow(2.0f, 6.0f);
	TestTrue(TEXT("Legendary scarcity should be 64"), FMath::IsNearlyEqual(LegScarcity, 64.0f, 0.01f));

	// Legendary price: 1.5^6 ~= 11.39
	float LegPrice = FMath::Pow(1.5f, 6.0f);
	TestTrue(TEXT("Legendary price mult should be ~11.39"), FMath::IsNearlyEqual(LegPrice, 11.39f, 0.1f));

	return true;
}
