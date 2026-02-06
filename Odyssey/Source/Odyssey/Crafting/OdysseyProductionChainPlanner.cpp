// OdysseyProductionChainPlanner.cpp
// Implementation of production chain planning and cost analysis
// Enables players to understand and optimize multi-step crafting pipelines

#include "OdysseyProductionChainPlanner.h"
#include "OdysseyCraftingManager.h"
#include "OdysseyAutomationNetworkSystem.h"
#include "OdysseyInventoryComponent.h"

UOdysseyProductionChainPlanner::UOdysseyProductionChainPlanner()
{
	PrimaryComponentTick.bCanEverTick = false;

	MaxChainDepth = 10;
	MaxPlanCacheSize = 20;

	CraftingManager = nullptr;
	InventoryComponent = nullptr;
}

void UOdysseyProductionChainPlanner::BeginPlay()
{
	Super::BeginPlay();

	// Auto-find components
	AActor* Owner = GetOwner();
	if (Owner)
	{
		if (!CraftingManager)
		{
			CraftingManager = Owner->FindComponentByClass<UOdysseyCraftingManager>();
		}
		if (!InventoryComponent)
		{
			InventoryComponent = Owner->FindComponentByClass<UOdysseyInventoryComponent>();
		}
	}
}

// ============================================================================
// Plan Generation
// ============================================================================

FProductionPlan UOdysseyProductionChainPlanner::GenerateProductionPlan(FName TargetRecipeID, int32 Quantity, bool bAccountForInventory)
{
	FProductionPlan Plan;
	Plan.PlanID = FGuid::NewGuid();
	Plan.TargetRecipeID = TargetRecipeID;
	Plan.TargetQuantity = Quantity;

	if (!CraftingManager)
	{
		Plan.bIsFeasible = false;
		Plan.BlockingReasons.Add(TEXT("Crafting manager not available"));
		return Plan;
	}

	FAdvancedCraftingRecipe TargetRecipe = CraftingManager->GetRecipe(TargetRecipeID);
	if (TargetRecipe.RecipeID.IsNone())
	{
		Plan.bIsFeasible = false;
		Plan.BlockingReasons.Add(FString::Printf(TEXT("Recipe not found: %s"), *TargetRecipeID.ToString()));
		return Plan;
	}

	// Resolve the full production chain recursively
	TSet<FName> Visited;
	ResolveRecipeChain(TargetRecipeID, Quantity, 0, Plan.Steps, Visited);

	// Calculate aggregated raw materials
	Plan.TotalRawMaterialsNeeded = AggregateRawMaterials(Plan.Steps);

	// Account for inventory
	if (bAccountForInventory && InventoryComponent)
	{
		Plan.MaterialsAlreadyOwned.Empty();
		Plan.MaterialsStillNeeded.Empty();

		for (const FCraftingIngredient& Req : Plan.TotalRawMaterialsNeeded)
		{
			int32 Owned = InventoryComponent->GetResourceAmount(Req.ResourceType);
			int32 Needed = Req.Amount;

			FCraftingIngredient OwnedEntry;
			OwnedEntry.ResourceType = Req.ResourceType;
			OwnedEntry.Amount = FMath::Min(Owned, Needed);
			Plan.MaterialsAlreadyOwned.Add(OwnedEntry);

			int32 StillNeeded = Needed - Owned;
			if (StillNeeded > 0)
			{
				FCraftingIngredient NeededEntry;
				NeededEntry.ResourceType = Req.ResourceType;
				NeededEntry.Amount = StillNeeded;
				Plan.MaterialsStillNeeded.Add(NeededEntry);
			}
		}
	}
	else
	{
		Plan.MaterialsStillNeeded = Plan.TotalRawMaterialsNeeded;
	}

	// Calculate totals
	Plan.TotalSteps = Plan.Steps.Num();
	Plan.TotalEstimatedTime = 0.0f;
	Plan.TotalEstimatedEnergyCost = 0;
	Plan.MaxDepth = 0;

	for (const FProductionStep& Step : Plan.Steps)
	{
		Plan.TotalEstimatedTime += Step.EstimatedTime;
		Plan.TotalEstimatedEnergyCost += Step.EstimatedEnergyCost;
		Plan.MaxDepth = FMath::Max(Plan.MaxDepth, Step.Depth);
	}

	// Check feasibility
	Plan.bIsFeasible = true;

	// Check recipe unlock status
	if (!CraftingManager->IsRecipeUnlocked(TargetRecipeID))
	{
		Plan.bIsFeasible = false;
		Plan.BlockingReasons.Add(FString::Printf(TEXT("Recipe not unlocked: %s"), *TargetRecipe.RecipeName));
	}

	// Check all intermediate recipe unlocks
	for (const FProductionStep& Step : Plan.Steps)
	{
		if (!CraftingManager->IsRecipeUnlocked(Step.RecipeID))
		{
			Plan.bIsFeasible = false;
			Plan.BlockingReasons.Add(FString::Printf(TEXT("Required recipe not unlocked: %s"), *Step.RecipeName));
		}
	}

	// Check material availability
	if (Plan.MaterialsStillNeeded.Num() > 0)
	{
		for (const FCraftingIngredient& Needed : Plan.MaterialsStillNeeded)
		{
			Plan.BlockingReasons.Add(FString::Printf(TEXT("Need %d more of resource type %d"),
				Needed.Amount, static_cast<int32>(Needed.ResourceType)));
		}
	}

	// Cache the plan
	if (CachedPlans.Num() >= MaxPlanCacheSize)
	{
		// Remove oldest cached plan
		FGuid OldestKey;
		for (const auto& Pair : CachedPlans)
		{
			OldestKey = Pair.Key;
			break;
		}
		CachedPlans.Remove(OldestKey);
	}
	CachedPlans.Add(Plan.PlanID, Plan);

	OnProductionPlanCreated.Broadcast(Plan.PlanID, TargetRecipeID);

	return Plan;
}

