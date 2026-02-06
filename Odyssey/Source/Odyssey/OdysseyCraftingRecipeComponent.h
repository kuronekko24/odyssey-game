// OdysseyCraftingRecipeComponent.h
// Dynamic recipe system with skill-based unlocks and efficiency modifiers

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "OdysseyCraftingManager.h"
#include "OdysseyCraftingRecipeComponent.generated.h"

// Forward declarations
class UOdysseyCraftingSkillSystem;

/**
 * Recipe variation with different input/output configurations
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FRecipeVariation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variation")
	FName VariationID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variation")
	FString VariationName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variation")
	TArray<FCraftingIngredient> AlternativeIngredients;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variation")
	TArray<FCraftingOutput> ModifiedOutputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variation")
	float TimeModifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variation")
	float QualityModifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variation")
	int32 RequiredSkillLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variation")
	bool bIsDiscovered;

	FRecipeVariation()
	{
		VariationID = NAME_None;
		VariationName = TEXT("Standard");
		TimeModifier = 1.0f;
		QualityModifier = 1.0f;
		RequiredSkillLevel = 1;
		bIsDiscovered = false;
	}
};

/**
 * Blueprint/schematic item that unlocks recipes
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FCraftingBlueprint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blueprint")
	FName BlueprintID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blueprint")
	FString BlueprintName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blueprint")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blueprint")
	TArray<FName> UnlockedRecipes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blueprint")
	int32 ResearchCost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blueprint")
	float ResearchTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blueprint")
	TMap<FName, int32> RequiredSkillLevels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blueprint")
	TArray<FName> PrerequisiteBlueprints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blueprint")
	bool bIsResearched;

	FCraftingBlueprint()
	{
		BlueprintID = NAME_None;
		BlueprintName = TEXT("Unknown Blueprint");
		Description = TEXT("");
		ResearchCost = 100;
		ResearchTime = 60.0f;
		bIsResearched = false;
	}
};

/**
 * Research progress tracking
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FBlueprintResearchProgress
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Research")
	FName BlueprintID;

	UPROPERTY(BlueprintReadOnly, Category = "Research")
	float Progress;

	UPROPERTY(BlueprintReadOnly, Category = "Research")
	float RemainingTime;

	UPROPERTY(BlueprintReadOnly, Category = "Research")
	bool bIsPaused;

	FBlueprintResearchProgress()
	{
		BlueprintID = NAME_None;
		Progress = 0.0f;
		RemainingTime = 0.0f;
		bIsPaused = false;
	}
};

/**
 * Recipe efficiency modifiers from various sources
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FRecipeEfficiencyModifiers
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Efficiency")
	float SkillSpeedBonus;

	UPROPERTY(BlueprintReadOnly, Category = "Efficiency")
	float FacilitySpeedBonus;

	UPROPERTY(BlueprintReadOnly, Category = "Efficiency")
	float SkillQualityBonus;

	UPROPERTY(BlueprintReadOnly, Category = "Efficiency")
	float FacilityQualityBonus;

	UPROPERTY(BlueprintReadOnly, Category = "Efficiency")
	float MaterialEfficiency;

	UPROPERTY(BlueprintReadOnly, Category = "Efficiency")
	float BonusOutputChance;

	UPROPERTY(BlueprintReadOnly, Category = "Efficiency")
	float CriticalCraftChance;

	FRecipeEfficiencyModifiers()
	{
		SkillSpeedBonus = 0.0f;
		FacilitySpeedBonus = 0.0f;
		SkillQualityBonus = 0.0f;
		FacilityQualityBonus = 0.0f;
		MaterialEfficiency = 1.0f;
		BonusOutputChance = 0.0f;
		CriticalCraftChance = 0.0f;
	}

	float GetTotalSpeedBonus() const { return SkillSpeedBonus + FacilitySpeedBonus; }
	float GetTotalQualityBonus() const { return SkillQualityBonus + FacilityQualityBonus; }
};

// Delegate declarations
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRecipeDiscovered, FName, RecipeID, FName, VariationID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBlueprintResearched, FName, BlueprintID, const TArray<FName>&, UnlockedRecipes);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnResearchProgress, FName, BlueprintID, float, Progress);

/**
 * Crafting Recipe Component
 * 
 * Manages dynamic recipe system including:
 * - Recipe variations with alternative inputs/outputs
 * - Blueprint research for recipe unlocks
 * - Skill-based efficiency calculations
 * - Recipe discovery through experimentation
 */
