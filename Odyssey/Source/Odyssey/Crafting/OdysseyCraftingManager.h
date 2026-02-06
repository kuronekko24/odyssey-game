// OdysseyCraftingManager.h
// Master crafting controller for the Advanced Crafting & Manufacturing System
// Oversees all crafting operations, manages queues, and coordinates with economy/skill systems

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "OdysseyInventoryComponent.h"
#include "OdysseyCraftingComponent.h"
#include "OdysseyCraftingManager.generated.h"

// Forward declarations
class UOdysseyCraftingRecipeComponent;
class UOdysseyAutomationNetworkSystem;
class UOdysseyQualityControlSystem;
class UOdysseyCraftingSkillSystem;
class UOdysseyTradingComponent;

/**
 * Item quality tier affecting value and performance
 */
UENUM(BlueprintType)
enum class EItemQuality : uint8
{
	Scrap = 0       UMETA(DisplayName = "Scrap"),
	Common = 1      UMETA(DisplayName = "Common"),
	Standard = 2    UMETA(DisplayName = "Standard"),
	Quality = 3     UMETA(DisplayName = "Quality"),
	Superior = 4    UMETA(DisplayName = "Superior"),
	Masterwork = 5  UMETA(DisplayName = "Masterwork"),
	Legendary = 6   UMETA(DisplayName = "Legendary")
};

/**
 * Crafting station tier affecting available recipes and bonuses
 */
UENUM(BlueprintType)
enum class ECraftingTier : uint8
{
	Primitive = 0   UMETA(DisplayName = "Primitive"),
	Basic = 1       UMETA(DisplayName = "Basic"),
	Advanced = 2    UMETA(DisplayName = "Advanced"),
	Industrial = 3  UMETA(DisplayName = "Industrial"),
	Automated = 4   UMETA(DisplayName = "Automated"),
	Quantum = 5     UMETA(DisplayName = "Quantum")
};

/**
 * Item category for recipe organization and equipment slots
 */
UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	RawMaterial = 0     UMETA(DisplayName = "Raw Material"),
	ProcessedMaterial = 1 UMETA(DisplayName = "Processed Material"),
	Component = 2       UMETA(DisplayName = "Component"),
	Equipment = 3       UMETA(DisplayName = "Equipment"),
	Weapon = 4          UMETA(DisplayName = "Weapon"),
	Ammunition = 5      UMETA(DisplayName = "Ammunition"),
	ShipModule = 6      UMETA(DisplayName = "Ship Module"),
	Consumable = 7      UMETA(DisplayName = "Consumable"),
	Blueprint = 8       UMETA(DisplayName = "Blueprint")
};