FProductionPlan UOdysseyProductionChainPlanner::GenerateOptimalPlan(FName TargetRecipeID, int32 Quantity)
{
	// Generate the standard plan
	FProductionPlan BestPlan = GenerateProductionPlan(TargetRecipeID, Quantity, true);

	// In a full implementation, this would generate plans using recipe variations
	// and compare total cost/time to find the optimal path.
	// For now, return the standard plan.

	return BestPlan;
}

bool UOdysseyProductionChainPlanner::IsPlanFeasible(FName TargetRecipeID, int32 Quantity) const
{
	if (!CraftingManager)
	{
		return false;
	}

	FAdvancedCraftingRecipe Recipe = CraftingManager->GetRecipe(TargetRecipeID);
	if (Recipe.RecipeID.IsNone())
	{
		return false;
	}

	if (!CraftingManager->IsRecipeUnlocked(TargetRecipeID))
	{
		return false;
	}

	return true;
}

TArray<FString> UOdysseyProductionChainPlanner::GetBlockingReasons(FName TargetRecipeID, int32 Quantity) const
{
	TArray<FString> Reasons;

	if (!CraftingManager)
	{
		Reasons.Add(TEXT("Crafting system not available"));
		return Reasons;
	}

	FAdvancedCraftingRecipe Recipe = CraftingManager->GetRecipe(TargetRecipeID);
	if (Recipe.RecipeID.IsNone())
	{
		Reasons.Add(FString::Printf(TEXT("Unknown recipe: %s"), *TargetRecipeID.ToString()));
		return Reasons;
	}

	if (!CraftingManager->IsRecipeUnlocked(TargetRecipeID))
	{
		Reasons.Add(FString::Printf(TEXT("Recipe '%s' is not unlocked"), *Recipe.RecipeName));
	}

	// Check skill requirements
	UOdysseyCraftingSkillSystem* SkillSystem = CraftingManager->GetSkillSystem();
	if (SkillSystem)
	{
		for (const auto& SkillReq : Recipe.RequiredSkillLevels)
		{
			int32 CurrentLevel = SkillSystem->GetSkillLevel(SkillReq.Key);
			if (CurrentLevel < SkillReq.Value)
			{
				Reasons.Add(FString::Printf(TEXT("Skill '%s' requires level %d (current: %d)"),
					*SkillReq.Key.ToString(), SkillReq.Value, CurrentLevel));
			}
		}
	}

	// Check materials
	if (InventoryComponent)
	{
		for (const FCraftingIngredient& Ingredient : Recipe.PrimaryIngredients)
		{
			int32 Required = Ingredient.Amount * Quantity;
			int32 Available = InventoryComponent->GetResourceAmount(Ingredient.ResourceType);
			if (Available < Required)
			{
				Reasons.Add(FString::Printf(TEXT("Need %d more of resource type %d (have %d, need %d)"),
					Required - Available, static_cast<int32>(Ingredient.ResourceType), Available, Required));
			}
		}
	}

	return Reasons;
}

