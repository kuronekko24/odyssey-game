// OdysseyRecipeSystemTests.cpp
// Comprehensive automation tests for the Recipe System
// Tests recipe creation, validation, prerequisites, and multi-step chains

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "Crafting/OdysseyCraftingManager.h"
#include "Crafting/OdysseyCraftingRecipeComponent.h"
#include "OdysseyInventoryComponent.h"
#include "OdysseyCraftingComponent.h"

// ============================================================================
// Helper: Create a test recipe with configurable fields
// ============================================================================
namespace OdysseyTestHelpers
{
	static FAdvancedCraftingRecipe MakeTestRecipe(
		FName InID,
		const FString& InName,
		ECraftingTier InTier = ECraftingTier::Basic,
		float InCraftTime = 5.0f)
	{
		FAdvancedCraftingRecipe R;
		R.RecipeID = InID;
		R.RecipeName = InName;
		R.Description = FString::Printf(TEXT("Test recipe: %s"), *InName);
		R.OutputCategory = EItemCategory::Component;
		R.RequiredTier = InTier;
		R.BaseCraftingTime = InCraftTime;
		R.EnergyCost = 10;
		R.BaseQualityChance = 0.5f;
		R.bQualityAffectedBySkill = true;
		R.bQualityAffectedByInputQuality = true;
		R.BaseExperienceReward = 10;
		R.bCanBeAutomated = true;
		R.ChainDepth = 0;
		return R;
	}

	static FCraftingIngredient MakeIngredient(EResourceType Type, int32 Amount)
	{
		return FCraftingIngredient(Type, Amount);
	}

	static FCraftingOutput MakeOutput(EResourceType Type, int32 Amount, float Chance = 1.0f)
	{
		return FCraftingOutput(Type, Amount, Chance);
	}
}

// ============================================================================
// 1. Recipe Struct Default Initialization
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRecipeDefaultInitialization,
	"Odyssey.Crafting.Recipe.DefaultInitialization",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRecipeDefaultInitialization::RunTest(const FString& Parameters)
{
	FAdvancedCraftingRecipe Recipe;

	TestEqual(TEXT("Default RecipeID should be NAME_None"), Recipe.RecipeID, NAME_None);
	TestEqual(TEXT("Default RecipeName should be 'Unknown Recipe'"), Recipe.RecipeName, FString(TEXT("Unknown Recipe")));
	TestEqual(TEXT("Default RequiredTier should be Basic"), Recipe.RequiredTier, ECraftingTier::Basic);
	TestEqual(TEXT("Default BaseCraftingTime should be 5.0"), Recipe.BaseCraftingTime, 5.0f);
	TestEqual(TEXT("Default EnergyCost should be 10"), Recipe.EnergyCost, 10);
	TestEqual(TEXT("Default BaseQualityChance should be 0.5"), Recipe.BaseQualityChance, 0.5f);
	TestTrue(TEXT("Default bQualityAffectedBySkill should be true"), Recipe.bQualityAffectedBySkill);
	TestTrue(TEXT("Default bQualityAffectedByInputQuality should be true"), Recipe.bQualityAffectedByInputQuality);
	TestEqual(TEXT("Default BaseExperienceReward should be 10"), Recipe.BaseExperienceReward, 10);
	TestTrue(TEXT("Default bCanBeAutomated should be true"), Recipe.bCanBeAutomated);
	TestEqual(TEXT("Default ChainDepth should be 0"), Recipe.ChainDepth, 0);
	TestEqual(TEXT("Default OutputCategory should be Component"), Recipe.OutputCategory, EItemCategory::Component);
	TestEqual(TEXT("PrimaryIngredients should be empty"), Recipe.PrimaryIngredients.Num(), 0);
	TestEqual(TEXT("PrimaryOutputs should be empty"), Recipe.PrimaryOutputs.Num(), 0);
	TestEqual(TEXT("PrerequisiteRecipes should be empty"), Recipe.PrerequisiteRecipes.Num(), 0);

	return true;
}

