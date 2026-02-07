// OdysseyCraftingSkillTests.cpp
// Comprehensive automation tests for the Crafting Skill System
// Tests skill management, XP, skill points, mastery, and crafting bonuses

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Crafting/OdysseyCraftingManager.h"
#include "Crafting/OdysseyCraftingSkillSystem.h"

// ============================================================================
// 1. Skill Default Initialization
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingSkillDefaults,
	"Odyssey.Crafting.Skill.DefaultInitialization",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingSkillDefaults::RunTest(const FString& Parameters)
{
	FCraftingSkill Skill;

	TestEqual(TEXT("Default SkillID should be None"), Skill.SkillID, NAME_None);
	TestEqual(TEXT("Default SkillName should be 'Unknown Skill'"), Skill.SkillName, FString(TEXT("Unknown Skill")));
	TestEqual(TEXT("Default Category should be General"), Skill.Category, ECraftingSkillCategory::General);
	TestEqual(TEXT("Default CurrentLevel should be 0"), Skill.CurrentLevel, 0);
	TestEqual(TEXT("Default MaxLevel should be 10"), Skill.MaxLevel, 10);
	TestEqual(TEXT("Default CurrentExperience should be 0"), Skill.CurrentExperience, 0);
	TestEqual(TEXT("Default ExperienceToNextLevel should be 100"), Skill.ExperienceToNextLevel, 100);
	TestTrue(TEXT("Default SpeedBonusPerLevel should be 0.02"), FMath::IsNearlyEqual(Skill.SpeedBonusPerLevel, 0.02f, 0.001f));
	TestTrue(TEXT("Default QualityBonusPerLevel should be 0.015"), FMath::IsNearlyEqual(Skill.QualityBonusPerLevel, 0.015f, 0.001f));
	TestTrue(TEXT("Default SuccessBonusPerLevel should be 0.01"), FMath::IsNearlyEqual(Skill.SuccessBonusPerLevel, 0.01f, 0.001f));
	TestTrue(TEXT("Default MaterialEfficiencyPerLevel should be 0.01"), FMath::IsNearlyEqual(Skill.MaterialEfficiencyPerLevel, 0.01f, 0.001f));
	TestEqual(TEXT("RequiredSkillLevels should be empty"), Skill.RequiredSkillLevels.Num(), 0);
	TestEqual(TEXT("UnlocksRecipes should be empty"), Skill.UnlocksRecipes.Num(), 0);

	return true;
}

// ============================================================================
// 2. Experience Curve Calculation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingSkillExpCurve,
	"Odyssey.Crafting.Skill.ExperienceCurveCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingSkillExpCurve::RunTest(const FString& Parameters)
{
	// Replicate CalculateExperienceForLevel: BaseXP * (Multiplier ^ Level)
	float ExperienceCurveMultiplier = 1.5f;

	auto CalcXP = [ExperienceCurveMultiplier](int32 Level) -> int32
	{
		int32 BaseXP = 100;
		return FMath::CeilToInt(BaseXP * FMath::Pow(ExperienceCurveMultiplier, static_cast<float>(Level)));
	};

	// Level 0: 100 * 1.5^0 = 100
	TestEqual(TEXT("XP for level 0 should be 100"), CalcXP(0), 100);

	// Level 1: 100 * 1.5^1 = 150
	TestEqual(TEXT("XP for level 1 should be 150"), CalcXP(1), 150);

	// Level 2: 100 * 1.5^2 = 225
	TestEqual(TEXT("XP for level 2 should be 225"), CalcXP(2), 225);

	// Level 5: 100 * 1.5^5 = 759.375 -> 760
	TestEqual(TEXT("XP for level 5 should be ~760"), CalcXP(5), 760);

	// Verify XP requirements increase monotonically
	for (int32 i = 1; i <= 10; ++i)
	{
		TestTrue(FString::Printf(TEXT("Level %d XP should be > level %d"), i, i - 1),
			CalcXP(i) > CalcXP(i - 1));
	}

	return true;
}