// ============================================================================
// Cost Analysis
// ============================================================================

FProductionCostBreakdown UOdysseyProductionChainPlanner::CalculateCostBreakdown(FName TargetRecipeID, int32 Quantity) const
{
	FProductionCostBreakdown Breakdown;

	if (!CraftingManager || !InventoryComponent)
	{
		return Breakdown;
	}

	// Get raw material requirements
	TArray<FCraftingIngredient> RawMaterials = GetRawMaterialRequirements(TargetRecipeID, Quantity);

	int32 TotalMaterialCost = 0;
	for (const FCraftingIngredient& Material : RawMaterials)
	{
		int32 Value = InventoryComponent->GetResourceValue(Material.ResourceType);
		int32 Cost = Value * Material.Amount;
		TotalMaterialCost += Cost;
		Breakdown.MaterialCostByType.Add(Material.ResourceType, Cost);
	}

	Breakdown.TotalMaterialCost = TotalMaterialCost;
	Breakdown.TotalTimeCost = GetTotalChainTime(TargetRecipeID, Quantity);

	// Estimate energy cost
	FAdvancedCraftingRecipe Recipe = CraftingManager->GetRecipe(TargetRecipeID);
	Breakdown.TotalEnergyCost = Recipe.EnergyCost * Quantity;

	// Estimate output value (using base resource values)
	int32 OutputValue = 0;
	for (const FCraftingOutput& Output : Recipe.PrimaryOutputs)
	{
		OutputValue += InventoryComponent->GetResourceValue(Output.ResourceType) * Output.Amount * Quantity;
	}
	Breakdown.EstimatedOutputValue = OutputValue;

	// Calculate profit margin
	int32 TotalCost = TotalMaterialCost + Breakdown.TotalEnergyCost;
	if (TotalCost > 0)
	{
		Breakdown.ProfitMargin = static_cast<float>(OutputValue - TotalCost) / TotalCost;
	}

	return Breakdown;
}

float UOdysseyProductionChainPlanner::EstimateProfitMargin(FName TargetRecipeID, int32 Quantity) const
{
	FProductionCostBreakdown Breakdown = CalculateCostBreakdown(TargetRecipeID, Quantity);
	return Breakdown.ProfitMargin;
}

TArray<FName> UOdysseyProductionChainPlanner::RankRecipesByProfit(const TArray<FName>& RecipeIDs) const
{
	// Calculate profit for each recipe
	TArray<TPair<FName, float>> RecipeProfits;
	for (const FName& RecipeID : RecipeIDs)
	{
		float Profit = EstimateProfitMargin(RecipeID, 1);
		RecipeProfits.Add(TPair<FName, float>(RecipeID, Profit));
	}

	// Sort by profit descending
	RecipeProfits.Sort([](const TPair<FName, float>& A, const TPair<FName, float>& B)
	{
		return A.Value > B.Value;
	});

	// Extract sorted names
	TArray<FName> Ranked;
	for (const auto& Pair : RecipeProfits)
	{
		Ranked.Add(Pair.Key);
	}

	return Ranked;
}

// ============================================================================
// Chain Information
// ============================================================================

