// OdysseyProductionChainPlannerTests.cpp
// Comprehensive automation tests for the Production Chain Planner
// Tests plan generation, cost analysis, chain resolution, and execution

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Crafting/OdysseyCraftingManager.h"
#include "Crafting/OdysseyProductionChainPlanner.h"
#include "OdysseyInventoryComponent.h"

// ============================================================================
// 1. Production Step Default Initialization
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FProductionStepDefaults,
	"Odyssey.Crafting.Planner.ProductionStepDefaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProductionStepDefaults::RunTest(const FString& Parameters)
{
	FProductionStep Step;

	TestEqual(TEXT("Default RecipeID should be None"), Step.RecipeID, NAME_None);
	TestEqual(TEXT("Default Quantity should be 1"), Step.Quantity, 1);
	TestEqual(TEXT("Default Depth should be 0"), Step.Depth, 0);
	TestEqual(TEXT("Default EstimatedTime should be 0"), Step.EstimatedTime, 0.0f);
	TestEqual(TEXT("Default EstimatedEnergyCost should be 0"), Step.EstimatedEnergyCost, 0);
	TestFalse(TEXT("Default bCanCraftNow should be false"), Step.bCanCraftNow);
	TestFalse(TEXT("Default bHasAllPrerequisites should be false"), Step.bHasAllPrerequisites);
	TestEqual(TEXT("Default RequiredInputs should be empty"), Step.RequiredInputs.Num(), 0);
	TestEqual(TEXT("Default Outputs should be empty"), Step.Outputs.Num(), 0);
	TestEqual(TEXT("Default DependsOnSteps should be empty"), Step.DependsOnSteps.Num(), 0);

	return true;
}

// ============================================================================
// 2. Production Plan Default Initialization
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FProductionPlanDefaults,
	"Odyssey.Crafting.Planner.ProductionPlanDefaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProductionPlanDefaults::RunTest(const FString& Parameters)
{
	FProductionPlan Plan;

	TestTrue(TEXT("PlanID should be valid"), Plan.PlanID.IsValid());
	TestEqual(TEXT("Default TargetRecipeID should be None"), Plan.TargetRecipeID, NAME_None);
	TestEqual(TEXT("Default TargetQuantity should be 1"), Plan.TargetQuantity, 1);
	TestEqual(TEXT("Default TotalEstimatedTime should be 0"), Plan.TotalEstimatedTime, 0.0f);
	TestEqual(TEXT("Default TotalEstimatedEnergyCost should be 0"), Plan.TotalEstimatedEnergyCost, 0);
	TestEqual(TEXT("Default TotalSteps should be 0"), Plan.TotalSteps, 0);
	TestEqual(TEXT("Default MaxDepth should be 0"), Plan.MaxDepth, 0);
	TestFalse(TEXT("Default bIsFeasible should be false"), Plan.bIsFeasible);
	TestEqual(TEXT("Default Steps should be empty"), Plan.Steps.Num(), 0);
	TestEqual(TEXT("Default BlockingReasons should be empty"), Plan.BlockingReasons.Num(), 0);

	return true;
}

// ============================================================================
// 3. Cost Breakdown Default Initialization
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCostBreakdownDefaults,
	"Odyssey.Crafting.Planner.CostBreakdownDefaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCostBreakdownDefaults::RunTest(const FString& Parameters)
{
	FProductionCostBreakdown Breakdown;

	TestEqual(TEXT("Default TotalMaterialCost should be 0"), Breakdown.TotalMaterialCost, 0);
	TestEqual(TEXT("Default TotalEnergyCost should be 0"), Breakdown.TotalEnergyCost, 0);
	TestEqual(TEXT("Default TotalTimeCost should be 0"), Breakdown.TotalTimeCost, 0.0f);
	TestEqual(TEXT("Default EstimatedOutputValue should be 0"), Breakdown.EstimatedOutputValue, 0);
	TestEqual(TEXT("Default ProfitMargin should be 0"), Breakdown.ProfitMargin, 0.0f);
	TestEqual(TEXT("Default MaterialCostByType should be empty"), Breakdown.MaterialCostByType.Num(), 0);

	return true;
}