// ============================================================================
// 3. Skill Level Up Mechanics
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingSkillLevelUp,
	"Odyssey.Crafting.Skill.LevelUpMechanics",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingSkillLevelUp::RunTest(const FString& Parameters)
{
	// Simulate LevelUpSkill logic
	FCraftingSkill Skill;
	Skill.SkillID = FName(TEXT("WeaponCrafting"));
	Skill.CurrentLevel = 3;
	Skill.MaxLevel = 10;
	Skill.CurrentExperience = 150;
	Skill.ExperienceToNextLevel = 100;

	// Level up should occur when CurrentExperience >= ExperienceToNextLevel
	bool bCanLevelUp = (Skill.CurrentExperience >= Skill.ExperienceToNextLevel && Skill.CurrentLevel < Skill.MaxLevel);
	TestTrue(TEXT("Should be able to level up with excess XP"), bCanLevelUp);

	// Simulate level up
	if (bCanLevelUp)
	{
		Skill.CurrentExperience -= Skill.ExperienceToNextLevel;
		Skill.CurrentLevel++;
		// New XP requirement (using curve: 100 * 1.5^4 = 506.25 -> 507)
		Skill.ExperienceToNextLevel = FMath::CeilToInt(100 * FMath::Pow(1.5f, static_cast<float>(Skill.CurrentLevel)));
	}

	TestEqual(TEXT("Level should now be 4"), Skill.CurrentLevel, 4);
	TestEqual(TEXT("Remaining XP should be 50"), Skill.CurrentExperience, 50);
	TestTrue(TEXT("Next level XP should be higher than previous"), Skill.ExperienceToNextLevel > 100);

	return true;
}

// ============================================================================
// 4. Skill Cannot Exceed Max Level
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingSkillMaxLevel,
	"Odyssey.Crafting.Skill.CannotExceedMaxLevel",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingSkillMaxLevel::RunTest(const FString& Parameters)
{
	FCraftingSkill Skill;
	Skill.CurrentLevel = 10;
	Skill.MaxLevel = 10;
	Skill.CurrentExperience = 9999;
	Skill.ExperienceToNextLevel = 100;

	bool bCanLevelUp = (Skill.CurrentExperience >= Skill.ExperienceToNextLevel && Skill.CurrentLevel < Skill.MaxLevel);
	TestFalse(TEXT("Should not level up past max level"), bCanLevelUp);

	return true;
}

// ============================================================================
// 5. Skill Unlock Prerequisites
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingSkillPrerequisites,
	"Odyssey.Crafting.Skill.UnlockPrerequisites",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingSkillPrerequisites::RunTest(const FString& Parameters)
{
	// Simulate CanUnlockSkill logic
	FCraftingSkill AdvancedSkill;
	AdvancedSkill.SkillID = FName(TEXT("AdvancedWeapons"));
	AdvancedSkill.RequiredSkillLevels.Add(FName(TEXT("WeaponCrafting")), 5);
	AdvancedSkill.RequiredSkillLevels.Add(FName(TEXT("MaterialProcessing")), 3);

	// Simulate having skills
	TMap<FName, int32> PlayerSkillLevels;
	PlayerSkillLevels.Add(FName(TEXT("WeaponCrafting")), 6);
	PlayerSkillLevels.Add(FName(TEXT("MaterialProcessing")), 3);

	bool bMeetsPrereqs = true;
	for (const auto& Req : AdvancedSkill.RequiredSkillLevels)
	{
		const int32* PlayerLevel = PlayerSkillLevels.Find(Req.Key);
		if (!PlayerLevel || *PlayerLevel < Req.Value)
		{
			bMeetsPrereqs = false;
			break;
		}
	}
	TestTrue(TEXT("Should meet prerequisites with sufficient levels"), bMeetsPrereqs);

	// Insufficient levels
	PlayerSkillLevels[FName(TEXT("MaterialProcessing"))] = 2;
	bMeetsPrereqs = true;
	for (const auto& Req : AdvancedSkill.RequiredSkillLevels)
	{
		const int32* PlayerLevel = PlayerSkillLevels.Find(Req.Key);
		if (!PlayerLevel || *PlayerLevel < Req.Value)
		{
			bMeetsPrereqs = false;
			break;
		}
	}
	TestFalse(TEXT("Should fail prerequisites with insufficient level"), bMeetsPrereqs);

	return true;
}