// ============================================================================
// 2. Recipe Creation With Valid Data
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRecipeCreationValidData,
	"Odyssey.Crafting.Recipe.CreationWithValidData",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRecipeCreationValidData::RunTest(const FString& Parameters)
{
	using namespace OdysseyTestHelpers;

	FAdvancedCraftingRecipe Recipe = MakeTestRecipe(
		FName(TEXT("Recipe_SteelPlate")),
		TEXT("Steel Plate"),
		ECraftingTier::Advanced,
		10.0f
	);

	Recipe.PrimaryIngredients.Add(MakeIngredient(EResourceType::RefinedSilicate, 5));
	Recipe.PrimaryIngredients.Add(MakeIngredient(EResourceType::Carbon, 3));
	Recipe.PrimaryOutputs.Add(MakeOutput(EResourceType::CompositeMaterial, 2));

	TestEqual(TEXT("RecipeID should match"), Recipe.RecipeID, FName(TEXT("Recipe_SteelPlate")));
	TestEqual(TEXT("RecipeName should match"), Recipe.RecipeName, FString(TEXT("Steel Plate")));
	TestEqual(TEXT("RequiredTier should be Advanced"), Recipe.RequiredTier, ECraftingTier::Advanced);
	TestEqual(TEXT("BaseCraftingTime should be 10"), Recipe.BaseCraftingTime, 10.0f);
	TestEqual(TEXT("Should have 2 primary ingredients"), Recipe.PrimaryIngredients.Num(), 2);
	TestEqual(TEXT("Should have 1 primary output"), Recipe.PrimaryOutputs.Num(), 1);
	TestEqual(TEXT("First ingredient should be RefinedSilicate"), Recipe.PrimaryIngredients[0].ResourceType, EResourceType::RefinedSilicate);
	TestEqual(TEXT("First ingredient amount should be 5"), Recipe.PrimaryIngredients[0].Amount, 5);
	TestEqual(TEXT("Output should be CompositeMaterial"), Recipe.PrimaryOutputs[0].ResourceType, EResourceType::CompositeMaterial);
	TestEqual(TEXT("Output amount should be 2"), Recipe.PrimaryOutputs[0].Amount, 2);

	return true;
}

// ============================================================================
// 3. Ingredient Struct Initialization
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIngredientDefaultInit,
	"Odyssey.Crafting.Recipe.IngredientDefaultInit",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIngredientDefaultInit::RunTest(const FString& Parameters)
{
	FCraftingIngredient DefaultIngredient;
	TestEqual(TEXT("Default ResourceType should be None"), DefaultIngredient.ResourceType, EResourceType::None);
	TestEqual(TEXT("Default Amount should be 1"), DefaultIngredient.Amount, 1);

	FCraftingIngredient ParamIngredient(EResourceType::Silicate, 42);
	TestEqual(TEXT("Parameterized ResourceType should be Silicate"), ParamIngredient.ResourceType, EResourceType::Silicate);
	TestEqual(TEXT("Parameterized Amount should be 42"), ParamIngredient.Amount, 42);

	return true;
}

// ============================================================================
// 4. Output Struct Initialization
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOutputDefaultInit,
	"Odyssey.Crafting.Recipe.OutputDefaultInit",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FOutputDefaultInit::RunTest(const FString& Parameters)
{
	FCraftingOutput DefaultOutput;
	TestEqual(TEXT("Default ResourceType should be None"), DefaultOutput.ResourceType, EResourceType::None);
	TestEqual(TEXT("Default Amount should be 1"), DefaultOutput.Amount, 1);
	TestEqual(TEXT("Default SuccessChance should be 1.0"), DefaultOutput.SuccessChance, 1.0f);

	FCraftingOutput ParamOutput(EResourceType::CompositeMaterial, 10, 0.75f);
	TestEqual(TEXT("Parameterized ResourceType should be CompositeMaterial"), ParamOutput.ResourceType, EResourceType::CompositeMaterial);
	TestEqual(TEXT("Parameterized Amount should be 10"), ParamOutput.Amount, 10);
	TestEqual(TEXT("Parameterized SuccessChance should be 0.75"), ParamOutput.SuccessChance, 0.75f);

	return true;
}

// ============================================================================
// 5. Recipe With Empty Ingredients Is Valid Struct (But Not Craftable)
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRecipeEmptyIngredients,
	"Odyssey.Crafting.Recipe.EmptyIngredientsStructValid",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRecipeEmptyIngredients::RunTest(const FString& Parameters)
{
	using namespace OdysseyTestHelpers;

	FAdvancedCraftingRecipe Recipe = MakeTestRecipe(FName(TEXT("Recipe_Empty")), TEXT("Empty Recipe"));
	// Deliberately leave ingredients empty

	TestEqual(TEXT("PrimaryIngredients should be empty"), Recipe.PrimaryIngredients.Num(), 0);
	TestEqual(TEXT("RecipeID should still be valid"), Recipe.RecipeID, FName(TEXT("Recipe_Empty")));

	return true;
}

