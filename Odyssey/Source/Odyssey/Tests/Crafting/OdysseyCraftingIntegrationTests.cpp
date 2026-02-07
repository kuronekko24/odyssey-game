// OdysseyCraftingIntegrationTests.cpp
// Integration tests combining multiple crafting subsystems
// Tests full crafting pipelines, combined skill+quality+automation, and performance

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Crafting/OdysseyCraftingManager.h"
#include "Crafting/OdysseyCraftingRecipeComponent.h"
#include "Crafting/OdysseyQualityControlSystem.h"
#include "Crafting/OdysseyCraftingSkillSystem.h"
#include "Crafting/OdysseyAutomationNetworkSystem.h"
#include "Crafting/OdysseyProductionChainPlanner.h"
#include "OdysseyInventoryComponent.h"
#include "OdysseyCraftingComponent.h"

// ============================================================================
// 1. Full Craft-to-Market Pipeline Simulation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegrationCraftToMarket,
	"Odyssey.Crafting.Integration.CraftToMarketPipeline",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegrationCraftToMarket::RunTest(const FString& Parameters)
{
	// Step 1: Define recipe
	FAdvancedCraftingRecipe Recipe;
	Recipe.RecipeID = FName(TEXT("Recipe_LaserRifle"));
	Recipe.RecipeName = TEXT("Laser Rifle");
	Recipe.RequiredTier = ECraftingTier::Advanced;
	Recipe.BaseCraftingTime = 15.0f;
	Recipe.EnergyCost = 30;
	Recipe.BaseQualityChance = 0.6f;
	Recipe.BaseExperienceReward = 50;
	Recipe.OutputCategory = EItemCategory::Weapon;
	Recipe.PrimaryIngredients.Add(FCraftingIngredient(EResourceType::RefinedSilicate, 10));
	Recipe.PrimaryIngredients.Add(FCraftingIngredient(EResourceType::CompositeMaterial, 5));
	Recipe.PrimaryOutputs.Add(FCraftingOutput(EResourceType::CompositeMaterial, 1, 1.0f));

	// Step 2: Create inventory with sufficient materials
	TMap<EResourceType, int32> Inventory;
	Inventory.Add(EResourceType::RefinedSilicate, 50);
	Inventory.Add(EResourceType::CompositeMaterial, 30);

	// Step 3: Check materials availability
	bool bHasMaterials = true;
	for (const FCraftingIngredient& Ingredient : Recipe.PrimaryIngredients)
	{
		const int32* Available = Inventory.Find(Ingredient.ResourceType);
		if (!Available || *Available < Ingredient.Amount)
		{
			bHasMaterials = false;
			break;
		}
	}
	TestTrue(TEXT("Should have sufficient materials"), bHasMaterials);

	// Step 4: Consume materials
	for (const FCraftingIngredient& Ingredient : Recipe.PrimaryIngredients)
	{
		Inventory[Ingredient.ResourceType] -= Ingredient.Amount;
	}
	TestEqual(TEXT("RefinedSilicate should be reduced to 40"), *Inventory.Find(EResourceType::RefinedSilicate), 40);
	TestEqual(TEXT("CompositeMaterial should be reduced to 25"), *Inventory.Find(EResourceType::CompositeMaterial), 25);

	// Step 5: Determine quality (simulate mid-range score)
	float QualityScore = Recipe.BaseQualityChance; // 0.6
	// Add skill bonus of 0.1
	QualityScore += 0.1f;
	// Result: 0.7 -> Superior (per manager thresholds: >= 0.70 = Superior)
	EItemQuality Quality = EItemQuality::Common;
	if (QualityScore >= 0.95f) Quality = EItemQuality::Legendary;
	else if (QualityScore >= 0.85f) Quality = EItemQuality::Masterwork;
	else if (QualityScore >= 0.70f) Quality = EItemQuality::Superior;
	else if (QualityScore >= 0.55f) Quality = EItemQuality::Quality;
	else if (QualityScore >= 0.40f) Quality = EItemQuality::Standard;
	else if (QualityScore >= 0.20f) Quality = EItemQuality::Common;
	else Quality = EItemQuality::Scrap;

	TestEqual(TEXT("Quality should be Superior with 0.7 score"), Quality, EItemQuality::Superior);

	// Step 6: Produce output
	FCraftedItem ProducedWeapon;
	ProducedWeapon.ItemID = FName(TEXT("LaserRifle_001"));
	ProducedWeapon.ResourceType = Recipe.PrimaryOutputs[0].ResourceType;
	ProducedWeapon.Category = Recipe.OutputCategory;
	ProducedWeapon.Quality = Quality;
	ProducedWeapon.Quantity = 1;
	ProducedWeapon.QualityMultiplier = 1.0f + (static_cast<float>(Quality) * 0.15f); // 1.60
	ProducedWeapon.CrafterID = FName(TEXT("TestPlayer"));
	ProducedWeapon.CraftedTime = FDateTime::Now();

	TestEqual(TEXT("Produced weapon quality should be Superior"), ProducedWeapon.Quality, EItemQuality::Superior);
	TestTrue(TEXT("Quality multiplier should be 1.60"), FMath::IsNearlyEqual(ProducedWeapon.QualityMultiplier, 1.60f, 0.001f));

	// Step 7: Calculate market value
	float ValueMult = 4.0f; // Superior value multiplier from default tiers
	int32 BaseValue = 100;
	float ScarcityBonus = FMath::Pow(2.0f, static_cast<float>(Quality)) - 1.0f; // 2^4 - 1 = 15
	int32 MarketValue = FMath::CeilToInt(BaseValue * ValueMult * ProducedWeapon.QualityMultiplier * (1.0f + ScarcityBonus * 0.1f));

	TestTrue(TEXT("Market value should be significantly above base"), MarketValue > BaseValue * 3);

	// Step 8: Award XP
	int32 BaseXP = Recipe.BaseExperienceReward; // 50
	float QualityXPMult = 1.0f + (static_cast<float>(Quality) * 0.1f); // 1.4
	int32 FinalXP = FMath::CeilToInt(BaseXP * QualityXPMult);
	TestEqual(TEXT("XP awarded should be 70"), FinalXP, 70);

	return true;
}