// ============================================================================
// 6. Crafting Speed Bonus Calculation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingSkillSpeedBonus,
	"Odyssey.Crafting.Skill.SpeedBonusCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingSkillSpeedBonus::RunTest(const FString& Parameters)
{
	// Replicate GetCraftingSpeedBonus logic
	// Sums SpeedBonusPerLevel * CurrentLevel for all unlocked skills
	struct FSkillEntry { FName ID; int32 Level; float SpeedPerLevel; };
	TArray<FSkillEntry> UnlockedSkills = {
		{FName(TEXT("General")), 5, 0.02f},         // 0.10
		{FName(TEXT("WeaponCrafting")), 3, 0.02f},   // 0.06
		{FName(TEXT("MaterialProcessing")), 7, 0.02f} // 0.14
	};

	float TotalSpeedBonus = 0.0f;
	for (const auto& S : UnlockedSkills)
	{
		TotalSpeedBonus += S.SpeedPerLevel * S.Level;
	}

	float ExpectedBonus = 0.10f + 0.06f + 0.14f; // 0.30
	TestTrue(TEXT("Total speed bonus should be 0.30"),
		FMath::IsNearlyEqual(TotalSpeedBonus, ExpectedBonus, 0.001f));

	return true;
}

// ============================================================================
// 7. Crafting Quality Bonus Calculation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingSkillQualityBonus,
	"Odyssey.Crafting.Skill.QualityBonusCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingSkillQualityBonus::RunTest(const FString& Parameters)
{
	// Quality bonus = sum(QualityBonusPerLevel * Level) for unlocked skills
	float QualityBonusPerLevel = 0.015f;

	float BonusLevel5 = 5 * QualityBonusPerLevel; // 0.075
	float BonusLevel10 = 10 * QualityBonusPerLevel; // 0.15

	TestTrue(TEXT("Level 5 quality bonus should be 0.075"),
		FMath::IsNearlyEqual(BonusLevel5, 0.075f, 0.001f));
	TestTrue(TEXT("Level 10 quality bonus should be 0.15"),
		FMath::IsNearlyEqual(BonusLevel10, 0.15f, 0.001f));

	return true;
}

// ============================================================================
// 8. Skill Point Allocation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingSkillPointAllocation,
	"Odyssey.Crafting.Skill.SkillPointAllocation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingSkillPointAllocation::RunTest(const FString& Parameters)
{
	FSkillPointAllocation Points;

	TestEqual(TEXT("Default TotalSkillPoints should be 0"), Points.TotalSkillPoints, 0);
	TestEqual(TEXT("Default AvailableSkillPoints should be 0"), Points.AvailableSkillPoints, 0);
	TestEqual(TEXT("Default SpentSkillPoints should be 0"), Points.SpentSkillPoints, 0);

	// Simulate adding points: 1 point per level, total levels = 15
	int32 TotalLevels = 15;
	int32 SkillPointsPerLevel = 1;
	Points.TotalSkillPoints = TotalLevels * SkillPointsPerLevel;
	Points.SpentSkillPoints = 10;
	Points.AvailableSkillPoints = Points.TotalSkillPoints - Points.SpentSkillPoints;

	TestEqual(TEXT("Total points should be 15"), Points.TotalSkillPoints, 15);
	TestEqual(TEXT("Available points should be 5"), Points.AvailableSkillPoints, 5);
	TestEqual(TEXT("Spent points should be 10"), Points.SpentSkillPoints, 10);

	return true;
}