// ============================================================================
// 6. Recipe Prerequisite Chain Validation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRecipePrerequisiteChain,
	"Odyssey.Crafting.Recipe.PrerequisiteChainSetup",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRecipePrerequisiteChain::RunTest(const FString& Parameters)
{
	using namespace OdysseyTestHelpers;

	// Create a chain: Raw -> Refined -> Advanced -> Final
	FAdvancedCraftingRecipe RawRecipe = MakeTestRecipe(FName(TEXT("Recipe_Raw")), TEXT("Raw Processing"));
	RawRecipe.ChainDepth = 0;

	FAdvancedCraftingRecipe RefinedRecipe = MakeTestRecipe(FName(TEXT("Recipe_Refined")), TEXT("Refining"));
	RefinedRecipe.PrerequisiteRecipes.Add(FName(TEXT("Recipe_Raw")));
	RefinedRecipe.ChainDepth = 1;

	FAdvancedCraftingRecipe AdvancedRecipe = MakeTestRecipe(FName(TEXT("Recipe_Advanced")), TEXT("Advanced Crafting"));
	AdvancedRecipe.PrerequisiteRecipes.Add(FName(TEXT("Recipe_Refined")));
	AdvancedRecipe.ChainDepth = 2;

	FAdvancedCraftingRecipe FinalRecipe = MakeTestRecipe(FName(TEXT("Recipe_Final")), TEXT("Final Assembly"));
	FinalRecipe.PrerequisiteRecipes.Add(FName(TEXT("Recipe_Advanced")));
	FinalRecipe.ChainDepth = 3;

	TestEqual(TEXT("Raw should have 0 prerequisites"), RawRecipe.PrerequisiteRecipes.Num(), 0);
	TestEqual(TEXT("Refined should have 1 prerequisite"), RefinedRecipe.PrerequisiteRecipes.Num(), 1);
	TestEqual(TEXT("Advanced should have 1 prerequisite"), AdvancedRecipe.PrerequisiteRecipes.Num(), 1);
	TestEqual(TEXT("Final should have 1 prerequisite"), FinalRecipe.PrerequisiteRecipes.Num(), 1);

	TestTrue(TEXT("Refined prerequisite should be Raw"), RefinedRecipe.PrerequisiteRecipes.Contains(FName(TEXT("Recipe_Raw"))));
	TestTrue(TEXT("Advanced prerequisite should be Refined"), AdvancedRecipe.PrerequisiteRecipes.Contains(FName(TEXT("Recipe_Refined"))));
	TestTrue(TEXT("Final prerequisite should be Advanced"), FinalRecipe.PrerequisiteRecipes.Contains(FName(TEXT("Recipe_Advanced"))));

	TestEqual(TEXT("Raw chain depth should be 0"), RawRecipe.ChainDepth, 0);
	TestEqual(TEXT("Refined chain depth should be 1"), RefinedRecipe.ChainDepth, 1);
	TestEqual(TEXT("Advanced chain depth should be 2"), AdvancedRecipe.ChainDepth, 2);
	TestEqual(TEXT("Final chain depth should be 3"), FinalRecipe.ChainDepth, 3);

	return true;
}

// ============================================================================
// 7. Recipe With Multiple Prerequisites
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRecipeMultiplePrerequisites,
	"Odyssey.Crafting.Recipe.MultiplePrerequisites",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRecipeMultiplePrerequisites::RunTest(const FString& Parameters)
{
	using namespace OdysseyTestHelpers;

	FAdvancedCraftingRecipe CompositeRecipe = MakeTestRecipe(FName(TEXT("Recipe_Composite")), TEXT("Composite"));
	CompositeRecipe.PrerequisiteRecipes.Add(FName(TEXT("Recipe_Metal")));
	CompositeRecipe.PrerequisiteRecipes.Add(FName(TEXT("Recipe_Polymer")));
	CompositeRecipe.PrerequisiteRecipes.Add(FName(TEXT("Recipe_Ceramic")));

	TestEqual(TEXT("Should have 3 prerequisites"), CompositeRecipe.PrerequisiteRecipes.Num(), 3);
	TestTrue(TEXT("Should contain Metal prereq"), CompositeRecipe.PrerequisiteRecipes.Contains(FName(TEXT("Recipe_Metal"))));
	TestTrue(TEXT("Should contain Polymer prereq"), CompositeRecipe.PrerequisiteRecipes.Contains(FName(TEXT("Recipe_Polymer"))));
	TestTrue(TEXT("Should contain Ceramic prereq"), CompositeRecipe.PrerequisiteRecipes.Contains(FName(TEXT("Recipe_Ceramic"))));

	return true;
}