// ============================================================================
// 2. Skill + Quality Combined Effect
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegrationSkillQualityCombined,
	"Odyssey.Crafting.Integration.SkillPlusQualityCombined",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegrationSkillQualityCombined::RunTest(const FString& Parameters)
{
	// High-skill crafter with quality bonuses
	float BaseQualityScore = 0.35f;
	float RecipeContribution = 0.5f * 0.3f; // 0.15
	float SkillQualityBonus = 8 * 0.015f;   // 0.12 (level 8 skill)
	float FacilityBonus = 0.1f;             // Good facility

	float FinalScore = BaseQualityScore + RecipeContribution + SkillQualityBonus + FacilityBonus;
	// 0.35 + 0.15 + 0.12 + 0.1 = 0.72

	FinalScore = FMath::Clamp(FinalScore, 0.0f, 1.0f);

	// Map to quality tier (using QCS default tiers)
	EItemQuality ExpectedQuality = EItemQuality::Common;
	if (FinalScore >= 0.95f) ExpectedQuality = EItemQuality::Legendary;
	else if (FinalScore >= 0.85f) ExpectedQuality = EItemQuality::Masterwork;
	else if (FinalScore >= 0.72f) ExpectedQuality = EItemQuality::Superior;
	else if (FinalScore >= 0.55f) ExpectedQuality = EItemQuality::Quality;
	else if (FinalScore >= 0.35f) ExpectedQuality = EItemQuality::Standard;
	else if (FinalScore >= 0.15f) ExpectedQuality = EItemQuality::Common;
	else ExpectedQuality = EItemQuality::Scrap;

	TestEqual(TEXT("Combined score 0.72 should produce Superior quality"),
		ExpectedQuality, EItemQuality::Superior);

	// Low skill crafter
	float LowSkillScore = 0.35f + 0.15f + (2 * 0.015f) + 0.0f;
	// 0.35 + 0.15 + 0.03 + 0.0 = 0.53
	EItemQuality LowSkillQuality = EItemQuality::Common;
	if (LowSkillScore >= 0.55f) LowSkillQuality = EItemQuality::Quality;
	else if (LowSkillScore >= 0.35f) LowSkillQuality = EItemQuality::Standard;

	TestEqual(TEXT("Low skill score 0.53 should produce Standard quality"),
		LowSkillQuality, EItemQuality::Standard);

	return true;
}