TArray<FCraftingIngredient> UOdysseyProductionChainPlanner::GetRawMaterialRequirements(FName TargetRecipeID, int32 Quantity) const
{
	if (!CraftingManager)
	{
		return TArray<FCraftingIngredient>();
	}

	return CraftingManager->CalculateChainMaterials(TargetRecipeID, Quantity);
}

int32 UOdysseyProductionChainPlanner::GetChainDepth(FName TargetRecipeID) const
{
	if (!CraftingManager)
	{
		return 0;
	}

	TArray<FName> Chain = CraftingManager->GetProductionChain(TargetRecipeID);
	return Chain.Num();
}

TArray<FName> UOdysseyProductionChainPlanner::GetIntermediateProducts(FName TargetRecipeID) const
{
	if (!CraftingManager)
	{
		return TArray<FName>();
	}

	TArray<FName> Chain = CraftingManager->GetProductionChain(TargetRecipeID);

	// Remove the final recipe (it is the target, not an intermediate)
	if (Chain.Num() > 0 && Chain.Last() == TargetRecipeID)
	{
		Chain.RemoveAt(Chain.Num() - 1);
	}

	return Chain;
}

float UOdysseyProductionChainPlanner::GetTotalChainTime(FName TargetRecipeID, int32 Quantity) const
{
	if (!CraftingManager)
	{
		return 0.0f;
	}

	TArray<FName> Chain = CraftingManager->GetProductionChain(TargetRecipeID);
	float TotalTime = 0.0f;

	for (const FName& StepRecipeID : Chain)
	{
		int32 StepQuantity = (StepRecipeID == TargetRecipeID) ? Quantity : 1;
		TotalTime += CraftingManager->CalculateCraftingTime(StepRecipeID, StepQuantity);
	}

	return TotalTime;
}

// ============================================================================
// Plan Execution
// ============================================================================

bool UOdysseyProductionChainPlanner::StartPlanExecution(FGuid PlanID)
{
	FProductionPlan* Plan = CachedPlans.Find(PlanID);
	if (!Plan)
	{
		return false;
	}

	if (!Plan->bIsFeasible)
	{
		return false;
	}

	// Initialize execution tracking
	ActivePlanProgress.Add(PlanID, 0);

	// Start the first step
	if (Plan->Steps.Num() > 0 && CraftingManager)
	{
		const FProductionStep& FirstStep = Plan->Steps[0];
		CraftingManager->StartCraftingJob(FirstStep.RecipeID, FirstStep.Quantity);
	}

	return true;
}

int32 UOdysseyProductionChainPlanner::GetPlanExecutionProgress(FGuid PlanID) const
{
	const int32* Progress = ActivePlanProgress.Find(PlanID);
	return Progress ? *Progress : -1;
}

bool UOdysseyProductionChainPlanner::CancelPlanExecution(FGuid PlanID)
{
	return ActivePlanProgress.Remove(PlanID) > 0;
}

TArray<FProductionPlan> UOdysseyProductionChainPlanner::GetActivePlans() const
{
	TArray<FProductionPlan> Active;
	for (const auto& ProgressPair : ActivePlanProgress)
	{
		const FProductionPlan* Plan = CachedPlans.Find(ProgressPair.Key);
		if (Plan)
		{
			Active.Add(*Plan);
		}
	}
	return Active;
}

// ============================================================================
// Integration
// ============================================================================

void UOdysseyProductionChainPlanner::SetCraftingManager(UOdysseyCraftingManager* NewManager)
{
	CraftingManager = NewManager;
}

void UOdysseyProductionChainPlanner::SetInventoryComponent(UOdysseyInventoryComponent* NewInventory)
{
	InventoryComponent = NewInventory;
}

// ============================================================================
// Internal Methods
// ============================================================================