// ============================================================================
// 8. Recipe Skill Requirements
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRecipeSkillRequirements,
	"Odyssey.Crafting.Recipe.SkillRequirements",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRecipeSkillRequirements::RunTest(const FString& Parameters)
{
	using namespace OdysseyTestHelpers;

	FAdvancedCraftingRecipe Recipe = MakeTestRecipe(FName(TEXT("Recipe_Laser")), TEXT("Laser Assembly"));
	Recipe.RequiredSkillLevels.Add(FName(TEXT("Electronics")), 5);
	Recipe.RequiredSkillLevels.Add(FName(TEXT("WeaponCrafting")), 3);

	TestEqual(TEXT("Should have 2 skill requirements"), Recipe.RequiredSkillLevels.Num(), 2);

	const int32* ElectronicsLevel = Recipe.RequiredSkillLevels.Find(FName(TEXT("Electronics")));
	TestNotNull(TEXT("Electronics skill should exist"), ElectronicsLevel);
	if (ElectronicsLevel)
	{
		TestEqual(TEXT("Electronics level should be 5"), *ElectronicsLevel, 5);
	}

	const int32* WeaponLevel = Recipe.RequiredSkillLevels.Find(FName(TEXT("WeaponCrafting")));
	TestNotNull(TEXT("WeaponCrafting skill should exist"), WeaponLevel);
	if (WeaponLevel)
	{
		TestEqual(TEXT("WeaponCrafting level should be 3"), *WeaponLevel, 3);
	}

	return true;
}

// ============================================================================
// 9. Recipe Bonus Outputs
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRecipeBonusOutputs,
	"Odyssey.Crafting.Recipe.BonusOutputConfiguration",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRecipeBonusOutputs::RunTest(const FString& Parameters)
{
	using namespace OdysseyTestHelpers;

	FAdvancedCraftingRecipe Recipe = MakeTestRecipe(FName(TEXT("Recipe_WithBonus")), TEXT("Bonus Recipe"));
	Recipe.PrimaryOutputs.Add(MakeOutput(EResourceType::CompositeMaterial, 1));
	Recipe.BonusOutputs.Add(MakeOutput(EResourceType::Carbon, 2, 0.5f));
	Recipe.BonusOutputChance = 0.25f;

	TestEqual(TEXT("Should have 1 primary output"), Recipe.PrimaryOutputs.Num(), 1);
	TestEqual(TEXT("Should have 1 bonus output"), Recipe.BonusOutputs.Num(), 1);
	TestEqual(TEXT("Bonus output chance should be 0.25"), Recipe.BonusOutputChance, 0.25f);
	TestEqual(TEXT("Bonus output type should be Carbon"), Recipe.BonusOutputs[0].ResourceType, EResourceType::Carbon);
	TestEqual(TEXT("Bonus output amount should be 2"), Recipe.BonusOutputs[0].Amount, 2);

	return true;
}

// ============================================================================
// 10. Recipe Automation Properties
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRecipeAutomationProperties,
	"Odyssey.Crafting.Recipe.AutomationProperties",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRecipeAutomationProperties::RunTest(const FString& Parameters)
{
	using namespace OdysseyTestHelpers;

	FAdvancedCraftingRecipe AutoRecipe = MakeTestRecipe(FName(TEXT("Recipe_Auto")), TEXT("Automated Recipe"));
	AutoRecipe.bCanBeAutomated = true;
	AutoRecipe.AutomationTierRequired = 4;
	AutoRecipe.AutomationEfficiencyPenalty = 0.1f;

	TestTrue(TEXT("bCanBeAutomated should be true"), AutoRecipe.bCanBeAutomated);
	TestEqual(TEXT("AutomationTierRequired should be 4"), AutoRecipe.AutomationTierRequired, 4);
	TestEqual(TEXT("AutomationEfficiencyPenalty should be 0.1"), AutoRecipe.AutomationEfficiencyPenalty, 0.1f);

	FAdvancedCraftingRecipe ManualRecipe = MakeTestRecipe(FName(TEXT("Recipe_Manual")), TEXT("Manual Only"));
	ManualRecipe.bCanBeAutomated = false;

	TestFalse(TEXT("Manual recipe should not be automatable"), ManualRecipe.bCanBeAutomated);

	return true;
}