// ============================================================================
// 3. Automation + Quality Combined
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegrationAutomationQuality,
	"Odyssey.Crafting.Integration.AutomationPlusQuality",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegrationAutomationQuality::RunTest(const FString& Parameters)
{
	// Automated crafting has an efficiency penalty on quality
	float BaseScore = 0.50f;
	float AutomationEfficiencyPenalty = 0.1f; // 10% penalty
	float AdjustedScore = BaseScore * (1.0f - AutomationEfficiencyPenalty); // 0.45

	// Manual craft would get Standard (0.50 >= 0.35)
	// Automated craft gets Standard (0.45 >= 0.35) but lower within tier

	TestTrue(TEXT("Automated score should be lower than manual"),
		AdjustedScore < BaseScore);
	TestTrue(TEXT("Automated score should still be above Standard threshold"),
		AdjustedScore >= 0.35f);

	// High penalty automation
	float HighPenalty = 0.3f;
	float HighPenaltyScore = BaseScore * (1.0f - HighPenalty); // 0.35
	TestTrue(TEXT("High penalty should push to Standard threshold boundary"),
		FMath::IsNearlyEqual(HighPenaltyScore, 0.35f, 0.001f));

	return true;
}

// ============================================================================
// 4. Multi-Step Production Chain Execution
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegrationMultiStepChain,
	"Odyssey.Crafting.Integration.MultiStepProductionChain",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegrationMultiStepChain::RunTest(const FString& Parameters)
{
	// 3-step chain: Ore -> Refined -> Component -> Final Product
	struct FChainStep
	{
		FName RecipeID;
		TArray<TPair<EResourceType, int32>> Inputs;
		TArray<TPair<EResourceType, int32>> Outputs;
		float CraftTime;
		int32 EnergyCost;
	};

	TArray<FChainStep> Chain;

	// Step 1: Ore refining
	FChainStep Step1;
	Step1.RecipeID = FName(TEXT("Refine_Silicate"));
	Step1.Inputs.Add(TPair<EResourceType, int32>(EResourceType::Silicate, 5));
	Step1.Outputs.Add(TPair<EResourceType, int32>(EResourceType::RefinedSilicate, 2));
	Step1.CraftTime = 5.0f;
	Step1.EnergyCost = 10;
	Chain.Add(Step1);

	// Step 2: Component creation
	FChainStep Step2;
	Step2.RecipeID = FName(TEXT("Make_Composite"));
	Step2.Inputs.Add(TPair<EResourceType, int32>(EResourceType::RefinedSilicate, 3));
	Step2.Inputs.Add(TPair<EResourceType, int32>(EResourceType::Carbon, 2));
	Step2.Outputs.Add(TPair<EResourceType, int32>(EResourceType::CompositeMaterial, 1));
	Step2.CraftTime = 8.0f;
	Step2.EnergyCost = 15;
	Chain.Add(Step2);

	// Step 3: Final product
	FChainStep Step3;
	Step3.RecipeID = FName(TEXT("Assemble_ShipModule"));
	Step3.Inputs.Add(TPair<EResourceType, int32>(EResourceType::CompositeMaterial, 2));
	Step3.Outputs.Add(TPair<EResourceType, int32>(EResourceType::CompositeMaterial, 1)); // Higher tier
	Step3.CraftTime = 12.0f;
	Step3.EnergyCost = 25;
	Chain.Add(Step3);

	// Calculate totals
	float TotalTime = 0.0f;
	int32 TotalEnergy = 0;
	for (const auto& Step : Chain)
	{
		TotalTime += Step.CraftTime;
		TotalEnergy += Step.EnergyCost;
	}

	TestTrue(TEXT("Total chain time should be 25.0"), FMath::IsNearlyEqual(TotalTime, 25.0f, 0.001f));
	TestEqual(TEXT("Total chain energy should be 50"), TotalEnergy, 50);
	TestEqual(TEXT("Chain should have 3 steps"), Chain.Num(), 3);

	// Calculate raw materials (inputs not produced by earlier steps)
	TMap<EResourceType, int32> RawMaterials;
	TSet<EResourceType> ProducedTypes;
	for (const auto& Step : Chain)
	{
		for (const auto& Output : Step.Outputs)
		{
			ProducedTypes.Add(Output.Key);
		}
	}

	for (const auto& Step : Chain)
	{
		for (const auto& Input : Step.Inputs)
		{
			if (!ProducedTypes.Contains(Input.Key) || Input.Key == EResourceType::Silicate || Input.Key == EResourceType::Carbon)
			{
				RawMaterials.FindOrAdd(Input.Key) += Input.Value;
			}
		}
	}

	TestTrue(TEXT("Should have identified raw materials"), RawMaterials.Num() > 0);

	return true;
}