UCLASS(ClassGroup=(Odyssey), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UOdysseyCraftingRecipeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdysseyCraftingRecipeComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// Recipe Variation System
	// ============================================================================

	/**
	 * Get available variations for a recipe
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipe Variations")
	TArray<FRecipeVariation> GetRecipeVariations(FName RecipeID) const;

	/**
	 * Get specific variation
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipe Variations")
	FRecipeVariation GetVariation(FName RecipeID, FName VariationID) const;

	/**
	 * Check if variation is discovered
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipe Variations")
	bool IsVariationDiscovered(FName RecipeID, FName VariationID) const;

	/**
	 * Discover a recipe variation
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipe Variations")
	bool DiscoverVariation(FName RecipeID, FName VariationID);

	/**
	 * Get effective recipe with variation applied
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipe Variations")
	FAdvancedCraftingRecipe GetEffectiveRecipe(FName RecipeID, FName VariationID = NAME_None) const;

	/**
	 * Attempt to discover new variation through experimentation
	 * @param RecipeID Base recipe to experiment with
	 * @param ExperimentalIngredients Additional ingredients to try
	 * @return Discovered variation ID or NAME_None if no discovery
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipe Variations")
	FName AttemptExperimentation(FName RecipeID, const TArray<FCraftingIngredient>& ExperimentalIngredients);

	// ============================================================================
	// Blueprint Research System
	// ============================================================================

	/**
	 * Get all blueprints
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprints")
	TArray<FCraftingBlueprint> GetAllBlueprints() const;

	/**
	 * Get blueprint by ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprints")
	FCraftingBlueprint GetBlueprint(FName BlueprintID) const;

	/**
	 * Check if blueprint is researched
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprints")
	bool IsBlueprintResearched(FName BlueprintID) const;

	/**
	 * Check if blueprint can be researched
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprints")
	bool CanResearchBlueprint(FName BlueprintID) const;

	/**
	 * Start researching a blueprint
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprints")
	bool StartBlueprintResearch(FName BlueprintID);

	/**
	 * Cancel blueprint research
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprints")
	bool CancelBlueprintResearch(FName BlueprintID);

	/**
	 * Pause/resume research
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprints")
	bool SetResearchPaused(FName BlueprintID, bool bPaused);

	/**
	 * Get current research progress
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprints")
	FBlueprintResearchProgress GetResearchProgress(FName BlueprintID) const;

	/**
	 * Get all active research projects
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprints")
	TArray<FBlueprintResearchProgress> GetActiveResearch() const;

	/**
	 * Add a blueprint to collection
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprints")
	bool AddBlueprint(const FCraftingBlueprint& Blueprint);

	// ============================================================================
	// Efficiency Calculations
	// ============================================================================

	/**
	 * Calculate all efficiency modifiers for a recipe
	 */
	UFUNCTION(BlueprintCallable, Category = "Efficiency")
	FRecipeEfficiencyModifiers CalculateEfficiencyModifiers(FName RecipeID, FName FacilityID = NAME_None) const;

	/**
	 * Calculate material efficiency (reduced input requirements)
	 */
	UFUNCTION(BlueprintCallable, Category = "Efficiency")
	float GetMaterialEfficiency(FName RecipeID) const;

	/**
	 * Calculate bonus output chance
	 */
	UFUNCTION(BlueprintCallable, Category = "Efficiency")
	float GetBonusOutputChance(FName RecipeID) const;

	/**
	 * Calculate critical craft chance (double quality tier)
	 */
	UFUNCTION(BlueprintCallable, Category = "Efficiency")
	float GetCriticalCraftChance(FName RecipeID) const;

	/**
	 * Apply efficiency to ingredient requirements
	 */
	UFUNCTION(BlueprintCallable, Category = "Efficiency")
	TArray<FCraftingIngredient> GetEfficientIngredients(FName RecipeID, int32 Quantity = 1) const;

	// ============================================================================
	// Recipe Information
	// ============================================================================

	/**
	 * Get recipe difficulty rating
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipe Info")
	float GetRecipeDifficulty(FName RecipeID) const;

	/**
	 * Get recipe profit margin estimate
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipe Info")
	float GetRecipeProfitMargin(FName RecipeID) const;

	/**
	 * Get recommended skill level for recipe
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipe Info")
	int32 GetRecommendedSkillLevel(FName RecipeID) const;

	/**
	 * Get recipes that use specific ingredient
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipe Info")
	TArray<FName> GetRecipesUsingIngredient(EResourceType ResourceType) const;

	/**
	 * Get recipes that produce specific output
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipe Info")
	TArray<FName> GetRecipesProducingOutput(EResourceType ResourceType) const;

	// ============================================================================
	// Integration
	// ============================================================================

	/**
	 * Set reference to skill system
	 */
	UFUNCTION(BlueprintCallable, Category = "Integration")
	void SetSkillSystem(UOdysseyCraftingSkillSystem* NewSkillSystem);

	/**
	 * Set reference to crafting manager
	 */
	UFUNCTION(BlueprintCallable, Category = "Integration")
	void SetCraftingManager(class UOdysseyCraftingManager* NewManager);

	// ============================================================================
	// Events
	// ============================================================================

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnRecipeDiscovered OnRecipeDiscovered;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnBlueprintResearched OnBlueprintResearched;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnResearchProgress OnResearchProgress;

protected:
	// ============================================================================
	// Configuration
	// ============================================================================

	/**
	 * Blueprint data table
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	UDataTable* BlueprintDataTable;

	/**
	 * Recipe variations data table
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	UDataTable* VariationDataTable;

	/**
	 * Maximum concurrent research projects
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	int32 MaxConcurrentResearch;

	/**
	 * Base experimentation success chance
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	float BaseExperimentationChance;

	/**
	 * Research speed multiplier
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	float ResearchSpeedMultiplier;

	// ============================================================================
	// Runtime State
	// ============================================================================

	/**
	 * Discovered recipe variations
	 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TMap<FName, TArray<FName>> DiscoveredVariations;

	/**
	 * Researched blueprints
	 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TSet<FName> ResearchedBlueprints;

	/**
	 * Active research progress
	 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TArray<FBlueprintResearchProgress> ActiveResearchProjects;

	/**
	 * Custom blueprints added at runtime
	 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TMap<FName, FCraftingBlueprint> CustomBlueprints;

	/**
	 * Experimentation history for discovery tracking
	 */
	TMap<FName, TSet<uint32>> ExperimentationHistory;

	// ============================================================================
	// Component References
	// ============================================================================

	UPROPERTY()
	UOdysseyCraftingSkillSystem* SkillSystem;

	UPROPERTY()
	class UOdysseyCraftingManager* CraftingManager;

	// ============================================================================
	// Internal Methods
	// ============================================================================

	/**
	 * Process active research
	 */
	void ProcessActiveResearch(float DeltaTime);

	/**
	 * Complete blueprint research
	 */
	void CompleteBlueprintResearch(int32 ResearchIndex);

	/**
	 * Load blueprints from data table
	 */
	void LoadBlueprints();

	/**
	 * Load variations from data table
	 */
	void LoadVariations();

	/**
	 * Calculate experimentation hash for tracking
	 */
	uint32 CalculateExperimentationHash(const TArray<FCraftingIngredient>& Ingredients) const;

	/**
	 * Get variation data from data table
	 */
	FRecipeVariation* FindVariationData(FName RecipeID, FName VariationID) const;
};
