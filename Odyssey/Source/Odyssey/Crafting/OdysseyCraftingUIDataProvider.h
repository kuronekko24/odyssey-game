// OdysseyCraftingUIDataProvider.h
// Mobile-optimized UI data provider for the crafting system
// Pre-computes and caches display data to minimize per-frame work on mobile devices

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OdysseyCraftingManager.h"
#include "OdysseyCraftingUIDataProvider.generated.h"

// Forward declarations
class UOdysseyCraftingManager;
class UOdysseyCraftingRecipeComponent;
class UOdysseyQualityControlSystem;
class UOdysseyCraftingSkillSystem;

/**
 * Pre-computed recipe display data for UI rendering
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FRecipeDisplayData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	FName RecipeID;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	FString DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	FString Description;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	EItemCategory Category;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	ECraftingTier RequiredTier;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	bool bIsUnlocked;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	bool bCanCraftNow;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	bool bHasMaterials;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	bool bHasSkills;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	float CraftingTime;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	int32 EnergyCost;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	EItemQuality ExpectedQuality;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	FLinearColor QualityColor;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	float SuccessChance;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	float DifficultyRating;

	// Ingredient status (for each ingredient: have / need)
	UPROPERTY(BlueprintReadOnly, Category = "Display")
	TArray<FString> IngredientStatusLines;

	// Output preview
	UPROPERTY(BlueprintReadOnly, Category = "Display")
	TArray<FString> OutputPreviewLines;

	FRecipeDisplayData()
	{
		RecipeID = NAME_None;
		Category = EItemCategory::RawMaterial;
		RequiredTier = ECraftingTier::Basic;
		bIsUnlocked = false;
		bCanCraftNow = false;
		bHasMaterials = false;
		bHasSkills = false;
		CraftingTime = 0.0f;
		EnergyCost = 0;
		ExpectedQuality = EItemQuality::Common;
		QualityColor = FLinearColor::White;
		SuccessChance = 0.0f;
		DifficultyRating = 0.0f;
	}
};

/**
 * Pre-computed job display data
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FJobDisplayData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	FGuid JobID;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	FString RecipeName;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	int32 Quantity;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	int32 CompletedQuantity;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	float Progress;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	FString TimeRemainingText;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	FString StatusText;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	FLinearColor StatusColor;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	bool bCanCancel;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	bool bCanPause;

	FJobDisplayData()
	{
		Quantity = 0;
		CompletedQuantity = 0;
		Progress = 0.0f;
		bCanCancel = false;
		bCanPause = false;
		StatusColor = FLinearColor::White;
	}
};

/**
 * Skill display data for skill tree UI
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FSkillDisplayData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	FName SkillID;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	FString SkillName;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	FString Description;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	ECraftingSkillCategory Category;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	int32 CurrentLevel;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	int32 MaxLevel;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	float ExperienceProgress;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	bool bIsUnlocked;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	bool bCanUnlock;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	FString BonusDescription;

	UPROPERTY(BlueprintReadOnly, Category = "Display")
	FVector2D TreePosition;

	FSkillDisplayData()
	{
		Category = ECraftingSkillCategory::General;
		CurrentLevel = 0;
		MaxLevel = 10;
		ExperienceProgress = 0.0f;
		bIsUnlocked = false;
		bCanUnlock = false;
		TreePosition = FVector2D::ZeroVector;
	}
};

// Delegate declarations
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCraftingUIDataUpdated);

/**
 * Crafting UI Data Provider
 *
 * Pre-computes and caches all UI display data at regular intervals to avoid
 * expensive per-frame calculations on mobile. Widgets poll this for their
 * display state instead of querying the crafting subsystems directly.
 *
 * Features:
 * - Batched recipe status computation
 * - Cached quality/color lookups
 * - Pre-formatted text strings for display
 * - Configurable refresh rate for mobile power savings
 */
UCLASS(ClassGroup=(Odyssey), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UOdysseyCraftingUIDataProvider : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdysseyCraftingUIDataProvider();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// Recipe Display Data
	// ============================================================================

	/**
	 * Get display data for all available recipes
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipe Display")
	TArray<FRecipeDisplayData> GetAllRecipeDisplayData() const;

	/**
	 * Get display data for recipes in a category
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipe Display")
	TArray<FRecipeDisplayData> GetRecipeDisplayDataByCategory(EItemCategory Category) const;

	/**
	 * Get display data for a single recipe
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipe Display")
	FRecipeDisplayData GetRecipeDisplayData(FName RecipeID) const;

	/**
	 * Get craftable recipes (pre-filtered)
	 */
	UFUNCTION(BlueprintCallable, Category = "Recipe Display")
	TArray<FRecipeDisplayData> GetCraftableRecipeDisplayData() const;

	// ============================================================================
	// Job Display Data
	// ============================================================================

	/**
	 * Get display data for all active jobs
	 */
	UFUNCTION(BlueprintCallable, Category = "Job Display")
	TArray<FJobDisplayData> GetActiveJobDisplayData() const;

	/**
	 * Get display data for a specific job
	 */
	UFUNCTION(BlueprintCallable, Category = "Job Display")
	FJobDisplayData GetJobDisplayData(FGuid JobID) const;

	// ============================================================================
	// Skill Display Data
	// ============================================================================

	/**
	 * Get display data for all skills
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill Display")
	TArray<FSkillDisplayData> GetAllSkillDisplayData() const;

	/**
	 * Get skill display data by category
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill Display")
	TArray<FSkillDisplayData> GetSkillDisplayDataByCategory(ECraftingSkillCategory Category) const;

	/**
	 * Get available skill points text
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill Display")
	FString GetSkillPointsText() const;

	// ============================================================================
	// Utility Display
	// ============================================================================

	/**
	 * Format time for display
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	static FString FormatTimeRemaining(float Seconds);

	/**
	 * Force refresh all cached data
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	void ForceRefresh();

	// ============================================================================
	// Events
	// ============================================================================

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCraftingUIDataUpdated OnCraftingUIDataUpdated;

protected:
	// ============================================================================
	// Configuration
	// ============================================================================

	/**
	 * How often to refresh cached data (seconds)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	float RefreshInterval;

	/**
	 * Max recipes to process per refresh tick (mobile optimization)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	int32 MaxRecipesPerRefresh;

	// ============================================================================
	// Cached State
	// ============================================================================

	UPROPERTY()
	TMap<FName, FRecipeDisplayData> CachedRecipeData;

	UPROPERTY()
	TArray<FJobDisplayData> CachedJobData;

	UPROPERTY()
	TArray<FSkillDisplayData> CachedSkillData;

	float TimeSinceLastRefresh;

	// ============================================================================
	// Component References
	// ============================================================================

	UPROPERTY()
	UOdysseyCraftingManager* CraftingManager;

	UPROPERTY()
	UOdysseyCraftingRecipeComponent* RecipeComponent;

	UPROPERTY()
	UOdysseyQualityControlSystem* QualitySystem;

	UPROPERTY()
	UOdysseyCraftingSkillSystem* SkillSystem;

	// ============================================================================
	// Internal
	// ============================================================================

	void RefreshRecipeData();
	void RefreshJobData();
	void RefreshSkillData();

	FRecipeDisplayData BuildRecipeDisplayData(const FAdvancedCraftingRecipe& Recipe) const;
	FJobDisplayData BuildJobDisplayData(const FCraftingJob& Job) const;
	FSkillDisplayData BuildSkillDisplayData(const FCraftingSkill& Skill) const;
};