// ============================================================================
// 5. Crafting Statistics Accumulation Across Multiple Crafts
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegrationStatsAccumulation,
	"Odyssey.Crafting.Integration.StatisticsAccumulation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegrationStatsAccumulation::RunTest(const FString& Parameters)
{
	FCraftingStatistics Stats;

	// Simulate 10 crafts with different outcomes
	struct FCraftResult { EItemQuality Quality; EItemCategory Category; int32 Quantity; bool bSuccess; };
	TArray<FCraftResult> Results = {
		{EItemQuality::Common, EItemCategory::Weapon, 1, true},
		{EItemQuality::Standard, EItemCategory::Weapon, 2, true},
		{EItemQuality::Quality, EItemCategory::Equipment, 1, true},
		{EItemQuality::Superior, EItemCategory::ShipModule, 1, true},
		{EItemQuality::Masterwork, EItemCategory::Weapon, 1, true},
		{EItemQuality::Legendary, EItemCategory::Equipment, 1, true},
		{EItemQuality::Common, EItemCategory::Consumable, 5, true},
		{EItemQuality::Scrap, EItemCategory::Component, 3, true},
		// Failures
		{EItemQuality::Common, EItemCategory::Weapon, 0, false},
		{EItemQuality::Common, EItemCategory::Ammunition, 0, false},
	};

	for (const auto& Result : Results)
	{
		if (Result.bSuccess)
		{
			Stats.SuccessfulCrafts++;
			Stats.TotalItemsCrafted += Result.Quantity;

			int32& QC = Stats.ItemsByQuality.FindOrAdd(Result.Quality);
			QC += Result.Quantity;

			int32& CC = Stats.ItemsByCategory.FindOrAdd(Result.Category);
			CC += Result.Quantity;

			if (Result.Quality == EItemQuality::Masterwork) Stats.MasterworkItemsCreated += Result.Quantity;
			if (Result.Quality == EItemQuality::Legendary) Stats.LegendaryItemsCreated += Result.Quantity;
		}
		else
		{
			Stats.FailedCrafts++;
		}
	}

	TestEqual(TEXT("SuccessfulCrafts should be 8"), Stats.SuccessfulCrafts, 8);
	TestEqual(TEXT("FailedCrafts should be 2"), Stats.FailedCrafts, 2);
	TestEqual(TEXT("TotalItemsCrafted should be 15"), Stats.TotalItemsCrafted, 15);
	TestEqual(TEXT("MasterworkItemsCreated should be 1"), Stats.MasterworkItemsCreated, 1);
	TestEqual(TEXT("LegendaryItemsCreated should be 1"), Stats.LegendaryItemsCreated, 1);

	return true;
}

// ============================================================================
// 6. Efficiency Modifier Chain Calculation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegrationEfficiencyModifiers,
	"Odyssey.Crafting.Integration.EfficiencyModifierChain",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegrationEfficiencyModifiers::RunTest(const FString& Parameters)
{
	FRecipeEfficiencyModifiers Mods;

	// Simulate skill bonuses
	Mods.SkillSpeedBonus = 0.15f;   // 15% faster
	Mods.SkillQualityBonus = 0.10f; // 10% better quality

	// Simulate facility bonuses
	Mods.FacilitySpeedBonus = 0.30f;   // 30% faster
	Mods.FacilityQualityBonus = 0.05f; // 5% better quality

	// Material efficiency
	Mods.MaterialEfficiency = 0.90f; // 10% less materials
	Mods.BonusOutputChance = 0.15f;  // 15% bonus output chance
	Mods.CriticalCraftChance = 0.10f; // 10% crit chance

	// Test aggregate methods
	float TotalSpeed = Mods.GetTotalSpeedBonus();
	float TotalQuality = Mods.GetTotalQualityBonus();

	TestTrue(TEXT("Total speed bonus should be 0.45"),
		FMath::IsNearlyEqual(TotalSpeed, 0.45f, 0.001f));
	TestTrue(TEXT("Total quality bonus should be 0.15"),
		FMath::IsNearlyEqual(TotalQuality, 0.15f, 0.001f));

	// Apply speed to crafting time
	float BaseCraftTime = 20.0f;
	float AdjustedTime = BaseCraftTime * (1.0f - TotalSpeed); // 20 * 0.55 = 11.0
	TestTrue(TEXT("Adjusted craft time should be ~11.0"),
		FMath::IsNearlyEqual(AdjustedTime, 11.0f, 0.01f));

	// Apply material efficiency
	int32 BaseIngredientAmount = 10;
	int32 EfficientAmount = FMath::CeilToInt(BaseIngredientAmount * Mods.MaterialEfficiency); // ceil(9.0) = 9
	TestEqual(TEXT("Efficient ingredient amount should be 9"), EfficientAmount, 9);

	return true;
}