/**
 * Extended item definition with quality support
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FCraftedItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EResourceType ResourceType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemCategory Category;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemQuality Quality;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 Quantity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float Durability;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float QualityMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName CrafterID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FDateTime CraftedTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TMap<FName, float> StatModifiers;

	FCraftedItem()
	{
		ItemID = NAME_None;
		ResourceType = EResourceType::None;
		Category = EItemCategory::RawMaterial;
		Quality = EItemQuality::Common;
		Quantity = 1;
		Durability = 100.0f;
		QualityMultiplier = 1.0f;
		CrafterID = NAME_None;
		CraftedTime = FDateTime::Now();
	}
};

/**
 * Advanced recipe with production chain support
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FAdvancedCraftingRecipe : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe Info")
	FName RecipeID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe Info")
	FString RecipeName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe Info")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe Info")
	EItemCategory OutputCategory;

	// Input requirements with alternative options
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs")
	TArray<FCraftingIngredient> PrimaryIngredients;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs")
	TArray<FCraftingIngredient> OptionalIngredients;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inputs")
	TArray<TArray<FCraftingIngredient>> AlternativeInputSets;

	// Output configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outputs")
	TArray<FCraftingOutput> PrimaryOutputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outputs")
	TArray<FCraftingOutput> BonusOutputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outputs")
	float BonusOutputChance;

	// Production requirements
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	ECraftingTier RequiredTier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	TMap<FName, int32> RequiredSkillLevels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	TArray<FName> RequiredBlueprints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	int32 RequiredFacilityLevel;

	// Time and energy costs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Costs")
	float BaseCraftingTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Costs")
	int32 EnergyCost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Costs")
	float WearOnTools;

	// Quality modifiers
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality")
	float BaseQualityChance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality")
	bool bQualityAffectedBySkill;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality")
	bool bQualityAffectedByInputQuality;

	// Experience and progression
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	int32 BaseExperienceReward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	TMap<FName, int32> SkillExperienceRewards;

	// Automation support
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Automation")
	bool bCanBeAutomated;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Automation")
	int32 AutomationTierRequired;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Automation")
	float AutomationEfficiencyPenalty;

	// Production chain metadata
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain")
	TArray<FName> PrerequisiteRecipes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain")
	TArray<FName> UnlocksRecipes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain")
	int32 ChainDepth;

	FAdvancedCraftingRecipe()
	{
		RecipeID = NAME_None;
		RecipeName = TEXT("Unknown Recipe");
		Description = TEXT("");
		OutputCategory = EItemCategory::Component;
		BonusOutputChance = 0.0f;
		RequiredTier = ECraftingTier::Basic;
		RequiredFacilityLevel = 1;
		BaseCraftingTime = 5.0f;
		EnergyCost = 10;
		WearOnTools = 1.0f;
		BaseQualityChance = 0.5f;
		bQualityAffectedBySkill = true;
		bQualityAffectedByInputQuality = true;
		BaseExperienceReward = 10;
		bCanBeAutomated = true;
		AutomationTierRequired = 4;
		AutomationEfficiencyPenalty = 0.1f;
		ChainDepth = 0;
	}
};

/**
 * Active crafting job with progress tracking
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FCraftingJob
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Job")
	FGuid JobID;

	UPROPERTY(BlueprintReadOnly, Category = "Job")
	FName RecipeID;

	UPROPERTY(BlueprintReadOnly, Category = "Job")
	int32 Quantity;

	UPROPERTY(BlueprintReadOnly, Category = "Job")
	int32 CompletedQuantity;

	UPROPERTY(BlueprintReadOnly, Category = "Job")
	float Progress;

	UPROPERTY(BlueprintReadOnly, Category = "Job")
	float RemainingTime;

	UPROPERTY(BlueprintReadOnly, Category = "Job")
	float TotalTime;

	UPROPERTY(BlueprintReadOnly, Category = "Job")
	ECraftingState State;

	UPROPERTY(BlueprintReadOnly, Category = "Job")
	EItemQuality TargetQuality;

	UPROPERTY(BlueprintReadOnly, Category = "Job")
	bool bIsAutomated;

	UPROPERTY(BlueprintReadOnly, Category = "Job")
	FName StationID;

	UPROPERTY(BlueprintReadOnly, Category = "Job")
	int32 Priority;

	UPROPERTY(BlueprintReadOnly, Category = "Job")
	TArray<FCraftedItem> ProducedItems;

	FCraftingJob()
	{
		JobID = FGuid::NewGuid();
		RecipeID = NAME_None;
		Quantity = 1;
		CompletedQuantity = 0;
		Progress = 0.0f;
		RemainingTime = 0.0f;
		TotalTime = 0.0f;
		State = ECraftingState::Idle;
		TargetQuality = EItemQuality::Standard;
		bIsAutomated = false;
		StationID = NAME_None;
		Priority = 0;
	}
};

/**
 * Crafting facility configuration
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FCraftingFacility
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facility")
	FName FacilityID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facility")
	FString FacilityName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facility")
	ECraftingTier Tier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facility")
	int32 Level;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facility")
	int32 MaxConcurrentJobs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facility")
	float SpeedMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facility")
	float QualityBonus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facility")
	float EnergyEfficiency;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facility")
	TArray<EItemCategory> SupportedCategories;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facility")
	TArray<FCraftingJob> ActiveJobs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facility")
	bool bIsOnline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facility")
	float CurrentEnergyDraw;

	FCraftingFacility()
	{
		FacilityID = NAME_None;
		FacilityName = TEXT("Crafting Station");
		Tier = ECraftingTier::Basic;
		Level = 1;
		MaxConcurrentJobs = 1;
		SpeedMultiplier = 1.0f;
		QualityBonus = 0.0f;
		EnergyEfficiency = 1.0f;
		bIsOnline = true;
		CurrentEnergyDraw = 0.0f;
	}
};

/**
 * Crafting statistics for analytics
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FCraftingStatistics
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 TotalItemsCrafted;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	TMap<EItemQuality, int32> ItemsByQuality;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	TMap<EItemCategory, int32> ItemsByCategory;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float TotalCraftingTimeSpent;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 SuccessfulCrafts;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 FailedCrafts;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 MasterworkItemsCreated;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 LegendaryItemsCreated;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	TMap<FName, int32> RecipesCraftedCount;

	FCraftingStatistics()
	{
		TotalItemsCrafted = 0;
		TotalCraftingTimeSpent = 0.0f;
		SuccessfulCrafts = 0;
		FailedCrafts = 0;
		MasterworkItemsCreated = 0;
		LegendaryItemsCreated = 0;
	}
};

// Delegate declarations
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCraftingJobStarted, FGuid, JobID, FName, RecipeID, int32, Quantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCraftingJobCompleted, FGuid, JobID, const TArray<FCraftedItem>&, ProducedItems, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCraftingJobCancelled, FGuid, JobID, FName, RecipeID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCraftingJobProgress, FGuid, JobID, float, Progress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQualityDetermined, FGuid, JobID, EItemQuality, Quality);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnRecipeUnlocked, FName, RecipeID, FName, SkillName, int32, SkillLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFacilityStatusChanged, FName, FacilityID, bool, bIsOnline);

/**
 * Master Crafting Manager Component
 * 
 * Core features:
 * - Manages all crafting operations across facilities
 * - Coordinates with skill, quality, and automation systems
 * - Handles complex production chains
 * - Provides crafting queue management
 * - Integrates with economy for resource consumption/production
 * - Mobile-optimized with efficient batch processing
 */