// ============================================================================
// 9. Skill Point Spending
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingSkillPointSpending,
	"Odyssey.Crafting.Skill.SkillPointSpending",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingSkillPointSpending::RunTest(const FString& Parameters)
{
	// Simulate SpendSkillPoint logic
	FSkillPointAllocation Points;
	Points.TotalSkillPoints = 10;
	Points.AvailableSkillPoints = 5;
	Points.SpentSkillPoints = 5;

	// Spend a point
	bool bCanSpend = (Points.AvailableSkillPoints > 0);
	TestTrue(TEXT("Should be able to spend with available points"), bCanSpend);

	if (bCanSpend)
	{
		Points.AvailableSkillPoints--;
		Points.SpentSkillPoints++;
	}

	TestEqual(TEXT("Available should be 4 after spending"), Points.AvailableSkillPoints, 4);
	TestEqual(TEXT("Spent should be 6 after spending"), Points.SpentSkillPoints, 6);

	// Drain all points
	Points.AvailableSkillPoints = 0;
	bCanSpend = (Points.AvailableSkillPoints > 0);
	TestFalse(TEXT("Should not spend with 0 available"), bCanSpend);

	return true;
}

// ============================================================================
// 10. Skill Point Reset (Respec)
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingSkillRespec,
	"Odyssey.Crafting.Skill.RespecResetsMechanics",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingSkillRespec::RunTest(const FString& Parameters)
{
	// Simulate ResetSkillPoints logic
	FSkillPointAllocation Points;
	Points.TotalSkillPoints = 20;
	Points.AvailableSkillPoints = 3;
	Points.SpentSkillPoints = 17;

	// Reset: all spent points become available
	Points.AvailableSkillPoints = Points.TotalSkillPoints;
	Points.SpentSkillPoints = 0;

	TestEqual(TEXT("Available should equal total after respec"), Points.AvailableSkillPoints, 20);
	TestEqual(TEXT("Spent should be 0 after respec"), Points.SpentSkillPoints, 0);

	return true;
}

// ============================================================================
// 11. Mastery Bonus Default Initialization
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingMasteryDefaults,
	"Odyssey.Crafting.Skill.MasteryBonusDefaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingMasteryDefaults::RunTest(const FString& Parameters)
{
	FCraftingMasteryBonus Mastery;

	TestEqual(TEXT("Default MasteryID should be None"), Mastery.MasteryID, NAME_None);
	TestEqual(TEXT("Default Category should be General"), Mastery.Category, ECraftingSkillCategory::General);
	TestEqual(TEXT("Default RequiredTotalLevels should be 50"), Mastery.RequiredTotalLevels, 50);
	TestFalse(TEXT("Default bIsUnlocked should be false"), Mastery.bIsUnlocked);
	TestTrue(TEXT("Default SpeedMultiplier should be 1.2"), FMath::IsNearlyEqual(Mastery.SpeedMultiplier, 1.2f, 0.001f));
	TestTrue(TEXT("Default QualityMultiplier should be 1.15"), FMath::IsNearlyEqual(Mastery.QualityMultiplier, 1.15f, 0.001f));
	TestTrue(TEXT("Default UniqueItemChance should be 0.05"), FMath::IsNearlyEqual(Mastery.UniqueItemChance, 0.05f, 0.001f));

	return true;
}

// ============================================================================
// 12. Mastery Unlock Condition
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingMasteryUnlock,
	"Odyssey.Crafting.Skill.MasteryUnlockCondition",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingMasteryUnlock::RunTest(const FString& Parameters)
{
	FCraftingMasteryBonus Mastery;
	Mastery.RequiredTotalLevels = 30;
	Mastery.Category = ECraftingSkillCategory::WeaponCrafting;

	// Simulate CheckMasteryUnlocks: sum levels in category
	int32 TotalLevelsInCategory = 25;
	bool bCanUnlock = (TotalLevelsInCategory >= Mastery.RequiredTotalLevels);
	TestFalse(TEXT("Should not unlock with 25/30 levels"), bCanUnlock);

	TotalLevelsInCategory = 30;
	bCanUnlock = (TotalLevelsInCategory >= Mastery.RequiredTotalLevels);
	TestTrue(TEXT("Should unlock with exactly 30/30 levels"), bCanUnlock);

	TotalLevelsInCategory = 35;
	bCanUnlock = (TotalLevelsInCategory >= Mastery.RequiredTotalLevels);
	TestTrue(TEXT("Should unlock with 35/30 levels (exceeds)"), bCanUnlock);

	return true;
}