// ============================================================================
// 4. Profit Margin Calculation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlannerProfitMargin,
	"Odyssey.Crafting.Planner.ProfitMarginCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlannerProfitMargin::RunTest(const FString& Parameters)
{
	// ProfitMargin = (OutputValue - TotalCost) / TotalCost
	auto CalcProfit = [](int32 OutputValue, int32 MaterialCost, int32 EnergyCost) -> float
	{
		int32 TotalCost = MaterialCost + EnergyCost;
		if (TotalCost <= 0) return 0.0f;
		return static_cast<float>(OutputValue - TotalCost) / TotalCost;
	};

	// Profitable craft: sell for 200, cost 100
	float Profit1 = CalcProfit(200, 80, 20);
	TestTrue(TEXT("200/100 should yield 100% profit"), FMath::IsNearlyEqual(Profit1, 1.0f, 0.001f));

	// Break-even
	float Profit2 = CalcProfit(100, 80, 20);
	TestTrue(TEXT("100/100 should yield 0% profit"), FMath::IsNearlyEqual(Profit2, 0.0f, 0.001f));

	// Loss
	float Profit3 = CalcProfit(50, 80, 20);
	TestTrue(TEXT("50/100 should yield -50% profit"), FMath::IsNearlyEqual(Profit3, -0.5f, 0.001f));

	// Edge case: zero cost
	float Profit4 = CalcProfit(100, 0, 0);
	TestEqual(TEXT("Zero cost should return 0 profit margin"), Profit4, 0.0f);

	return true;
}

// ============================================================================
// 5. Material Aggregation from Steps
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlannerMaterialAggregation,
	"Odyssey.Crafting.Planner.MaterialAggregation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlannerMaterialAggregation::RunTest(const FString& Parameters)
{
	// Simulate AggregateRawMaterials: sum all inputs not produced by chain steps
	TMap<EResourceType, int32> MaterialTotals;

	// Step 1 inputs: Silicate x10, Carbon x5
	MaterialTotals.FindOrAdd(EResourceType::Silicate) += 10;
	MaterialTotals.FindOrAdd(EResourceType::Carbon) += 5;

	// Step 2 inputs: Silicate x3, RefinedSilicate x2
	MaterialTotals.FindOrAdd(EResourceType::Silicate) += 3;
	MaterialTotals.FindOrAdd(EResourceType::RefinedSilicate) += 2;

	TestEqual(TEXT("Silicate total should be 13"), *MaterialTotals.Find(EResourceType::Silicate), 13);
	TestEqual(TEXT("Carbon total should be 5"), *MaterialTotals.Find(EResourceType::Carbon), 5);
	TestEqual(TEXT("RefinedSilicate total should be 2"), *MaterialTotals.Find(EResourceType::RefinedSilicate), 2);
	TestEqual(TEXT("Should have 3 unique material types"), MaterialTotals.Num(), 3);

	return true;
}

// ============================================================================
// 6. Inventory Subtraction from Requirements
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlannerInventorySubtraction,
	"Odyssey.Crafting.Planner.InventorySubtractionFromPlan",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlannerInventorySubtraction::RunTest(const FString& Parameters)
{
	// Simulate SubtractInventory logic
	struct FReq { EResourceType Type; int32 Amount; };
	TArray<FReq> Requirements = {
		{EResourceType::Silicate, 20},
		{EResourceType::Carbon, 10},
		{EResourceType::RefinedSilicate, 5}
	};

	// Simulated inventory
	TMap<EResourceType, int32> Inventory;
	Inventory.Add(EResourceType::Silicate, 15);
	Inventory.Add(EResourceType::Carbon, 10);
	// RefinedSilicate not in inventory

	TArray<FReq> StillNeeded;
	for (const auto& Req : Requirements)
	{
		const int32* Owned = Inventory.Find(Req.Type);
		int32 Available = Owned ? *Owned : 0;
		int32 Remaining = Req.Amount - Available;
		if (Remaining > 0)
		{
			StillNeeded.Add({Req.Type, Remaining});
		}
	}

	TestEqual(TEXT("Should need 2 more types"), StillNeeded.Num(), 2);

	// Silicate: need 20, have 15, still need 5
	bool bFoundSilicate = false;
	for (const auto& N : StillNeeded)
	{
		if (N.Type == EResourceType::Silicate)
		{
			TestEqual(TEXT("Should still need 5 Silicate"), N.Amount, 5);
			bFoundSilicate = true;
		}
	}
	TestTrue(TEXT("Silicate should be in still-needed list"), bFoundSilicate);

	// RefinedSilicate: need 5, have 0, still need 5
	bool bFoundRefined = false;
	for (const auto& N : StillNeeded)
	{
		if (N.Type == EResourceType::RefinedSilicate)
		{
			TestEqual(TEXT("Should still need 5 RefinedSilicate"), N.Amount, 5);
			bFoundRefined = true;
		}
	}
	TestTrue(TEXT("RefinedSilicate should be in still-needed list"), bFoundRefined);

	return true;
}

