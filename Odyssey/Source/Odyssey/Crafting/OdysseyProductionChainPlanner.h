// OdysseyProductionChainPlanner.h
// Production chain planning and visualization for multi-step crafting
// Calculates optimal production paths, resource requirements, and bottleneck predictions

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OdysseyCraftingManager.h"
#include "OdysseyProductionChainPlanner.generated.h"

// Forward declarations
class UOdysseyCraftingManager;
class UOdysseyAutomationNetworkSystem;
class UOdysseyInventoryComponent;

/**
 * Production step in a planned chain
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FProductionStep
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Step")
	FName RecipeID;

	UPROPERTY(BlueprintReadOnly, Category = "Step")
	FString RecipeName;

	UPROPERTY(BlueprintReadOnly, Category = "Step")
	int32 Quantity;

	UPROPERTY(BlueprintReadOnly, Category = "Step")
	int32 Depth;

	UPROPERTY(BlueprintReadOnly, Category = "Step")
	TArray<FCraftingIngredient> RequiredInputs;

	UPROPERTY(BlueprintReadOnly, Category = "Step")
	TArray<FCraftingOutput> Outputs;

	UPROPERTY(BlueprintReadOnly, Category = "Step")
	float EstimatedTime;

	UPROPERTY(BlueprintReadOnly, Category = "Step")
	int32 EstimatedEnergyCost;

	UPROPERTY(BlueprintReadOnly, Category = "Step")
	bool bCanCraftNow;

	UPROPERTY(BlueprintReadOnly, Category = "Step")
	bool bHasAllPrerequisites;

	UPROPERTY(BlueprintReadOnly, Category = "Step")
	TArray<FName> DependsOnSteps;

	FProductionStep()
	{
		RecipeID = NAME_None;
		Quantity = 1;
		Depth = 0;
		EstimatedTime = 0.0f;
		EstimatedEnergyCost = 0;
		bCanCraftNow = false;
		bHasAllPrerequisites = false;
	}
};

/**
 * Complete production plan for crafting a target item
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FProductionPlan
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Plan")
	FGuid PlanID;

	UPROPERTY(BlueprintReadOnly, Category = "Plan")
	FName TargetRecipeID;

	UPROPERTY(BlueprintReadOnly, Category = "Plan")
	int32 TargetQuantity;

	UPROPERTY(BlueprintReadOnly, Category = "Plan")
	TArray<FProductionStep> Steps;

	UPROPERTY(BlueprintReadOnly, Category = "Plan")
	TArray<FCraftingIngredient> TotalRawMaterialsNeeded;

	UPROPERTY(BlueprintReadOnly, Category = "Plan")
	TArray<FCraftingIngredient> MaterialsAlreadyOwned;

	UPROPERTY(BlueprintReadOnly, Category = "Plan")
	TArray<FCraftingIngredient> MaterialsStillNeeded;

	UPROPERTY(BlueprintReadOnly, Category = "Plan")
	float TotalEstimatedTime;

	UPROPERTY(BlueprintReadOnly, Category = "Plan")
	int32 TotalEstimatedEnergyCost;

	UPROPERTY(BlueprintReadOnly, Category = "Plan")
	int32 TotalSteps;

	UPROPERTY(BlueprintReadOnly, Category = "Plan")
	int32 MaxDepth;

	UPROPERTY(BlueprintReadOnly, Category = "Plan")
	bool bIsFeasible;

	UPROPERTY(BlueprintReadOnly, Category = "Plan")
	TArray<FString> BlockingReasons;

	FProductionPlan()
	{
		PlanID = FGuid::NewGuid();
		TargetRecipeID = NAME_None;
		TargetQuantity = 1;
		TotalEstimatedTime = 0.0f;
		TotalEstimatedEnergyCost = 0;
		TotalSteps = 0;
		MaxDepth = 0;
		bIsFeasible = false;
	}
};

/**
 * Cost breakdown for economic analysis
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FProductionCostBreakdown
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Cost")
	int32 TotalMaterialCost;

	UPROPERTY(BlueprintReadOnly, Category = "Cost")
	int32 TotalEnergyCost;

	UPROPERTY(BlueprintReadOnly, Category = "Cost")
	float TotalTimeCost;

	UPROPERTY(BlueprintReadOnly, Category = "Cost")
	int32 EstimatedOutputValue;

	UPROPERTY(BlueprintReadOnly, Category = "Cost")
	float ProfitMargin;

	UPROPERTY(BlueprintReadOnly, Category = "Cost")
	TMap<EResourceType, int32> MaterialCostByType;

	FProductionCostBreakdown()
	{
		TotalMaterialCost = 0;
		TotalEnergyCost = 0;
		TotalTimeCost = 0.0f;
		EstimatedOutputValue = 0;
		ProfitMargin = 0.0f;
	}
};

// Delegate declarations
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProductionPlanCreated, FGuid, PlanID, FName, TargetRecipe);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnProductionStepCompleted, FGuid, PlanID, int32, StepIndex, bool, bSuccess);

/**
 * Production Chain Planner
 *
 * Analyzes recipe dependency graphs and generates optimal production plans:
 * - Recursive chain resolution from target product to raw materials
 * - Inventory-aware planning (skips steps for items already owned)
 * - Time and cost estimation for complete chains
 * - Feasibility checking against skill/facility requirements
 * - Economic profit/loss analysis per production chain
 */