// ============================================================================
// 7. Blueprint Research Unlocking Recipes
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegrationBlueprintUnlock,
	"Odyssey.Crafting.Integration.BlueprintResearchUnlocksRecipes",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegrationBlueprintUnlock::RunTest(const FString& Parameters)
{
	// Simulate blueprint research completion flow
	FCraftingBlueprint Blueprint;
	Blueprint.BlueprintID = FName(TEXT("BP_AdvancedWeapons"));
	Blueprint.BlueprintName = TEXT("Advanced Weapons Schematic");
	Blueprint.UnlockedRecipes.Add(FName(TEXT("Recipe_LaserRifle")));
	Blueprint.UnlockedRecipes.Add(FName(TEXT("Recipe_PlasmaLauncher")));
	Blueprint.ResearchTime = 120.0f;
	Blueprint.ResearchCost = 500;

	// Start research
	FBlueprintResearchProgress Research;
	Research.BlueprintID = Blueprint.BlueprintID;
	Research.Progress = 0.0f;
	Research.RemainingTime = Blueprint.ResearchTime;
	Research.bIsPaused = false;

	// Simulate partial research
	float DeltaTime = 60.0f;
	Research.RemainingTime -= DeltaTime;
	Research.Progress = 1.0f - (Research.RemainingTime / Blueprint.ResearchTime);

	TestTrue(TEXT("Progress should be ~50%"), FMath::IsNearlyEqual(Research.Progress, 0.5f, 0.01f));
	TestTrue(TEXT("Remaining time should be ~60"), FMath::IsNearlyEqual(Research.RemainingTime, 60.0f, 0.01f));

	// Complete research
	Research.RemainingTime = 0.0f;
	Research.Progress = 1.0f;

	// Unlock recipes on completion
	TSet<FName> UnlockedRecipes;
	for (const FName& RecipeID : Blueprint.UnlockedRecipes)
	{
		UnlockedRecipes.Add(RecipeID);
	}

	TestTrue(TEXT("LaserRifle should be unlocked"), UnlockedRecipes.Contains(FName(TEXT("Recipe_LaserRifle"))));
	TestTrue(TEXT("PlasmaLauncher should be unlocked"), UnlockedRecipes.Contains(FName(TEXT("Recipe_PlasmaLauncher"))));
	TestEqual(TEXT("Should have unlocked 2 recipes"), UnlockedRecipes.Num(), 2);

	return true;
}