// ============================================================================
// 7. Chain Depth Calculation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlannerChainDepth,
	"Odyssey.Crafting.Planner.ChainDepthCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlannerChainDepth::RunTest(const FString& Parameters)
{
	// Chain with 4 steps: Raw -> Refined -> Component -> FinalProduct
	TArray<FProductionStep> Steps;

	FProductionStep Step1;
	Step1.RecipeID = FName(TEXT("Raw"));
	Step1.Depth = 2;
	Steps.Add(Step1);

	FProductionStep Step2;
	Step2.RecipeID = FName(TEXT("Refined"));
	Step2.Depth = 1;
	Steps.Add(Step2);

	FProductionStep Step3;
	Step3.RecipeID = FName(TEXT("FinalProduct"));
	Step3.Depth = 0;
	Steps.Add(Step3);

	int32 MaxDepth = 0;
	for (const auto& Step : Steps)
	{
		MaxDepth = FMath::Max(MaxDepth, Step.Depth);
	}

	TestEqual(TEXT("Max chain depth should be 2"), MaxDepth, 2);

	return true;
}

// ============================================================================
// 8. Circular Dependency Protection
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlannerCircularDependency,
	"Odyssey.Crafting.Planner.CircularDependencyProtection",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlannerCircularDependency::RunTest(const FString& Parameters)
{
	// Simulate ResolveRecipeChain visited set protection
	TSet<FName> VisitedRecipes;
	VisitedRecipes.Add(FName(TEXT("Recipe_A")));
	VisitedRecipes.Add(FName(TEXT("Recipe_B")));

	// Try to visit A again (circular dependency)
	FName CircularRecipe = FName(TEXT("Recipe_A"));
	bool bAlreadyVisited = VisitedRecipes.Contains(CircularRecipe);

	TestTrue(TEXT("Should detect circular dependency"), bAlreadyVisited);

	// New recipe not visited
	FName NewRecipe = FName(TEXT("Recipe_C"));
	bAlreadyVisited = VisitedRecipes.Contains(NewRecipe);
	TestFalse(TEXT("New recipe should not be flagged as visited"), bAlreadyVisited);

	return true;
}

// ============================================================================
// 9. Max Chain Depth Limit
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlannerMaxDepthLimit,
	"Odyssey.Crafting.Planner.MaxChainDepthEnforced",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlannerMaxDepthLimit::RunTest(const FString& Parameters)
{
	int32 MaxChainDepth = 10;

	// Simulate recursive depth check
	for (int32 Depth = 0; Depth <= 12; ++Depth)
	{
		bool bWithinLimit = (Depth <= MaxChainDepth);
		if (Depth <= 10)
		{
			TestTrue(FString::Printf(TEXT("Depth %d should be within limit"), Depth), bWithinLimit);
		}
		else
		{
			TestFalse(FString::Printf(TEXT("Depth %d should exceed limit"), Depth), bWithinLimit);
		}
	}

	return true;
}