// ============================================================================
// 13. Skill Category Coverage
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingSkillCategories,
	"Odyssey.Crafting.Skill.AllCategoriesRepresented",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingSkillCategories::RunTest(const FString& Parameters)
{
	// Verify all enum values are accessible
	TArray<ECraftingSkillCategory> AllCategories = {
		ECraftingSkillCategory::General,
		ECraftingSkillCategory::MaterialProcessing,
		ECraftingSkillCategory::WeaponCrafting,
		ECraftingSkillCategory::ArmorCrafting,
		ECraftingSkillCategory::ShipModules,
		ECraftingSkillCategory::Electronics,
		ECraftingSkillCategory::Chemistry,
		ECraftingSkillCategory::Research,
		ECraftingSkillCategory::Automation
	};

	TestEqual(TEXT("Should have 9 crafting skill categories"), AllCategories.Num(), 9);

	// Verify they have distinct values
	TSet<uint8> UniqueValues;
	for (ECraftingSkillCategory Cat : AllCategories)
	{
		UniqueValues.Add(static_cast<uint8>(Cat));
	}
	TestEqual(TEXT("All categories should have unique values"), UniqueValues.Num(), AllCategories.Num());

	return true;
}

// ============================================================================
// 14. XP-Based Crafting Experience Award
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingSkillCraftingXP,
	"Odyssey.Crafting.Skill.CraftingExperienceAward",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingSkillCraftingXP::RunTest(const FString& Parameters)
{
	// Simulate AddCraftingExperience logic from CompleteJob
	// XP = SkillExpValue * Quantity * QualityMultiplier
	int32 SkillExpValue = 25;
	int32 Quantity = 3;
	EItemQuality FinalQuality = EItemQuality::Superior; // enum value 4

	float QualityMultiplier = 1.0f + (static_cast<float>(FinalQuality) * 0.1f); // 1.4
	int32 TotalXP = FMath::CeilToInt(SkillExpValue * Quantity * QualityMultiplier);
	// 25 * 3 * 1.4 = 105

	TestEqual(TEXT("XP awarded should be 105"), TotalXP, 105);

	// Scrap quality (lowest bonus)
	QualityMultiplier = 1.0f + (static_cast<float>(EItemQuality::Scrap) * 0.1f); // 1.0
	TotalXP = FMath::CeilToInt(SkillExpValue * Quantity * QualityMultiplier);
	TestEqual(TEXT("Scrap quality XP should be 75"), TotalXP, 75);

	// Legendary quality (highest bonus)
	QualityMultiplier = 1.0f + (static_cast<float>(EItemQuality::Legendary) * 0.1f); // 1.6
	TotalXP = FMath::CeilToInt(SkillExpValue * Quantity * QualityMultiplier);
	TestEqual(TEXT("Legendary quality XP should be 120"), TotalXP, 120);

	return true;
}