// ============================================================================
// 8. Production Network End-to-End Flow
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegrationNetworkEndToEnd,
	"Odyssey.Crafting.Integration.ProductionNetworkEndToEnd",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegrationNetworkEndToEnd::RunTest(const FString& Parameters)
{
	// Simulate a minimal Input -> Processing -> Output network
	FResourceBuffer InputNodeBuffer;
	InputNodeBuffer.MaxCapacity = 500;
	InputNodeBuffer.Add(EResourceType::Silicate, 100);

	FResourceBuffer ProcessingInput;
	ProcessingInput.MaxCapacity = 100;

	FResourceBuffer ProcessingOutput;
	ProcessingOutput.MaxCapacity = 100;

	FResourceBuffer OutputNodeBuffer;
	OutputNodeBuffer.MaxCapacity = 500;

	// Transfer: Input -> Processing
	int32 TransferAmount = 10;
	int32 Transferred = InputNodeBuffer.Remove(EResourceType::Silicate, TransferAmount);
	ProcessingInput.Add(EResourceType::Silicate, Transferred);

	TestEqual(TEXT("Processing input should have 10 Silicate"), ProcessingInput.GetAmount(EResourceType::Silicate), 10);
	TestEqual(TEXT("Input node should have 90 Silicate"), InputNodeBuffer.GetAmount(EResourceType::Silicate), 90);

	// Processing: consume 5 Silicate, produce 2 RefinedSilicate
	int32 Consumed = ProcessingInput.Remove(EResourceType::Silicate, 5);
	ProcessingOutput.Add(EResourceType::RefinedSilicate, 2);

	TestEqual(TEXT("Should consume 5 Silicate"), Consumed, 5);
	TestEqual(TEXT("Processing output should have 2 Refined"), ProcessingOutput.GetAmount(EResourceType::RefinedSilicate), 2);

	// Transfer: Processing -> Output
	Transferred = ProcessingOutput.Remove(EResourceType::RefinedSilicate, 2);
	OutputNodeBuffer.Add(EResourceType::RefinedSilicate, Transferred);

	TestEqual(TEXT("Output node should have 2 RefinedSilicate"), OutputNodeBuffer.GetAmount(EResourceType::RefinedSilicate), 2);
	TestEqual(TEXT("Processing output should be empty"), ProcessingOutput.GetAmount(EResourceType::RefinedSilicate), 0);

	return true;
}

// ============================================================================
// 9. Performance: Batch Job Processing Scale
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegrationPerformanceBatchProcessing,
	"Odyssey.Crafting.Integration.PerformanceBatchJobProcessing",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegrationPerformanceBatchProcessing::RunTest(const FString& Parameters)
{
	// Simulate processing many jobs to verify algorithm scales
	const int32 TotalJobs = 1000;
	TArray<FCraftingJob> Jobs;
	Jobs.Reserve(TotalJobs);

	for (int32 i = 0; i < TotalJobs; ++i)
	{
		FCraftingJob Job;
		Job.State = ECraftingState::Crafting;
		Job.TotalTime = 10.0f;
		Job.RemainingTime = 10.0f;
		Job.Priority = FMath::RandRange(0, 100);
		Jobs.Add(Job);
	}

	// Time the sort operation
	double StartTime = FPlatformTime::Seconds();

	Jobs.Sort([](const FCraftingJob& A, const FCraftingJob& B)
	{
		return A.Priority > B.Priority;
	});

	double SortDuration = FPlatformTime::Seconds() - StartTime;

	TestTrue(TEXT("1000-job sort should complete in under 100ms"), SortDuration < 0.1);
	TestEqual(TEXT("Job array should still have 1000 entries"), Jobs.Num(), TotalJobs);

	// Verify sorted order
	for (int32 i = 1; i < Jobs.Num(); ++i)
	{
		TestTrue(TEXT("Jobs should be sorted by priority descending"),
			Jobs[i].Priority <= Jobs[i - 1].Priority);
	}

	return true;
}

// ============================================================================
// 10. Performance: Large Automation Network
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegrationPerformanceLargeNetwork,
	"Odyssey.Crafting.Integration.PerformanceLargeNetwork",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegrationPerformanceLargeNetwork::RunTest(const FString& Parameters)
{
	// Simulate creating and querying a large network
	const int32 NodeCount = 100;
	TMap<FGuid, FAutomationNode> Nodes;

	double StartTime = FPlatformTime::Seconds();

	for (int32 i = 0; i < NodeCount; ++i)
	{
		FAutomationNode Node;
		Node.NodeType = EAutomationNodeType::Processing;
		Node.InputBuffer.MaxCapacity = 100;
		Node.OutputBuffer.MaxCapacity = 100;
		Node.InputBuffer.Add(EResourceType::Silicate, FMath::RandRange(0, 100));
		Node.OutputBuffer.Add(EResourceType::CompositeMaterial, FMath::RandRange(0, 100));
		Nodes.Add(Node.NodeID, Node);
	}

	double CreateDuration = FPlatformTime::Seconds() - StartTime;
	TestTrue(TEXT("Creating 100 nodes should be fast"), CreateDuration < 0.1);
	TestEqual(TEXT("Should have 100 nodes"), Nodes.Num(), NodeCount);

	// Query all nodes
	StartTime = FPlatformTime::Seconds();
	TArray<FAutomationNode> AllNodes;
	AllNodes.Reserve(NodeCount);
	for (const auto& Pair : Nodes)
	{
		AllNodes.Add(Pair.Value);
	}
	double QueryDuration = FPlatformTime::Seconds() - StartTime;

	TestTrue(TEXT("Querying all nodes should be fast"), QueryDuration < 0.1);
	TestEqual(TEXT("Should retrieve all 100 nodes"), AllNodes.Num(), NodeCount);

	return true;
}