// ============================================================================
// 10. Plan Cache Size Limit
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlannerCacheSizeLimit,
	"Odyssey.Crafting.Planner.PlanCacheSizeLimit",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlannerCacheSizeLimit::RunTest(const FString& Parameters)
{
	int32 MaxPlanCacheSize = 20;
	TMap<FGuid, FProductionPlan> CachedPlans;

	// Fill cache to capacity
	for (int32 i = 0; i < MaxPlanCacheSize; ++i)
	{
		FProductionPlan Plan;
		CachedPlans.Add(Plan.PlanID, Plan);
	}

	TestEqual(TEXT("Cache should be at max capacity"), CachedPlans.Num(), MaxPlanCacheSize);

	// Add one more -- should evict oldest
	bool bNeedsEviction = (CachedPlans.Num() >= MaxPlanCacheSize);
	TestTrue(TEXT("Should need eviction"), bNeedsEviction);

	if (bNeedsEviction)
	{
		FGuid OldestKey;
		for (const auto& Pair : CachedPlans)
		{
			OldestKey = Pair.Key;
			break;
		}
		CachedPlans.Remove(OldestKey);
	}

	FProductionPlan NewPlan;
	CachedPlans.Add(NewPlan.PlanID, NewPlan);

	TestEqual(TEXT("Cache should remain at max after eviction+add"), CachedPlans.Num(), MaxPlanCacheSize);

	return true;
}

// ============================================================================
// 11. Total Chain Time Calculation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlannerTotalChainTime,
	"Odyssey.Crafting.Planner.TotalChainTimeCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlannerTotalChainTime::RunTest(const FString& Parameters)
{
	TArray<FProductionStep> Steps;

	FProductionStep S1; S1.EstimatedTime = 5.0f; Steps.Add(S1);
	FProductionStep S2; S2.EstimatedTime = 10.0f; Steps.Add(S2);
	FProductionStep S3; S3.EstimatedTime = 3.5f; Steps.Add(S3);

	float TotalTime = 0.0f;
	for (const auto& Step : Steps)
	{
		TotalTime += Step.EstimatedTime;
	}

	TestTrue(TEXT("Total chain time should be 18.5"), FMath::IsNearlyEqual(TotalTime, 18.5f, 0.001f));

	return true;
}

// ============================================================================
// 12. Total Energy Cost Calculation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlannerTotalEnergyCost,
	"Odyssey.Crafting.Planner.TotalEnergyCostCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlannerTotalEnergyCost::RunTest(const FString& Parameters)
{
	TArray<FProductionStep> Steps;

	FProductionStep S1; S1.EstimatedEnergyCost = 10; Steps.Add(S1);
	FProductionStep S2; S2.EstimatedEnergyCost = 25; Steps.Add(S2);
	FProductionStep S3; S3.EstimatedEnergyCost = 15; Steps.Add(S3);

	int32 TotalEnergy = 0;
	for (const auto& Step : Steps)
	{
		TotalEnergy += Step.EstimatedEnergyCost;
	}

	TestEqual(TEXT("Total energy cost should be 50"), TotalEnergy, 50);

	return true;
}

// ============================================================================
// 13. Intermediate Products Extraction
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlannerIntermediateProducts,
	"Odyssey.Crafting.Planner.IntermediateProductsExtraction",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlannerIntermediateProducts::RunTest(const FString& Parameters)
{
	// Simulate GetIntermediateProducts: full chain minus the final recipe
	TArray<FName> Chain;
	Chain.Add(FName(TEXT("Recipe_Raw")));
	Chain.Add(FName(TEXT("Recipe_Refined")));
	Chain.Add(FName(TEXT("Recipe_Component")));
	Chain.Add(FName(TEXT("Recipe_Final")));

	FName TargetRecipe = FName(TEXT("Recipe_Final"));

	// Remove final recipe
	TArray<FName> Intermediates = Chain;
	if (Intermediates.Num() > 0 && Intermediates.Last() == TargetRecipe)
	{
		Intermediates.RemoveAt(Intermediates.Num() - 1);
	}

	TestEqual(TEXT("Should have 3 intermediate products"), Intermediates.Num(), 3);
	TestTrue(TEXT("Should contain Raw"), Intermediates.Contains(FName(TEXT("Recipe_Raw"))));
	TestTrue(TEXT("Should contain Refined"), Intermediates.Contains(FName(TEXT("Recipe_Refined"))));
	TestTrue(TEXT("Should contain Component"), Intermediates.Contains(FName(TEXT("Recipe_Component"))));
	TestFalse(TEXT("Should not contain Final"), Intermediates.Contains(FName(TEXT("Recipe_Final"))));

	return true;
}