void UOdysseyProductionChainPlanner::ResolveRecipeChain(FName RecipeID, int32 Quantity, int32 CurrentDepth,
	TArray<FProductionStep>& OutSteps, TSet<FName>& VisitedRecipes) const
{
	if (CurrentDepth > MaxChainDepth)
	{
		return; // Prevent infinite recursion
	}

	if (VisitedRecipes.Contains(RecipeID))
	{
		return; // Avoid circular dependencies
	}

	if (!CraftingManager)
	{
		return;
	}

	FAdvancedCraftingRecipe Recipe = CraftingManager->GetRecipe(RecipeID);
	if (Recipe.RecipeID.IsNone())
	{
		return;
	}

	VisitedRecipes.Add(RecipeID);

	// First, resolve prerequisites (sub-recipes needed for ingredients)
	for (const FName& PrereqID : Recipe.PrerequisiteRecipes)
	{
		ResolveRecipeChain(PrereqID, 1, CurrentDepth + 1, OutSteps, VisitedRecipes);
	}

	// Create production step
	FProductionStep Step;
	Step.RecipeID = RecipeID;
	Step.RecipeName = Recipe.RecipeName;
	Step.Quantity = Quantity;
	Step.Depth = CurrentDepth;
	Step.RequiredInputs = Recipe.PrimaryIngredients;
	Step.Outputs = Recipe.PrimaryOutputs;
	Step.EstimatedTime = CraftingManager->CalculateCraftingTime(RecipeID, Quantity);
	Step.EstimatedEnergyCost = CraftingManager->CalculateEnergyCost(RecipeID, Quantity);
	Step.bCanCraftNow = CraftingManager->CanCraftRecipe(RecipeID, Quantity);
	Step.bHasAllPrerequisites = CraftingManager->IsRecipeUnlocked(RecipeID);
	Step.DependsOnSteps = Recipe.PrerequisiteRecipes;

	// Scale ingredient amounts by quantity
	for (FCraftingIngredient& Input : Step.RequiredInputs)
	{
		Input.Amount *= Quantity;
	}

	OutSteps.Add(Step);
}

TArray<FCraftingIngredient> UOdysseyProductionChainPlanner::AggregateRawMaterials(const TArray<FProductionStep>& Steps) const
{
	TMap<EResourceType, int32> MaterialTotals;

	for (const FProductionStep& Step : Steps)
	{
		for (const FCraftingIngredient& Input : Step.RequiredInputs)
		{
			// Only count materials that are raw (not produced by another step in the chain)
			bool bProducedInChain = false;
			for (const FProductionStep& OtherStep : Steps)
			{
				for (const FCraftingOutput& Output : OtherStep.Outputs)
				{
					if (Output.ResourceType == Input.ResourceType)
					{
						bProducedInChain = true;
						break;
					}
				}
				if (bProducedInChain) break;
			}

			if (!bProducedInChain || IsRawMaterial(Input.ResourceType))
			{
				int32& Total = MaterialTotals.FindOrAdd(Input.ResourceType);
				Total += Input.Amount;
			}
		}
	}

	// Convert to array
	TArray<FCraftingIngredient> Result;
	for (const auto& Pair : MaterialTotals)
	{
		FCraftingIngredient Ingredient;
		Ingredient.ResourceType = Pair.Key;
		Ingredient.Amount = Pair.Value;
		Result.Add(Ingredient);
	}

	return Result;
}

TArray<FCraftingIngredient> UOdysseyProductionChainPlanner::SubtractInventory(const TArray<FCraftingIngredient>& Requirements) const
{
	TArray<FCraftingIngredient> Remaining;

	if (!InventoryComponent)
	{
		return Requirements;
	}

	for (const FCraftingIngredient& Req : Requirements)
	{
		int32 Available = InventoryComponent->GetResourceAmount(Req.ResourceType);
		int32 StillNeeded = Req.Amount - Available;

		if (StillNeeded > 0)
		{
			FCraftingIngredient NeededIngredient;
			NeededIngredient.ResourceType = Req.ResourceType;
			NeededIngredient.Amount = StillNeeded;
			Remaining.Add(NeededIngredient);
		}
	}

	return Remaining;
}

bool UOdysseyProductionChainPlanner::IsRawMaterial(EResourceType ResourceType) const
{
	// Raw materials are the base resource types that are gathered, not crafted
	switch (ResourceType)
	{
	case EResourceType::Silicate:
	case EResourceType::Carbon:
		return true;
	default:
		return false;
	}
}