// ============================================================================
// 11. Recipe Category Filtering
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRecipeCategoryFiltering,
	"Odyssey.Crafting.Recipe.CategoryAssignment",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRecipeCategoryFiltering::RunTest(const FString& Parameters)
{
	using namespace OdysseyTestHelpers;

	TArray<FAdvancedCraftingRecipe> Recipes;

	auto AddRecipeWithCategory = [&](FName ID, const FString& Name, EItemCategory Cat)
	{
		FAdvancedCraftingRecipe R = MakeTestRecipe(ID, Name);
		R.OutputCategory = Cat;
		Recipes.Add(R);
	};

	AddRecipeWithCategory(FName(TEXT("R1")), TEXT("Weapon A"), EItemCategory::Weapon);
	AddRecipeWithCategory(FName(TEXT("R2")), TEXT("Armor A"), EItemCategory::Equipment);
	AddRecipeWithCategory(FName(TEXT("R3")), TEXT("Weapon B"), EItemCategory::Weapon);
	AddRecipeWithCategory(FName(TEXT("R4")), TEXT("Ammo A"), EItemCategory::Ammunition);
	AddRecipeWithCategory(FName(TEXT("R5")), TEXT("Module A"), EItemCategory::ShipModule);

	// Filter weapons
	int32 WeaponCount = 0;
	for (const auto& R : Recipes)
	{
		if (R.OutputCategory == EItemCategory::Weapon)
		{
			WeaponCount++;
		}
	}
	TestEqual(TEXT("Should have 2 weapon recipes"), WeaponCount, 2);

	// Filter ship modules
	int32 ModuleCount = 0;
	for (const auto& R : Recipes)
	{
		if (R.OutputCategory == EItemCategory::ShipModule)
		{
			ModuleCount++;
		}
	}
	TestEqual(TEXT("Should have 1 ship module recipe"), ModuleCount, 1);

	return true;
}

// ============================================================================
// 12. Recipe Tier Hierarchy
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRecipeTierHierarchy,
	"Odyssey.Crafting.Recipe.TierHierarchyOrder",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRecipeTierHierarchy::RunTest(const FString& Parameters)
{
	// Verify tier ordering is correct for comparison operations
	TestTrue(TEXT("Primitive < Basic"),
		static_cast<uint8>(ECraftingTier::Primitive) < static_cast<uint8>(ECraftingTier::Basic));
	TestTrue(TEXT("Basic < Advanced"),
		static_cast<uint8>(ECraftingTier::Basic) < static_cast<uint8>(ECraftingTier::Advanced));
	TestTrue(TEXT("Advanced < Industrial"),
		static_cast<uint8>(ECraftingTier::Advanced) < static_cast<uint8>(ECraftingTier::Industrial));
	TestTrue(TEXT("Industrial < Automated"),
		static_cast<uint8>(ECraftingTier::Industrial) < static_cast<uint8>(ECraftingTier::Automated));
	TestTrue(TEXT("Automated < Quantum"),
		static_cast<uint8>(ECraftingTier::Automated) < static_cast<uint8>(ECraftingTier::Quantum));

	return true;
}

// ============================================================================
// 13. CraftedItem Default Initialization
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftedItemDefaults,
	"Odyssey.Crafting.Recipe.CraftedItemDefaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftedItemDefaults::RunTest(const FString& Parameters)
{
	FCraftedItem Item;

	TestEqual(TEXT("Default ItemID should be NAME_None"), Item.ItemID, NAME_None);
	TestEqual(TEXT("Default ResourceType should be None"), Item.ResourceType, EResourceType::None);
	TestEqual(TEXT("Default Category should be RawMaterial"), Item.Category, EItemCategory::RawMaterial);
	TestEqual(TEXT("Default Quality should be Common"), Item.Quality, EItemQuality::Common);
	TestEqual(TEXT("Default Quantity should be 1"), Item.Quantity, 1);
	TestEqual(TEXT("Default Durability should be 100"), Item.Durability, 100.0f);
	TestEqual(TEXT("Default QualityMultiplier should be 1.0"), Item.QualityMultiplier, 1.0f);
	TestEqual(TEXT("Default CrafterID should be NAME_None"), Item.CrafterID, NAME_None);
	TestEqual(TEXT("StatModifiers should be empty"), Item.StatModifiers.Num(), 0);

	return true;
}