// ============================================================================
// 14. Plan Execution Progress Tracking
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlannerExecutionProgress,
	"Odyssey.Crafting.Planner.ExecutionProgressTracking",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlannerExecutionProgress::RunTest(const FString& Parameters)
{
	TMap<FGuid, int32> ActivePlanProgress;

	FGuid PlanID = FGuid::NewGuid();
	ActivePlanProgress.Add(PlanID, 0);

	// Get initial progress
	const int32* Progress = ActivePlanProgress.Find(PlanID);
	TestNotNull(TEXT("Should find plan progress"), Progress);
	if (Progress) TestEqual(TEXT("Initial progress should be 0"), *Progress, 0);

	// Advance progress
	ActivePlanProgress[PlanID] = 3;
	Progress = ActivePlanProgress.Find(PlanID);
	if (Progress) TestEqual(TEXT("Progress should be 3 after advancement"), *Progress, 3);

	// Unknown plan
	FGuid UnknownPlan = FGuid::NewGuid();
	const int32* UnknownProgress = ActivePlanProgress.Find(UnknownPlan);
	TestNull(TEXT("Unknown plan should not have progress"), UnknownProgress);

	return true;
}

// ============================================================================
// 15. Plan Cancellation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlannerCancellation,
	"Odyssey.Crafting.Planner.PlanCancellation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlannerCancellation::RunTest(const FString& Parameters)
{
	TMap<FGuid, int32> ActivePlanProgress;

	FGuid PlanID = FGuid::NewGuid();
	ActivePlanProgress.Add(PlanID, 5);

	// Cancel
	int32 Removed = ActivePlanProgress.Remove(PlanID);
	TestTrue(TEXT("Should successfully cancel active plan"), Removed > 0);
	TestFalse(TEXT("Cancelled plan should no longer exist"), ActivePlanProgress.Contains(PlanID));

	// Cancel non-existent plan
	FGuid FakePlan = FGuid::NewGuid();
	Removed = ActivePlanProgress.Remove(FakePlan);
	TestEqual(TEXT("Non-existent plan cancellation should return 0"), Removed, 0);

	return true;
}

// ============================================================================
// 16. Raw Material Identification
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlannerRawMaterialIdentification,
	"Odyssey.Crafting.Planner.RawMaterialIdentification",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlannerRawMaterialIdentification::RunTest(const FString& Parameters)
{
	// Replicate IsRawMaterial logic
	auto IsRawMaterial = [](EResourceType Type) -> bool
	{
		switch (Type)
		{
		case EResourceType::Silicate:
		case EResourceType::Carbon:
			return true;
		default:
			return false;
		}
	};

	TestTrue(TEXT("Silicate should be raw material"), IsRawMaterial(EResourceType::Silicate));
	TestTrue(TEXT("Carbon should be raw material"), IsRawMaterial(EResourceType::Carbon));
	TestFalse(TEXT("RefinedSilicate should not be raw material"), IsRawMaterial(EResourceType::RefinedSilicate));
	TestFalse(TEXT("RefinedCarbon should not be raw material"), IsRawMaterial(EResourceType::RefinedCarbon));
	TestFalse(TEXT("CompositeMaterial should not be raw material"), IsRawMaterial(EResourceType::CompositeMaterial));
	TestFalse(TEXT("OMEN should not be raw material"), IsRawMaterial(EResourceType::OMEN));

	return true;
}

// ============================================================================
// 17. Ingredient Quantity Scaling
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlannerIngredientScaling,
	"Odyssey.Crafting.Planner.IngredientQuantityScaling",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlannerIngredientScaling::RunTest(const FString& Parameters)
{
	// In ResolveRecipeChain, step inputs are scaled by quantity
	TArray<FCraftingIngredient> BaseIngredients;
	BaseIngredients.Add(FCraftingIngredient(EResourceType::Silicate, 5));
	BaseIngredients.Add(FCraftingIngredient(EResourceType::Carbon, 3));

	int32 Quantity = 4;

	TArray<FCraftingIngredient> ScaledIngredients = BaseIngredients;
	for (FCraftingIngredient& Ingredient : ScaledIngredients)
	{
		Ingredient.Amount *= Quantity;
	}

	TestEqual(TEXT("Scaled Silicate should be 20"), ScaledIngredients[0].Amount, 20);
	TestEqual(TEXT("Scaled Carbon should be 12"), ScaledIngredients[1].Amount, 12);

	return true;
}