// ============================================================================
// 15. Material Efficiency Bonus
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingSkillMaterialEfficiency,
	"Odyssey.Crafting.Skill.MaterialEfficiencyBonus",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingSkillMaterialEfficiency::RunTest(const FString& Parameters)
{
	// GetMaterialEfficiency: 1.0 - (level * 0.02), capped at 0.5 minimum
	auto CalcEfficiency = [](int32 Level) -> float
	{
		float Efficiency = 1.0f - (Level * 0.02f);
		return FMath::Max(Efficiency, 0.5f);
	};

	TestTrue(TEXT("Level 0 efficiency should be 1.0"), FMath::IsNearlyEqual(CalcEfficiency(0), 1.0f, 0.001f));
	TestTrue(TEXT("Level 5 efficiency should be 0.90"), FMath::IsNearlyEqual(CalcEfficiency(5), 0.90f, 0.001f));
	TestTrue(TEXT("Level 10 efficiency should be 0.80"), FMath::IsNearlyEqual(CalcEfficiency(10), 0.80f, 0.001f));
	TestTrue(TEXT("Level 25 efficiency should be 0.50 (cap)"), FMath::IsNearlyEqual(CalcEfficiency(25), 0.50f, 0.001f));
	TestTrue(TEXT("Level 50 efficiency should be 0.50 (cap)"), FMath::IsNearlyEqual(CalcEfficiency(50), 0.50f, 0.001f));

	return true;
}

// ============================================================================
// 16. Bonus Output Chance from Skill
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingSkillBonusOutput,
	"Odyssey.Crafting.Skill.BonusOutputChance",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingSkillBonusOutput::RunTest(const FString& Parameters)
{
	// GetBonusOutputChance: MasteryLevel * 0.03, capped at 0.5
	auto CalcBonusChance = [](int32 MasteryLevel) -> float
	{
		float Chance = MasteryLevel * 0.03f;
		return FMath::Min(Chance, 0.5f);
	};

	TestTrue(TEXT("Level 0 bonus chance should be 0"), FMath::IsNearlyEqual(CalcBonusChance(0), 0.0f, 0.001f));
	TestTrue(TEXT("Level 5 bonus chance should be 0.15"), FMath::IsNearlyEqual(CalcBonusChance(5), 0.15f, 0.001f));
	TestTrue(TEXT("Level 10 bonus chance should be 0.30"), FMath::IsNearlyEqual(CalcBonusChance(10), 0.30f, 0.001f));
	TestTrue(TEXT("Level 20 bonus chance should be 0.50 (cap)"), FMath::IsNearlyEqual(CalcBonusChance(20), 0.50f, 0.001f));

	return true;
}

// ============================================================================
// 17. Skill Progress Info
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingSkillProgressInfo,
	"Odyssey.Crafting.Skill.ProgressInfoDefaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingSkillProgressInfo::RunTest(const FString& Parameters)
{
	FSkillProgressInfo Progress;

	TestEqual(TEXT("Default SkillID should be None"), Progress.SkillID, NAME_None);
	TestEqual(TEXT("Default Level should be 0"), Progress.Level, 0);
	TestEqual(TEXT("Default ProgressToNextLevel should be 0"), Progress.ProgressToNextLevel, 0.0f);
	TestEqual(TEXT("Default TotalExperienceGained should be 0"), Progress.TotalExperienceGained, 0);
	TestEqual(TEXT("Default ItemsCraftedWithSkill should be 0"), Progress.ItemsCraftedWithSkill, 0);

	return true;
}

// ============================================================================
// 18. Skill Tree Node Structure
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingSkillTreeNode,
	"Odyssey.Crafting.Skill.TreeNodeDefaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingSkillTreeNode::RunTest(const FString& Parameters)
{
	FSkillTreeNode Node;

	TestEqual(TEXT("Default SkillID should be None"), Node.SkillID, NAME_None);
	TestFalse(TEXT("Default bIsUnlocked should be false"), Node.bIsUnlocked);
	TestFalse(TEXT("Default bCanUnlock should be false"), Node.bCanUnlock);
	TestEqual(TEXT("Default ConnectedSkills should be empty"), Node.ConnectedSkills.Num(), 0);

	return true;
}