// ============================================================================
// 14. Recipe Experience Rewards Configuration
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRecipeExperienceRewards,
	"Odyssey.Crafting.Recipe.ExperienceRewardsConfig",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRecipeExperienceRewards::RunTest(const FString& Parameters)
{
	using namespace OdysseyTestHelpers;

	FAdvancedCraftingRecipe Recipe = MakeTestRecipe(FName(TEXT("Recipe_XP")), TEXT("XP Recipe"));
	Recipe.BaseExperienceReward = 50;
	Recipe.SkillExperienceRewards.Add(FName(TEXT("WeaponCrafting")), 25);
	Recipe.SkillExperienceRewards.Add(FName(TEXT("MaterialProcessing")), 10);

	TestEqual(TEXT("BaseExperienceReward should be 50"), Recipe.BaseExperienceReward, 50);
	TestEqual(TEXT("Should have 2 skill XP rewards"), Recipe.SkillExperienceRewards.Num(), 2);

	const int32* WeaponXP = Recipe.SkillExperienceRewards.Find(FName(TEXT("WeaponCrafting")));
	TestNotNull(TEXT("WeaponCrafting XP should exist"), WeaponXP);
	if (WeaponXP)
	{
		TestEqual(TEXT("WeaponCrafting XP should be 25"), *WeaponXP, 25);
	}

	return true;
}

// ============================================================================
// 15. Crafting Statistics Default Initialization
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingStatsDefaults,
	"Odyssey.Crafting.Recipe.StatisticsDefaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingStatsDefaults::RunTest(const FString& Parameters)
{
	FCraftingStatistics Stats;

	TestEqual(TEXT("TotalItemsCrafted should be 0"), Stats.TotalItemsCrafted, 0);
	TestEqual(TEXT("TotalCraftingTimeSpent should be 0"), Stats.TotalCraftingTimeSpent, 0.0f);
	TestEqual(TEXT("SuccessfulCrafts should be 0"), Stats.SuccessfulCrafts, 0);
	TestEqual(TEXT("FailedCrafts should be 0"), Stats.FailedCrafts, 0);
	TestEqual(TEXT("MasterworkItemsCreated should be 0"), Stats.MasterworkItemsCreated, 0);
	TestEqual(TEXT("LegendaryItemsCreated should be 0"), Stats.LegendaryItemsCreated, 0);

	return true;
}

// ============================================================================
// 16. Crafting Job Default State
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingJobDefaults,
	"Odyssey.Crafting.Recipe.CraftingJobDefaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingJobDefaults::RunTest(const FString& Parameters)
{
	FCraftingJob Job;

	TestTrue(TEXT("JobID should be valid GUID"), Job.JobID.IsValid());
	TestEqual(TEXT("RecipeID should be NAME_None"), Job.RecipeID, NAME_None);
	TestEqual(TEXT("Quantity should be 1"), Job.Quantity, 1);
	TestEqual(TEXT("CompletedQuantity should be 0"), Job.CompletedQuantity, 0);
	TestEqual(TEXT("Progress should be 0"), Job.Progress, 0.0f);
	TestEqual(TEXT("RemainingTime should be 0"), Job.RemainingTime, 0.0f);
	TestEqual(TEXT("TotalTime should be 0"), Job.TotalTime, 0.0f);
	TestEqual(TEXT("State should be Idle"), Job.State, ECraftingState::Idle);
	TestEqual(TEXT("TargetQuality should be Standard"), Job.TargetQuality, EItemQuality::Standard);
	TestFalse(TEXT("bIsAutomated should be false"), Job.bIsAutomated);
	TestEqual(TEXT("StationID should be NAME_None"), Job.StationID, NAME_None);
	TestEqual(TEXT("Priority should be 0"), Job.Priority, 0);
	TestEqual(TEXT("ProducedItems should be empty"), Job.ProducedItems.Num(), 0);

	return true;
}