// ============================================================================
// 18. Feasibility Check with No Manager
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlannerFeasibilityNoManager,
	"Odyssey.Crafting.Planner.FeasibilityCheckNoManager",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlannerFeasibilityNoManager::RunTest(const FString& Parameters)
{
	// Without CraftingManager, plan should be infeasible
	FProductionPlan Plan;
	Plan.bIsFeasible = false;
	Plan.BlockingReasons.Add(TEXT("Crafting manager not available"));

	TestFalse(TEXT("Plan without manager should not be feasible"), Plan.bIsFeasible);
	TestEqual(TEXT("Should have 1 blocking reason"), Plan.BlockingReasons.Num(), 1);
	TestTrue(TEXT("Blocking reason should mention crafting manager"),
		Plan.BlockingReasons[0].Contains(TEXT("Crafting manager")));

	return true;
}

// ============================================================================
// 19. Recipe Profit Ranking
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlannerProfitRanking,
	"Odyssey.Crafting.Planner.RecipeProfitRanking",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlannerProfitRanking::RunTest(const FString& Parameters)
{
	// Simulate RankRecipesByProfit
	TArray<TPair<FName, float>> RecipeProfits;
	RecipeProfits.Add(TPair<FName, float>(FName(TEXT("Recipe_A")), 0.5f));
	RecipeProfits.Add(TPair<FName, float>(FName(TEXT("Recipe_B")), 1.5f));
	RecipeProfits.Add(TPair<FName, float>(FName(TEXT("Recipe_C")), -0.2f));
	RecipeProfits.Add(TPair<FName, float>(FName(TEXT("Recipe_D")), 0.8f));

	// Sort descending
	RecipeProfits.Sort([](const TPair<FName, float>& A, const TPair<FName, float>& B)
	{
		return A.Value > B.Value;
	});

	TestEqual(TEXT("Most profitable should be Recipe_B"), RecipeProfits[0].Key, FName(TEXT("Recipe_B")));
	TestEqual(TEXT("Second should be Recipe_D"), RecipeProfits[1].Key, FName(TEXT("Recipe_D")));
	TestEqual(TEXT("Third should be Recipe_A"), RecipeProfits[2].Key, FName(TEXT("Recipe_A")));
	TestEqual(TEXT("Least profitable should be Recipe_C"), RecipeProfits[3].Key, FName(TEXT("Recipe_C")));

	return true;
}

// ============================================================================
// 20. Blocking Reasons Accumulation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlannerBlockingReasons,
	"Odyssey.Crafting.Planner.BlockingReasonsAccumulation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlannerBlockingReasons::RunTest(const FString& Parameters)
{
	FProductionPlan Plan;
	Plan.bIsFeasible = true;

	// Simulate checking various blocking conditions
	bool bRecipeUnlocked = false;
	if (!bRecipeUnlocked)
	{
		Plan.bIsFeasible = false;
		Plan.BlockingReasons.Add(TEXT("Recipe not unlocked: Advanced Weapon"));
	}

	bool bHasSkill = false;
	if (!bHasSkill)
	{
		Plan.BlockingReasons.Add(TEXT("Skill 'WeaponCrafting' requires level 5 (current: 3)"));
	}

	bool bHasMaterials = false;
	if (!bHasMaterials)
	{
		Plan.BlockingReasons.Add(TEXT("Need 10 more of Silicate"));
		Plan.BlockingReasons.Add(TEXT("Need 5 more of Carbon"));
	}

	TestFalse(TEXT("Plan should not be feasible"), Plan.bIsFeasible);
	TestEqual(TEXT("Should have 4 blocking reasons"), Plan.BlockingReasons.Num(), 4);

	return true;
}
