// OdysseyCraftingSkillSystem.h
// Crafting skill progression and mastery system

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OdysseyCraftingManager.h"
#include "OdysseyCraftingSkillSystem.generated.h"

// Forward declarations
class UOdysseyCraftingManager;

/**
 * Skill category for organization
 */
UENUM(BlueprintType)
enum class ECraftingSkillCategory : uint8
{
	General = 0         UMETA(DisplayName = "General"),
	MaterialProcessing = 1 UMETA(DisplayName = "Material Processing"),
	WeaponCrafting = 2  UMETA(DisplayName = "Weapon Crafting"),
	ArmorCrafting = 3   UMETA(DisplayName = "Armor Crafting"),
	ShipModules = 4     UMETA(DisplayName = "Ship Modules"),
	Electronics = 5     UMETA(DisplayName = "Electronics"),
	Chemistry = 6       UMETA(DisplayName = "Chemistry"),
	Research = 7        UMETA(DisplayName = "Research"),
	Automation = 8      UMETA(DisplayName = "Automation")
};

/**
 * Crafting skill definition
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FCraftingSkill
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	FName SkillID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	FString SkillName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	ECraftingSkillCategory Category;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	int32 CurrentLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	int32 MaxLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	int32 CurrentExperience;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	int32 ExperienceToNextLevel;

	// Skill effects per level
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	float SpeedBonusPerLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	float QualityBonusPerLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	float SuccessBonusPerLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	float MaterialEfficiencyPerLevel;

	// Prerequisites
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prerequisites")
	TMap<FName, int32> RequiredSkillLevels;

	// Unlocks
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unlocks")
	TArray<FName> UnlocksRecipes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unlocks")
	TArray<FName> UnlocksSkills;

	FCraftingSkill()
	{
		SkillID = NAME_None;
		SkillName = TEXT("Unknown Skill");
		Description = TEXT("");
		Category = ECraftingSkillCategory::General;
		CurrentLevel = 0;
		MaxLevel = 10;
		CurrentExperience = 0;
		ExperienceToNextLevel = 100;
		SpeedBonusPerLevel = 0.02f;
		QualityBonusPerLevel = 0.015f;
		SuccessBonusPerLevel = 0.01f;
		MaterialEfficiencyPerLevel = 0.01f;
	}
};

/**
 * Skill point allocation
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FSkillPointAllocation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Skill Points")
	int32 TotalSkillPoints;

	UPROPERTY(BlueprintReadOnly, Category = "Skill Points")
	int32 AvailableSkillPoints;

	UPROPERTY(BlueprintReadOnly, Category = "Skill Points")
	int32 SpentSkillPoints;

	FSkillPointAllocation()
	{
		TotalSkillPoints = 0;
		AvailableSkillPoints = 0;
		SpentSkillPoints = 0;
	}
};

/**
 * Mastery bonus for specialized crafting
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FCraftingMasteryBonus
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mastery")
	FName MasteryID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mastery")
	FString MasteryName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mastery")
	ECraftingSkillCategory Category;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mastery")
	int32 RequiredTotalLevels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mastery")
	bool bIsUnlocked;

	// Mastery effects
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	float SpeedMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	float QualityMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	float UniqueItemChance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TArray<FName> ExclusiveRecipes;

	FCraftingMasteryBonus()
	{
		MasteryID = NAME_None;
		MasteryName = TEXT("Unknown Mastery");
		Category = ECraftingSkillCategory::General;
		RequiredTotalLevels = 50;
		bIsUnlocked = false;
		SpeedMultiplier = 1.2f;
		QualityMultiplier = 1.15f;
		UniqueItemChance = 0.05f;
	}
};

/**
 * Skill progress tracking
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FSkillProgressInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	FName SkillID;

	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	int32 Level;

	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	float ProgressToNextLevel;

	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	int32 TotalExperienceGained;

	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	int32 ItemsCraftedWithSkill;

	FSkillProgressInfo()
	{
		SkillID = NAME_None;
		Level = 0;
		ProgressToNextLevel = 0.0f;
		TotalExperienceGained = 0;
		ItemsCraftedWithSkill = 0;
	}
};

/**
 * Skill tree node for UI
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FSkillTreeNode
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Node")
	FName SkillID;

	UPROPERTY(BlueprintReadOnly, Category = "Node")
	FVector2D Position;

	UPROPERTY(BlueprintReadOnly, Category = "Node")
	TArray<FName> ConnectedSkills;

	UPROPERTY(BlueprintReadOnly, Category = "Node")
	bool bIsUnlocked;

	UPROPERTY(BlueprintReadOnly, Category = "Node")
	bool bCanUnlock;

	FSkillTreeNode()
	{
		SkillID = NAME_None;
		Position = FVector2D::ZeroVector;
		bIsUnlocked = false;
		bCanUnlock = false;
	}
};

// Delegate declarations
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSkillLevelUp, FName, SkillID, int32, OldLevel, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillExperienceGained, FName, SkillID, int32, ExperienceAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMasteryUnlocked, FName, MasteryID, ECraftingSkillCategory, Category);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillPointsEarned, int32, PointsEarned, int32, TotalPoints);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillUnlocked, FName, SkillID, FName, UnlockedBy);

/**
 * Crafting Skill System
 * 
 * Manages crafting skill progression:
 * - Multiple skill trees by crafting category
 * - Experience gain from crafting activities
 * - Mastery bonuses for specialized paths
 * - Skill point allocation system
 * - Recipe unlocks through progression
 */