UCLASS(ClassGroup=(Odyssey), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UOdysseyProductionChainPlanner : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdysseyProductionChainPlanner();

protected:
	virtual void BeginPlay() override;

public:
	// ============================================================================
	// Plan Generation
	// ============================================================================

	/**
	 * Generate a complete production plan for a target recipe
	 * @param TargetRecipeID Final product to produce
	 * @param Quantity Number of final items desired
	 * @param bAccountForInventory If true, subtract owned materials from plan
	 * @return Complete production plan with all intermediate steps
	 */
	UFUNCTION(BlueprintCallable, Category = "Planning")
	FProductionPlan GenerateProductionPlan(FName TargetRecipeID, int32 Quantity = 1, bool bAccountForInventory = true);

	/**
	 * Generate multiple alternative plans and return the best one
	 * Plans differ by using recipe variations and alternative inputs
	 */
	UFUNCTION(BlueprintCallable, Category = "Planning")
	FProductionPlan GenerateOptimalPlan(FName TargetRecipeID, int32 Quantity = 1);

	/**
	 * Check feasibility of producing a target item
	 */
	UFUNCTION(BlueprintCallable, Category = "Planning")
	bool IsPlanFeasible(FName TargetRecipeID, int32 Quantity = 1) const;

	/**
	 * Get blocking reasons preventing production
	 */
	UFUNCTION(BlueprintCallable, Category = "Planning")
	TArray<FString> GetBlockingReasons(FName TargetRecipeID, int32 Quantity = 1) const;

	// ============================================================================
	// Cost Analysis
	// ============================================================================

	/**
	 * Calculate complete cost breakdown for a production chain
	 */
	UFUNCTION(BlueprintCallable, Category = "Cost Analysis")
	FProductionCostBreakdown CalculateCostBreakdown(FName TargetRecipeID, int32 Quantity = 1) const;

	/**
	 * Estimate profit margin for crafting and selling
	 */
	UFUNCTION(BlueprintCallable, Category = "Cost Analysis")
	float EstimateProfitMargin(FName TargetRecipeID, int32 Quantity = 1) const;

	/**
	 * Compare profitability of multiple recipes
	 */
	UFUNCTION(BlueprintCallable, Category = "Cost Analysis")
	TArray<FName> RankRecipesByProfit(const TArray<FName>& RecipeIDs) const;

	// ============================================================================
	// Chain Information
	// ============================================================================

	/**
	 * Get all raw materials needed in the entire chain
	 */
	UFUNCTION(BlueprintCallable, Category = "Chain Info")
	TArray<FCraftingIngredient> GetRawMaterialRequirements(FName TargetRecipeID, int32 Quantity = 1) const;

	/**
	 * Get depth of production chain (number of intermediate steps)
	 */
	UFUNCTION(BlueprintCallable, Category = "Chain Info")
	int32 GetChainDepth(FName TargetRecipeID) const;

	/**
	 * Get all intermediate products in a chain
	 */
	UFUNCTION(BlueprintCallable, Category = "Chain Info")
	TArray<FName> GetIntermediateProducts(FName TargetRecipeID) const;

	/**
	 * Get total estimated crafting time for entire chain
	 */
	UFUNCTION(BlueprintCallable, Category = "Chain Info")
	float GetTotalChainTime(FName TargetRecipeID, int32 Quantity = 1) const;

	// ============================================================================
	// Plan Execution
	// ============================================================================

	/**
	 * Start executing a production plan step by step
	 */
	UFUNCTION(BlueprintCallable, Category = "Execution")
	bool StartPlanExecution(FGuid PlanID);

	/**
	 * Get current execution status of a plan
	 */
	UFUNCTION(BlueprintCallable, Category = "Execution")
	int32 GetPlanExecutionProgress(FGuid PlanID) const;

	/**
	 * Cancel plan execution
	 */
	UFUNCTION(BlueprintCallable, Category = "Execution")
	bool CancelPlanExecution(FGuid PlanID);

	/**
	 * Get active plans
	 */
	UFUNCTION(BlueprintCallable, Category = "Execution")
	TArray<FProductionPlan> GetActivePlans() const;

	// ============================================================================
	// Integration
	// ============================================================================

	UFUNCTION(BlueprintCallable, Category = "Integration")
	void SetCraftingManager(UOdysseyCraftingManager* NewManager);

	UFUNCTION(BlueprintCallable, Category = "Integration")
	void SetInventoryComponent(UOdysseyInventoryComponent* NewInventory);

	// ============================================================================
	// Events
	// ============================================================================

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnProductionPlanCreated OnProductionPlanCreated;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnProductionStepCompleted OnProductionStepCompleted;

protected:
	// ============================================================================
	// Configuration
	// ============================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	int32 MaxChainDepth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	int32 MaxPlanCacheSize;

	// ============================================================================
	// Runtime State
	// ============================================================================

	UPROPERTY()
	TMap<FGuid, FProductionPlan> CachedPlans;

	UPROPERTY()
	TMap<FGuid, int32> ActivePlanProgress; // PlanID -> current step index

	// ============================================================================
	// Component References
	// ============================================================================

	UPROPERTY()
	UOdysseyCraftingManager* CraftingManager;

	UPROPERTY()
	UOdysseyInventoryComponent* InventoryComponent;

	// ============================================================================
	// Internal Methods
	// ============================================================================

	/**
	 * Recursively resolve a recipe into production steps
	 */
	void ResolveRecipeChain(FName RecipeID, int32 Quantity, int32 CurrentDepth,
		TArray<FProductionStep>& OutSteps, TSet<FName>& VisitedRecipes) const;

	/**
	 * Calculate raw material totals from a set of steps
	 */
	TArray<FCraftingIngredient> AggregateRawMaterials(const TArray<FProductionStep>& Steps) const;

	/**
	 * Subtract owned inventory from material requirements
	 */
	TArray<FCraftingIngredient> SubtractInventory(const TArray<FCraftingIngredient>& Requirements) const;

	/**
	 * Check if a recipe represents a raw material (no crafting recipe produces it)
	 */
	bool IsRawMaterial(EResourceType ResourceType) const;
};