UCLASS(ClassGroup=(Odyssey), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UOdysseyCraftingManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdysseyCraftingManager();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// Core Crafting Operations
	// ============================================================================

	/**
	 * Start a new crafting job
	 * @param RecipeID Recipe to craft
	 * @param Quantity Number of items to craft
	 * @param FacilityID Optional facility to use
	 * @param Priority Job priority (higher = processed first)
	 * @return Job ID or invalid GUID if failed
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	FGuid StartCraftingJob(FName RecipeID, int32 Quantity = 1, FName FacilityID = NAME_None, int32 Priority = 0);

	/**
	 * Cancel an active crafting job
	 * @param JobID Job to cancel
	 * @param bRefundMaterials Whether to refund consumed materials
	 * @return Success status
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool CancelCraftingJob(FGuid JobID, bool bRefundMaterials = true);

	/**
	 * Pause an active crafting job
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool PauseCraftingJob(FGuid JobID);

	/**
	 * Resume a paused crafting job
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool ResumeCraftingJob(FGuid JobID);

	/**
	 * Instant craft a recipe (debug/premium feature)
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	TArray<FCraftedItem> InstantCraft(FName RecipeID, int32 Quantity = 1);

	/**
	 * Check if a recipe can be crafted
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool CanCraftRecipe(FName RecipeID, int32 Quantity = 1, FName FacilityID = NAME_None) const;

	/**
	 * Get list of craftable recipes for current state
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	TArray<FName> GetCraftableRecipes(FName FacilityID = NAME_None) const;

	// ============================================================================
	// Recipe Management
	// ============================================================================

	/**
	 * Get recipe data by ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipes")
	FAdvancedCraftingRecipe GetRecipe(FName RecipeID) const;

	/**
	 * Get all available recipes
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipes")
	TArray<FAdvancedCraftingRecipe> GetAllRecipes() const;

	/**
	 * Get recipes by category
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipes")
	TArray<FAdvancedCraftingRecipe> GetRecipesByCategory(EItemCategory Category) const;

	/**
	 * Check if recipe is unlocked
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipes")
	bool IsRecipeUnlocked(FName RecipeID) const;

	/**
	 * Unlock a recipe
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipes")
	bool UnlockRecipe(FName RecipeID, FName UnlockSource = NAME_None);

	/**
	 * Get production chain for a recipe
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipes")
	TArray<FName> GetProductionChain(FName RecipeID) const;

	/**
	 * Calculate total materials needed for recipe chain
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipes")
	TArray<FCraftingIngredient> CalculateChainMaterials(FName RecipeID, int32 Quantity = 1) const;

	// ============================================================================
	// Facility Management
	// ============================================================================

	/**
	 * Register a crafting facility
	 */
	UFUNCTION(BlueprintCallable, Category = "Facilities")
	bool RegisterFacility(const FCraftingFacility& Facility);

	/**
	 * Unregister a crafting facility
	 */
	UFUNCTION(BlueprintCallable, Category = "Facilities")
	bool UnregisterFacility(FName FacilityID);

	/**
	 * Get facility by ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Facilities")
	FCraftingFacility GetFacility(FName FacilityID) const;

	/**
	 * Get all registered facilities
	 */
	UFUNCTION(BlueprintCallable, Category = "Facilities")
	TArray<FCraftingFacility> GetAllFacilities() const;

	/**
	 * Set facility online status
	 */
	UFUNCTION(BlueprintCallable, Category = "Facilities")
	bool SetFacilityOnlineStatus(FName FacilityID, bool bOnline);

	/**
	 * Upgrade facility tier
	 */
	UFUNCTION(BlueprintCallable, Category = "Facilities")
	bool UpgradeFacility(FName FacilityID);

	/**
	 * Get best facility for a recipe
	 */
	UFUNCTION(BlueprintCallable, Category = "Facilities")
	FName GetBestFacilityForRecipe(FName RecipeID) const;

	// ============================================================================
	// Job Queue Management
	// ============================================================================

	/**
	 * Get all active crafting jobs
	 */
	UFUNCTION(BlueprintCallable, Category = "Jobs")
	TArray<FCraftingJob> GetActiveJobs() const;

	/**
	 * Get jobs for a specific facility
	 */
	UFUNCTION(BlueprintCallable, Category = "Jobs")
	TArray<FCraftingJob> GetJobsForFacility(FName FacilityID) const;

	/**
	 * Get job by ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Jobs")
	FCraftingJob GetJob(FGuid JobID) const;

	/**
	 * Reorder job priority
	 */
	UFUNCTION(BlueprintCallable, Category = "Jobs")
	bool SetJobPriority(FGuid JobID, int32 NewPriority);

	/**
	 * Get estimated completion time for job
	 */
	UFUNCTION(BlueprintCallable, Category = "Jobs")
	float GetJobEstimatedCompletion(FGuid JobID) const;

	/**
	 * Get total queue time for all jobs
	 */
	UFUNCTION(BlueprintCallable, Category = "Jobs")
	float GetTotalQueueTime() const;

	// ============================================================================
	// Quality and Crafting Calculations
	// ============================================================================

	/**
	 * Calculate expected quality for a craft
	 */
	UFUNCTION(BlueprintCallable, Category = "Quality")
	EItemQuality CalculateExpectedQuality(FName RecipeID, FName FacilityID = NAME_None) const;

	/**
	 * Calculate actual crafting time with modifiers
	 */
	UFUNCTION(BlueprintCallable, Category = "Calculations")
	float CalculateCraftingTime(FName RecipeID, int32 Quantity, FName FacilityID = NAME_None) const;

	/**
	 * Calculate energy cost with modifiers
	 */
	UFUNCTION(BlueprintCallable, Category = "Calculations")
	int32 CalculateEnergyCost(FName RecipeID, int32 Quantity, FName FacilityID = NAME_None) const;

	/**
	 * Calculate success chance
	 */
	UFUNCTION(BlueprintCallable, Category = "Calculations")
	float CalculateSuccessChance(FName RecipeID, FName FacilityID = NAME_None) const;

	// ============================================================================
	// Component Integration
	// ============================================================================

	/**
	 * Set inventory component
	 */
	UFUNCTION(BlueprintCallable, Category = "Integration")
	void SetInventoryComponent(UOdysseyInventoryComponent* Inventory);

	/**
	 * Set trading component for economy integration
	 */
	UFUNCTION(BlueprintCallable, Category = "Integration")
	void SetTradingComponent(UOdysseyTradingComponent* Trading);

	/**
	 * Get recipe component
	 */
	UFUNCTION(BlueprintCallable, Category = "Integration")
	UOdysseyCraftingRecipeComponent* GetRecipeComponent() const { return RecipeComponent; }

	/**
	 * Get automation system
	 */
	UFUNCTION(BlueprintCallable, Category = "Integration")
	UOdysseyAutomationNetworkSystem* GetAutomationSystem() const { return AutomationSystem; }

	/**
	 * Get quality control system
	 */
	UFUNCTION(BlueprintCallable, Category = "Integration")
	UOdysseyQualityControlSystem* GetQualitySystem() const { return QualitySystem; }

	/**
	 * Get skill system
	 */
	UFUNCTION(BlueprintCallable, Category = "Integration")
	UOdysseyCraftingSkillSystem* GetSkillSystem() const { return SkillSystem; }

	// ============================================================================
	// Statistics
	// ============================================================================

	/**
	 * Get crafting statistics
	 */
	UFUNCTION(BlueprintCallable, Category = "Statistics")
	FCraftingStatistics GetCraftingStatistics() const { return Statistics; }

	/**
	 * Reset crafting statistics
	 */
	UFUNCTION(BlueprintCallable, Category = "Statistics")
	void ResetStatistics();

	// ============================================================================
	// Events
	// ============================================================================

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCraftingJobStarted OnCraftingJobStarted;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCraftingJobCompleted OnCraftingJobCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCraftingJobCancelled OnCraftingJobCancelled;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCraftingJobProgress OnCraftingJobProgress;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnQualityDetermined OnQualityDetermined;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnRecipeUnlocked OnRecipeUnlocked;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnFacilityStatusChanged OnFacilityStatusChanged;

protected:
	// ============================================================================
	// Configuration
	// ============================================================================

	/**
	 * Recipe data table
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	UDataTable* RecipeDataTable;

	/**
	 * Maximum concurrent jobs across all facilities
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	int32 MaxGlobalConcurrentJobs;

	/**
	 * Update frequency for job processing (mobile optimization)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration|Mobile")
	float JobUpdateFrequency;

	/**
	 * Batch size for job processing (mobile optimization)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration|Mobile")
	int32 JobBatchSize;

	/**
	 * Enable detailed logging for debugging
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration|Debug")
	bool bEnableDebugLogging;

	// ============================================================================
	// Runtime State
	// ============================================================================

	/**
	 * All registered facilities
	 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TMap<FName, FCraftingFacility> Facilities;

	/**
	 * All active crafting jobs
	 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TArray<FCraftingJob> ActiveJobs;

	/**
	 * Unlocked recipe IDs
	 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TSet<FName> UnlockedRecipes;

	/**
	 * Crafting statistics
	 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	FCraftingStatistics Statistics;

	/**
	 * Time since last job update
	 */
	float TimeSinceLastUpdate;

	// ============================================================================
	// Component References
	// ============================================================================

	UPROPERTY()
	UOdysseyInventoryComponent* InventoryComponent;

	UPROPERTY()
	UOdysseyTradingComponent* TradingComponent;

	UPROPERTY()
	UOdysseyCraftingRecipeComponent* RecipeComponent;

	UPROPERTY()
	UOdysseyAutomationNetworkSystem* AutomationSystem;

	UPROPERTY()
	UOdysseyQualityControlSystem* QualitySystem;

	UPROPERTY()
	UOdysseyCraftingSkillSystem* SkillSystem;

	// ============================================================================
	// Internal Methods
	// ============================================================================

	/**
	 * Initialize sub-components
	 */
	void InitializeSubsystems();

	/**
	 * Process active crafting jobs
	 */
	void ProcessActiveJobs(float DeltaTime);

	/**
	 * Complete a crafting job
	 */
	void CompleteJob(int32 JobIndex);

	/**
	 * Consume ingredients for a job
	 */
	bool ConsumeJobIngredients(const FAdvancedCraftingRecipe& Recipe, int32 Quantity);

	/**
	 * Produce outputs for completed job
	 */
	TArray<FCraftedItem> ProduceJobOutputs(const FAdvancedCraftingRecipe& Recipe, int32 Quantity, EItemQuality Quality);

	/**
	 * Calculate quality for produced items
	 */
	EItemQuality DetermineOutputQuality(const FAdvancedCraftingRecipe& Recipe, FName FacilityID);

	/**
	 * Get facility for job processing
	 */
	FCraftingFacility* FindFacilityForJob(FCraftingJob& Job);

	/**
	 * Validate recipe requirements
	 */
	bool ValidateRecipeRequirements(const FAdvancedCraftingRecipe& Recipe, int32 Quantity, FName FacilityID) const;

	/**
	 * Update statistics after craft
	 */
	void UpdateStatistics(const TArray<FCraftedItem>& ProducedItems, bool bSuccess);

	/**
	 * Load recipes from data table
	 */
	void LoadRecipes();

	/**
	 * Sort jobs by priority
	 */
	void SortJobsByPriority();
};