// ============================================================================
// 17. Crafting Facility Default State
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingFacilityDefaults,
	"Odyssey.Crafting.Recipe.FacilityDefaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingFacilityDefaults::RunTest(const FString& Parameters)
{
	FCraftingFacility Facility;

	TestEqual(TEXT("FacilityID should be NAME_None"), Facility.FacilityID, NAME_None);
	TestEqual(TEXT("Tier should be Basic"), Facility.Tier, ECraftingTier::Basic);
	TestEqual(TEXT("Level should be 1"), Facility.Level, 1);
	TestEqual(TEXT("MaxConcurrentJobs should be 1"), Facility.MaxConcurrentJobs, 1);
	TestEqual(TEXT("SpeedMultiplier should be 1.0"), Facility.SpeedMultiplier, 1.0f);
	TestEqual(TEXT("QualityBonus should be 0.0"), Facility.QualityBonus, 0.0f);
	TestEqual(TEXT("EnergyEfficiency should be 1.0"), Facility.EnergyEfficiency, 1.0f);
	TestTrue(TEXT("bIsOnline should be true"), Facility.bIsOnline);
	TestEqual(TEXT("CurrentEnergyDraw should be 0"), Facility.CurrentEnergyDraw, 0.0f);

	return true;
}

// ============================================================================
// 18. Recipe Optional Ingredients
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRecipeOptionalIngredients,
	"Odyssey.Crafting.Recipe.OptionalIngredients",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRecipeOptionalIngredients::RunTest(const FString& Parameters)
{
	using namespace OdysseyTestHelpers;

	FAdvancedCraftingRecipe Recipe = MakeTestRecipe(FName(TEXT("Recipe_Optional")), TEXT("Optional Ingredients"));
	Recipe.PrimaryIngredients.Add(MakeIngredient(EResourceType::Silicate, 5));
	Recipe.OptionalIngredients.Add(MakeIngredient(EResourceType::Carbon, 2));
	Recipe.OptionalIngredients.Add(MakeIngredient(EResourceType::RefinedCarbon, 1));

	TestEqual(TEXT("Should have 1 primary ingredient"), Recipe.PrimaryIngredients.Num(), 1);
	TestEqual(TEXT("Should have 2 optional ingredients"), Recipe.OptionalIngredients.Num(), 2);

	return true;
}

// ============================================================================
// 19. Recipe Unlock Chain
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRecipeUnlockChain,
	"Odyssey.Crafting.Recipe.UnlockChainSetup",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRecipeUnlockChain::RunTest(const FString& Parameters)
{
	using namespace OdysseyTestHelpers;

	FAdvancedCraftingRecipe Recipe = MakeTestRecipe(FName(TEXT("Recipe_Base")), TEXT("Base Recipe"));
	Recipe.UnlocksRecipes.Add(FName(TEXT("Recipe_Tier2A")));
	Recipe.UnlocksRecipes.Add(FName(TEXT("Recipe_Tier2B")));

	TestEqual(TEXT("Should unlock 2 recipes"), Recipe.UnlocksRecipes.Num(), 2);
	TestTrue(TEXT("Should unlock Tier2A"), Recipe.UnlocksRecipes.Contains(FName(TEXT("Recipe_Tier2A"))));
	TestTrue(TEXT("Should unlock Tier2B"), Recipe.UnlocksRecipes.Contains(FName(TEXT("Recipe_Tier2B"))));

	return true;
}

// ============================================================================
// 20. Recipe Required Blueprints
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRecipeRequiredBlueprints,
	"Odyssey.Crafting.Recipe.RequiredBlueprints",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRecipeRequiredBlueprints::RunTest(const FString& Parameters)
{
	using namespace OdysseyTestHelpers;

	FAdvancedCraftingRecipe Recipe = MakeTestRecipe(FName(TEXT("Recipe_BlueprintReq")), TEXT("Blueprint Required"));
	Recipe.RequiredBlueprints.Add(FName(TEXT("BP_AdvancedWeapon")));
	Recipe.RequiredBlueprints.Add(FName(TEXT("BP_EnergyCore")));

	TestEqual(TEXT("Should require 2 blueprints"), Recipe.RequiredBlueprints.Num(), 2);
	TestTrue(TEXT("Should require AdvancedWeapon BP"), Recipe.RequiredBlueprints.Contains(FName(TEXT("BP_AdvancedWeapon"))));
	TestTrue(TEXT("Should require EnergyCore BP"), Recipe.RequiredBlueprints.Contains(FName(TEXT("BP_EnergyCore"))));

	return true;
}