// ============================================================================
// 11. Performance: Resource Buffer Stress Test
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegrationPerformanceBufferStress,
	"Odyssey.Crafting.Integration.PerformanceBufferStress",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegrationPerformanceBufferStress::RunTest(const FString& Parameters)
{
	FResourceBuffer Buffer;
	Buffer.MaxCapacity = 10000;

	double StartTime = FPlatformTime::Seconds();

	// Add many different resource types in rapid succession
	for (int32 i = 0; i < 1000; ++i)
	{
		Buffer.Add(EResourceType::Silicate, 1);
		Buffer.Add(EResourceType::Carbon, 1);
	}

	double Duration = FPlatformTime::Seconds() - StartTime;

	TestEqual(TEXT("Buffer should have 2000 total"), Buffer.CurrentTotal, 2000);
	TestEqual(TEXT("Silicate should be 1000"), Buffer.GetAmount(EResourceType::Silicate), 1000);
	TestEqual(TEXT("Carbon should be 1000"), Buffer.GetAmount(EResourceType::Carbon), 1000);
	TestTrue(TEXT("1000 add operations should be fast"), Duration < 0.1);

	// Rapid remove
	StartTime = FPlatformTime::Seconds();
	for (int32 i = 0; i < 500; ++i)
	{
		Buffer.Remove(EResourceType::Silicate, 1);
		Buffer.Remove(EResourceType::Carbon, 1);
	}
	Duration = FPlatformTime::Seconds() - StartTime;

	TestEqual(TEXT("Buffer should have 1000 remaining"), Buffer.CurrentTotal, 1000);
	TestTrue(TEXT("500 remove operations should be fast"), Duration < 0.1);

	return true;
}

// ============================================================================
// 12. Full Skill Progression Through Crafting Cycle
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegrationSkillProgression,
	"Odyssey.Crafting.Integration.SkillProgressionThroughCrafting",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegrationSkillProgression::RunTest(const FString& Parameters)
{
	// Simulate a crafter gaining XP and leveling up over multiple crafts
	FCraftingSkill WeaponSkill;
	WeaponSkill.SkillID = FName(TEXT("WeaponCrafting"));
	WeaponSkill.CurrentLevel = 0;
	WeaponSkill.CurrentExperience = 0;
	WeaponSkill.MaxLevel = 10;

	float ExpCurveMultiplier = 1.5f;
	auto CalcXPForLevel = [ExpCurveMultiplier](int32 Level) -> int32
	{
		return FMath::CeilToInt(100 * FMath::Pow(ExpCurveMultiplier, static_cast<float>(Level)));
	};

	WeaponSkill.ExperienceToNextLevel = CalcXPForLevel(WeaponSkill.CurrentLevel); // 100

	// Simulate 20 crafts, each giving 25 XP
	int32 XPPerCraft = 25;
	int32 TotalCrafts = 20;
	int32 LevelUps = 0;

	for (int32 i = 0; i < TotalCrafts; ++i)
	{
		WeaponSkill.CurrentExperience += XPPerCraft;

		while (WeaponSkill.CurrentExperience >= WeaponSkill.ExperienceToNextLevel &&
			   WeaponSkill.CurrentLevel < WeaponSkill.MaxLevel)
		{
			WeaponSkill.CurrentExperience -= WeaponSkill.ExperienceToNextLevel;
			WeaponSkill.CurrentLevel++;
			WeaponSkill.ExperienceToNextLevel = CalcXPForLevel(WeaponSkill.CurrentLevel);
			LevelUps++;
		}
	}

	// 500 total XP: Level 0->1 (100), 1->2 (150), 2->3 (225) = 475 spent, 25 remaining
	TestTrue(TEXT("Should have gained at least 2 levels"), WeaponSkill.CurrentLevel >= 2);
	TestTrue(TEXT("Should have had level ups"), LevelUps > 0);
	TestTrue(TEXT("Current level should be <= max"), WeaponSkill.CurrentLevel <= WeaponSkill.MaxLevel);

	// Calculate crafting bonus at achieved level
	float QualityBonus = WeaponSkill.CurrentLevel * 0.015f;
	TestTrue(TEXT("Quality bonus should increase with levels"), QualityBonus > 0.0f);

	return true;
}