UCLASS(ClassGroup=(Odyssey), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UOdysseyCraftingSkillSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdysseyCraftingSkillSystem();

protected:
	virtual void BeginPlay() override;

public:
	// ============================================================================
	// Skill Management
	// ============================================================================

	/**
	 * Get skill by ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Skills")
	FCraftingSkill GetSkill(FName SkillID) const;

	/**
	 * Get all skills
	 */
	UFUNCTION(BlueprintCallable, Category = "Skills")
	TArray<FCraftingSkill> GetAllSkills() const;

	/**
	 * Get skills by category
	 */
	UFUNCTION(BlueprintCallable, Category = "Skills")
	TArray<FCraftingSkill> GetSkillsByCategory(ECraftingSkillCategory Category) const;

	/**
	 * Get skill level
	 */
	UFUNCTION(BlueprintCallable, Category = "Skills")
	int32 GetSkillLevel(FName SkillID) const;

	/**
	 * Check if skill is unlocked
	 */
	UFUNCTION(BlueprintCallable, Category = "Skills")
	bool IsSkillUnlocked(FName SkillID) const;

	/**
	 * Check if skill can be unlocked
	 */
	UFUNCTION(BlueprintCallable, Category = "Skills")
	bool CanUnlockSkill(FName SkillID) const;

	/**
	 * Unlock a skill
	 */
	UFUNCTION(BlueprintCallable, Category = "Skills")
	bool UnlockSkill(FName SkillID);

	// ============================================================================
	// Experience System
	// ============================================================================

	/**
	 * Add experience to a skill
	 */
	UFUNCTION(BlueprintCallable, Category = "Experience")
	void AddSkillExperience(FName SkillID, int32 Experience);

	/**
	 * Add experience based on crafting activity
	 */
	UFUNCTION(BlueprintCallable, Category = "Experience")
	void AddCraftingExperience(FName RecipeID, int32 Quantity, EItemQuality Quality);

	/**
	 * Get experience progress for skill
	 */
	UFUNCTION(BlueprintCallable, Category = "Experience")
	FSkillProgressInfo GetSkillProgress(FName SkillID) const;

	/**
	 * Calculate experience required for level
	 */
	UFUNCTION(BlueprintCallable, Category = "Experience")
	int32 CalculateExperienceForLevel(int32 Level) const;

	// ============================================================================
	// Skill Points
	// ============================================================================

	/**
	 * Get skill point allocation info
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill Points")
	FSkillPointAllocation GetSkillPointAllocation() const;

	/**
	 * Spend skill point to level up skill
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill Points")
	bool SpendSkillPoint(FName SkillID);

	/**
	 * Reset skill points (respec)
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill Points")
	bool ResetSkillPoints();

	/**
	 * Add skill points (from progression)
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill Points")
	void AddSkillPoints(int32 Points);

	// ============================================================================
	// Crafting Bonuses
	// ============================================================================

	/**
	 * Get total crafting speed bonus from skills
	 */
	UFUNCTION(BlueprintCallable, Category = "Bonuses")
	float GetCraftingSpeedBonus() const;

	/**
	 * Get total quality bonus from skills
	 */
	UFUNCTION(BlueprintCallable, Category = "Bonuses")
	float GetCraftingQualityBonus() const;

	/**
	 * Get total success bonus from skills
	 */
	UFUNCTION(BlueprintCallable, Category = "Bonuses")
	float GetCraftingSuccessBonus() const;

	/**
	 * Get material efficiency bonus
	 */
	UFUNCTION(BlueprintCallable, Category = "Bonuses")
	float GetMaterialEfficiencyBonus() const;

	/**
	 * Get category-specific bonus
	 */
	UFUNCTION(BlueprintCallable, Category = "Bonuses")
	float GetCategoryBonus(ECraftingSkillCategory Category, FName BonusType) const;

	/**
	 * Get effective skill bonus for a recipe
	 */
	UFUNCTION(BlueprintCallable, Category = "Bonuses")
	float GetRecipeSkillBonus(FName RecipeID, FName BonusType) const;

	// ============================================================================
	// Mastery System
	// ============================================================================

	/**
	 * Get all masteries
	 */
	UFUNCTION(BlueprintCallable, Category = "Mastery")
	TArray<FCraftingMasteryBonus> GetAllMasteries() const;

	/**
	 * Get mastery by ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Mastery")
	FCraftingMasteryBonus GetMastery(FName MasteryID) const;

	/**
	 * Check if mastery is unlocked
	 */
	UFUNCTION(BlueprintCallable, Category = "Mastery")
	bool IsMasteryUnlocked(FName MasteryID) const;

	/**
	 * Get mastery progress
	 */
	UFUNCTION(BlueprintCallable, Category = "Mastery")
	float GetMasteryProgress(FName MasteryID) const;

	/**
	 * Get total levels in category
	 */
	UFUNCTION(BlueprintCallable, Category = "Mastery")
	int32 GetTotalLevelsInCategory(ECraftingSkillCategory Category) const;

	/**
	 * Get active mastery bonuses
	 */
	UFUNCTION(BlueprintCallable, Category = "Mastery")
	TArray<FCraftingMasteryBonus> GetActiveMasteryBonuses() const;

	// ============================================================================
	// Skill Tree
	// ============================================================================

	/**
	 * Get skill tree nodes for UI
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill Tree")
	TArray<FSkillTreeNode> GetSkillTreeNodes(ECraftingSkillCategory Category) const;

	/**
	 * Get skill prerequisites
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill Tree")
	TArray<FName> GetSkillPrerequisites(FName SkillID) const;

	/**
	 * Get skills unlocked by this skill
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill Tree")
	TArray<FName> GetUnlockedBySkill(FName SkillID) const;

	// ============================================================================
	// Statistics
	// ============================================================================

	/**
	 * Get total skill levels
	 */
	UFUNCTION(BlueprintCallable, Category = "Statistics")
	int32 GetTotalSkillLevels() const;

	/**
	 * Get total experience earned
	 */
	UFUNCTION(BlueprintCallable, Category = "Statistics")
	int32 GetTotalExperienceEarned() const;

	/**
	 * Get highest skill level
	 */
	UFUNCTION(BlueprintCallable, Category = "Statistics")
	int32 GetHighestSkillLevel() const;

	/**
	 * Get unlocked skills count
	 */
	UFUNCTION(BlueprintCallable, Category = "Statistics")
	int32 GetUnlockedSkillsCount() const;

	// ============================================================================
	// Integration
	// ============================================================================

	/**
	 * Set crafting manager reference
	 */
	UFUNCTION(BlueprintCallable, Category = "Integration")
	void SetCraftingManager(UOdysseyCraftingManager* NewManager);

	// ============================================================================
	// Events
	// ============================================================================

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSkillLevelUp OnSkillLevelUp;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSkillExperienceGained OnSkillExperienceGained;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMasteryUnlocked OnMasteryUnlocked;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSkillPointsEarned OnSkillPointsEarned;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSkillUnlocked OnSkillUnlocked;

protected:
	// ============================================================================
	// Configuration
	// ============================================================================

	/**
	 * Skill data table
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	UDataTable* SkillDataTable;

	/**
	 * Base experience curve multiplier
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	float ExperienceCurveMultiplier;

	/**
	 * Skill points earned per total level
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	int32 SkillPointsPerLevel;

	/**
	 * Respec cost multiplier
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	float RespecCostMultiplier;

	// ============================================================================
	// Runtime State
	// ============================================================================

	/**
	 * All skills
	 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TMap<FName, FCraftingSkill> Skills;

	/**
	 * Unlocked skills
	 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TSet<FName> UnlockedSkills;

	/**
	 * Skill point allocation
	 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	FSkillPointAllocation SkillPoints;

	/**
	 * Mastery bonuses
	 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TMap<FName, FCraftingMasteryBonus> Masteries;

	/**
	 * Unlocked masteries
	 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TSet<FName> UnlockedMasteries;

	/**
	 * Total experience earned
	 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	int32 TotalExperience;

	// ============================================================================
	// Component References
	// ============================================================================

	UPROPERTY()
	UOdysseyCraftingManager* CraftingManager;

	// ============================================================================
	// Internal Methods
	// ============================================================================

	/**
	 * Initialize default skills
	 */
	void InitializeDefaultSkills();

	/**
	 * Initialize masteries
	 */
	void InitializeDefaultMasteries();

	/**
	 * Level up a skill
	 */
	void LevelUpSkill(FName SkillID);

	/**
	 * Check and unlock masteries
	 */
	void CheckMasteryUnlocks();

	/**
	 * Calculate skill points earned
	 */
	void UpdateSkillPoints();

	/**
	 * Get relevant skills for recipe
	 */
	TArray<FName> GetRelevantSkillsForRecipe(FName RecipeID) const;
};