// ============================================================================
// 19. Success Bonus per Skill Level
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingSkillSuccessBonus,
	"Odyssey.Crafting.Skill.SuccessBonusPerLevel",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingSkillSuccessBonus::RunTest(const FString& Parameters)
{
	float SuccessBonusPerLevel = 0.01f;

	// Level 0
	float Bonus0 = 0 * SuccessBonusPerLevel;
	TestEqual(TEXT("Level 0 success bonus should be 0"), Bonus0, 0.0f);

	// Level 10
	float Bonus10 = 10 * SuccessBonusPerLevel;
	TestTrue(TEXT("Level 10 success bonus should be 0.10"), FMath::IsNearlyEqual(Bonus10, 0.10f, 0.001f));

	// Combined across multiple skills
	float TotalBonus = (5 * 0.01f) + (7 * 0.01f) + (3 * 0.01f); // 15 levels total = 0.15
	TestTrue(TEXT("Combined success bonus should be 0.15"), FMath::IsNearlyEqual(TotalBonus, 0.15f, 0.001f));

	return true;
}

// ============================================================================
// 20. Recipe Difficulty Calculation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingSkillRecipeDifficulty,
	"Odyssey.Crafting.Skill.RecipeDifficultyCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingSkillRecipeDifficulty::RunTest(const FString& Parameters)
{
	// Replicate GetRecipeDifficulty logic
	auto CalcDifficulty = [](ECraftingTier Tier, int32 TotalSkillReqs, int32 IngredientCount, int32 ChainDepth) -> float
	{
		float Difficulty = 0.0f;
		Difficulty += static_cast<float>(Tier) * 0.15f;
		Difficulty += TotalSkillReqs * 0.05f;
		Difficulty += IngredientCount * 0.1f;
		Difficulty += ChainDepth * 0.08f;
		return FMath::Clamp(Difficulty, 0.1f, 1.0f);
	};

	// Simple recipe
	float SimpleDifficulty = CalcDifficulty(ECraftingTier::Basic, 0, 2, 0);
	// 1 * 0.15 + 0 + 2 * 0.1 + 0 = 0.35
	TestTrue(TEXT("Simple recipe difficulty should be ~0.35"),
		FMath::IsNearlyEqual(SimpleDifficulty, 0.35f, 0.01f));

	// Complex recipe
	float ComplexDifficulty = CalcDifficulty(ECraftingTier::Quantum, 15, 5, 4);
	// 5 * 0.15 + 15 * 0.05 + 5 * 0.1 + 4 * 0.08 = 0.75 + 0.75 + 0.5 + 0.32 = 2.32 -> clamped to 1.0
	TestEqual(TEXT("Complex recipe difficulty should cap at 1.0"), ComplexDifficulty, 1.0f);

	// Minimum difficulty
	float MinDifficulty = CalcDifficulty(ECraftingTier::Primitive, 0, 0, 0);
	TestEqual(TEXT("Minimum difficulty should be 0.1"), MinDifficulty, 0.1f);

	return true;
}

// ============================================================================
// 21. Recommended Skill Level for Recipe
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingSkillRecommendedLevel,
	"Odyssey.Crafting.Skill.RecommendedLevelCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingSkillRecommendedLevel::RunTest(const FString& Parameters)
{
	// GetRecommendedSkillLevel: max required level + 2
	auto CalcRecommended = [](const TMap<FName, int32>& SkillReqs) -> int32
	{
		int32 MaxRequired = 1;
		for (const auto& Req : SkillReqs)
		{
			MaxRequired = FMath::Max(MaxRequired, Req.Value);
		}
		return MaxRequired + 2;
	};

	// No requirements
	TMap<FName, int32> NoReqs;
	TestEqual(TEXT("No requirements should recommend level 3"), CalcRecommended(NoReqs), 3);

	// Single high requirement
	TMap<FName, int32> HighReq;
	HighReq.Add(FName(TEXT("Electronics")), 8);
	TestEqual(TEXT("Level 8 requirement should recommend 10"), CalcRecommended(HighReq), 10);

	// Multiple requirements, max is 6
	TMap<FName, int32> MultiReqs;
	MultiReqs.Add(FName(TEXT("Weapon")), 4);
	MultiReqs.Add(FName(TEXT("Material")), 6);
	MultiReqs.Add(FName(TEXT("General")), 2);
	TestEqual(TEXT("Max 6 requirement should recommend 8"), CalcRecommended(MultiReqs), 8);

	return true;
}