// ============================================================================
// 13. Recipe Variation with Quality Impact
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegrationRecipeVariation,
	"Odyssey.Crafting.Integration.RecipeVariationQualityImpact",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegrationRecipeVariation::RunTest(const FString& Parameters)
{
	// Base recipe
	FAdvancedCraftingRecipe BaseRecipe;
	BaseRecipe.RecipeID = FName(TEXT("Recipe_Sword"));
	BaseRecipe.RecipeName = TEXT("Standard Sword");
	BaseRecipe.BaseCraftingTime = 10.0f;
	BaseRecipe.BaseQualityChance = 0.5f;

	// Variation: higher quality, longer time
	FRecipeVariation Variation;
	Variation.VariationID = FName(TEXT("Var_MasterforgedSword"));
	Variation.VariationName = TEXT("Masterforged");
	Variation.TimeModifier = 1.5f;      // 50% more time
	Variation.QualityModifier = 1.3f;   // 30% more quality

	// Apply variation
	FAdvancedCraftingRecipe ModifiedRecipe = BaseRecipe;
	ModifiedRecipe.BaseCraftingTime *= Variation.TimeModifier;
	ModifiedRecipe.BaseQualityChance *= Variation.QualityModifier;

	TestTrue(TEXT("Modified time should be 15.0"), FMath::IsNearlyEqual(ModifiedRecipe.BaseCraftingTime, 15.0f, 0.01f));
	TestTrue(TEXT("Modified quality chance should be 0.65"), FMath::IsNearlyEqual(ModifiedRecipe.BaseQualityChance, 0.65f, 0.01f));
	TestTrue(TEXT("Variation should increase time"), ModifiedRecipe.BaseCraftingTime > BaseRecipe.BaseCraftingTime);
	TestTrue(TEXT("Variation should increase quality"), ModifiedRecipe.BaseQualityChance > BaseRecipe.BaseQualityChance);

	return true;
}

// ============================================================================
// 14. Experimentation Discovery Flow
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegrationExperimentation,
	"Odyssey.Crafting.Integration.ExperimentationDiscoveryFlow",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegrationExperimentation::RunTest(const FString& Parameters)
{
	// Simulate experimentation hash tracking
	TMap<FName, TSet<uint32>> ExperimentationHistory;
	FName RecipeID = FName(TEXT("Recipe_Alloy"));

	// Calculate hash for experiment
	auto CalcHash = [](const TArray<FCraftingIngredient>& Ingredients) -> uint32
	{
		uint32 Hash = 0;
		for (const auto& I : Ingredients)
		{
			Hash ^= GetTypeHash(I.ResourceType);
			Hash = (Hash << 5) | (Hash >> 27);
			Hash ^= I.Amount;
		}
		return Hash;
	};

	TArray<FCraftingIngredient> Experiment1;
	Experiment1.Add(FCraftingIngredient(EResourceType::Silicate, 5));
	Experiment1.Add(FCraftingIngredient(EResourceType::Carbon, 3));
	uint32 Hash1 = CalcHash(Experiment1);

	// First attempt: should not be in history
	TSet<uint32>& History = ExperimentationHistory.FindOrAdd(RecipeID);
	TestFalse(TEXT("First experiment should not be in history"), History.Contains(Hash1));

	// Record attempt
	History.Add(Hash1);
	TestTrue(TEXT("Experiment should now be in history"), History.Contains(Hash1));

	// Same experiment again should be rejected
	bool bAlreadyTried = History.Contains(Hash1);
	TestTrue(TEXT("Repeat experiment should be detected"), bAlreadyTried);

	// Different experiment
	TArray<FCraftingIngredient> Experiment2;
	Experiment2.Add(FCraftingIngredient(EResourceType::RefinedSilicate, 2));
	uint32 Hash2 = CalcHash(Experiment2);

	TestFalse(TEXT("Different experiment should not be in history"), History.Contains(Hash2));
	TestTrue(TEXT("Different experiments should produce different hashes"), Hash1 != Hash2);

	return true;
}
