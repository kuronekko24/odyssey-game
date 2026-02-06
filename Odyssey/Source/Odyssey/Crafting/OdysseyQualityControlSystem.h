// OdysseyQualityControlSystem.h
// Quality system creating item tiers and value differentiation

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OdysseyCraftingManager.h"
#include "OdysseyQualityControlSystem.generated.h"

// Forward declarations
class UOdysseyCraftingSkillSystem;
class UOdysseyCraftingManager;

/**
 * Quality modifier source
 */
UENUM(BlueprintType)
enum class EQualityModifierSource : uint8
{
	Skill = 0       UMETA(DisplayName = "Skill"),
	Facility = 1    UMETA(DisplayName = "Facility"),
	Material = 2    UMETA(DisplayName = "Material"),
	Tool = 3        UMETA(DisplayName = "Tool"),
	Catalyst = 4    UMETA(DisplayName = "Catalyst"),
	Random = 5      UMETA(DisplayName = "Random")
};

/**
 * Quality modifier entry
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FQualityModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	EQualityModifierSource Source;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FString SourceName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	float Modifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	bool bIsMultiplicative;

	FQualityModifier()
	{
		Source = EQualityModifierSource::Random;
		SourceName = TEXT("Unknown");
		Modifier = 0.0f;
		bIsMultiplicative = false;
	}

	FQualityModifier(EQualityModifierSource InSource, FString InName, float InModifier, bool bMultiplicative = false)
	{
		Source = InSource;
		SourceName = InName;
		Modifier = InModifier;
		bIsMultiplicative = bMultiplicative;
	}
};

/**
 * Quality tier thresholds configuration
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FQualityTierConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Tier")
	EItemQuality Quality;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Tier")
	float MinScore;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Tier")
	float MaxScore;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Tier")
	float ValueMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Tier")
	float StatBonus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality Tier")
	FLinearColor TierColor;

	FQualityTierConfig()
	{
		Quality = EItemQuality::Common;
		MinScore = 0.0f;
		MaxScore = 0.25f;
		ValueMultiplier = 1.0f;
		StatBonus = 0.0f;
		TierColor = FLinearColor::White;
	}
};

/**
 * Quality roll result with detailed breakdown
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FQualityRollResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Roll Result")
	EItemQuality ResultQuality;

	UPROPERTY(BlueprintReadOnly, Category = "Roll Result")
	float BaseScore;

	UPROPERTY(BlueprintReadOnly, Category = "Roll Result")
	float FinalScore;

	UPROPERTY(BlueprintReadOnly, Category = "Roll Result")
	TArray<FQualityModifier> AppliedModifiers;

	UPROPERTY(BlueprintReadOnly, Category = "Roll Result")
	bool bWasCritical;

	UPROPERTY(BlueprintReadOnly, Category = "Roll Result")
	float ValueMultiplier;

	UPROPERTY(BlueprintReadOnly, Category = "Roll Result")
	float StatBonus;

	FQualityRollResult()
	{
		ResultQuality = EItemQuality::Common;
		BaseScore = 0.0f;
		FinalScore = 0.0f;
		bWasCritical = false;
		ValueMultiplier = 1.0f;
		StatBonus = 0.0f;
	}
};

/**
 * Equipment quality effect
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FQualityEquipmentEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Effect")
	EItemQuality Quality;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Effect")
	float DamageMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Effect")
	float DefenseMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Effect")
	float DurabilityMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Effect")
	int32 BonusSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Effect")
	TMap<FName, float> StatBonuses;

	FQualityEquipmentEffect()
	{
		Quality = EItemQuality::Common;
		DamageMultiplier = 1.0f;
		DefenseMultiplier = 1.0f;
		DurabilityMultiplier = 1.0f;
		BonusSlots = 0;
	}
};

/**
 * Market demand modifier by quality
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FQualityMarketDemand
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market Demand")
	EItemQuality Quality;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market Demand")
	float DemandMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market Demand")
	float PriceMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market Demand")
	float SupplyScarcity;

	FQualityMarketDemand()
	{
		Quality = EItemQuality::Common;
		DemandMultiplier = 1.0f;
		PriceMultiplier = 1.0f;
		SupplyScarcity = 1.0f;
	}
};

/**
 * Quality inspection result
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FQualityInspection
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inspection")
	EItemQuality Quality;

	UPROPERTY(BlueprintReadOnly, Category = "Inspection")
	float QualityScore;

	UPROPERTY(BlueprintReadOnly, Category = "Inspection")
	float Authenticity;

	UPROPERTY(BlueprintReadOnly, Category = "Inspection")
	FString CrafterName;

	UPROPERTY(BlueprintReadOnly, Category = "Inspection")
	FDateTime CraftDate;

	UPROPERTY(BlueprintReadOnly, Category = "Inspection")
	int32 EstimatedValue;

	UPROPERTY(BlueprintReadOnly, Category = "Inspection")
	TArray<FString> QualityNotes;

	FQualityInspection()
	{
		Quality = EItemQuality::Common;
		QualityScore = 0.0f;
		Authenticity = 1.0f;
		EstimatedValue = 0;
	}
};

// Delegate declarations
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQualityRolled, const FQualityRollResult&, Result, FName, RecipeID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCriticalCraft, FName, RecipeID, EItemQuality, Quality, float, BonusValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLegendaryCreated, FName, RecipeID, const FCraftedItem&, Item);

/**
 * Quality Control System
 * 
 * Manages quality determination and item value:
 * - Multi-factor quality calculation
 * - Skill, facility, and material influences
 * - Quality-based pricing and market demand
 * - Equipment stat modifications by quality
 * - Critical craft system for exceptional items
 */
UCLASS(ClassGroup=(Odyssey), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UOdysseyQualityControlSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdysseyQualityControlSystem();

protected:
	virtual void BeginPlay() override;

public:
	// ============================================================================
	// Quality Calculation
	// ============================================================================

	/**
	 * Roll for quality with all modifiers
	 */
	UFUNCTION(BlueprintCallable, Category = "Quality")
	EItemQuality RollQuality(FName RecipeID, FName FacilityID = NAME_None);

	/**
	 * Roll with detailed result breakdown
	 */
	UFUNCTION(BlueprintCallable, Category = "Quality")
	FQualityRollResult RollQualityDetailed(FName RecipeID, FName FacilityID = NAME_None);

	/**
	 * Calculate expected quality (no randomness)
	 */
	UFUNCTION(BlueprintCallable, Category = "Quality")
	EItemQuality CalculateExpectedQuality(FName RecipeID, FName FacilityID = NAME_None) const;

	/**
	 * Get quality tier from score
	 */
	UFUNCTION(BlueprintCallable, Category = "Quality")
	EItemQuality GetQualityTierFromScore(float Score) const;

	/**
	 * Get quality score range for tier
	 */
	UFUNCTION(BlueprintCallable, Category = "Quality")
	FQualityTierConfig GetQualityTierConfig(EItemQuality Quality) const;

	// ============================================================================
	// Modifier Management
	// ============================================================================

	/**
	 * Calculate all quality modifiers
	 */
	UFUNCTION(BlueprintCallable, Category = "Modifiers")
	TArray<FQualityModifier> CalculateAllModifiers(FName RecipeID, FName FacilityID = NAME_None) const;

	/**
	 * Get skill-based quality modifier
	 */
	UFUNCTION(BlueprintCallable, Category = "Modifiers")
	float GetSkillQualityModifier() const;

	/**
	 * Get facility quality bonus
	 */
	UFUNCTION(BlueprintCallable, Category = "Modifiers")
	float GetFacilityQualityBonus(FName FacilityID) const;

	/**
	 * Get material quality contribution
	 */
	UFUNCTION(BlueprintCallable, Category = "Modifiers")
	float GetMaterialQualityBonus(const TArray<FCraftedItem>& InputMaterials) const;

	/**
	 * Add temporary quality bonus
	 */
	UFUNCTION(BlueprintCallable, Category = "Modifiers")
	void AddTemporaryBonus(FName BonusID, float Modifier, float Duration);

	/**
	 * Remove temporary bonus
	 */
	UFUNCTION(BlueprintCallable, Category = "Modifiers")
	void RemoveTemporaryBonus(FName BonusID);

	// ============================================================================
	// Value Calculation
	// ============================================================================

	/**
	 * Get value multiplier for quality
	 */
	UFUNCTION(BlueprintCallable, Category = "Value")
	float GetQualityValueMultiplier(EItemQuality Quality) const;

	/**
	 * Calculate item value based on quality
	 */
	UFUNCTION(BlueprintCallable, Category = "Value")
	int32 CalculateItemValue(const FCraftedItem& Item, int32 BaseValue) const;

	/**
	 * Get market demand modifier for quality
	 */
	UFUNCTION(BlueprintCallable, Category = "Value")
	FQualityMarketDemand GetMarketDemand(EItemQuality Quality) const;

	/**
	 * Calculate scarcity bonus for quality tier
	 */
	UFUNCTION(BlueprintCallable, Category = "Value")
	float GetScarcityBonus(EItemQuality Quality) const;

	// ============================================================================
	// Equipment Effects
	// ============================================================================

	/**
	 * Get equipment stat modifiers for quality
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	FQualityEquipmentEffect GetEquipmentEffects(EItemQuality Quality) const;

	/**
	 * Apply quality effects to weapon stats
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void ApplyQualityToWeaponStats(EItemQuality Quality, UPARAM(ref) float& Damage, UPARAM(ref) float& FireRate, UPARAM(ref) float& Range);

	/**
	 * Apply quality effects to armor stats
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void ApplyQualityToArmorStats(EItemQuality Quality, UPARAM(ref) float& Defense, UPARAM(ref) float& Durability, UPARAM(ref) float& Weight);

	/**
	 * Apply quality to ship module stats
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void ApplyQualityToModuleStats(EItemQuality Quality, UPARAM(ref) float& Efficiency, UPARAM(ref) float& PowerDraw, UPARAM(ref) int32& BonusCapacity);

	// ============================================================================
	// Item Inspection
	// ============================================================================

	/**
	 * Inspect item for quality details
	 */
	UFUNCTION(BlueprintCallable, Category = "Inspection")
	FQualityInspection InspectItem(const FCraftedItem& Item) const;

	/**
	 * Verify item authenticity
	 */
	UFUNCTION(BlueprintCallable, Category = "Inspection")
	float VerifyAuthenticity(const FCraftedItem& Item) const;

	/**
	 * Get quality tier display name
	 */
	UFUNCTION(BlueprintCallable, Category = "Inspection")
	FString GetQualityDisplayName(EItemQuality Quality) const;

	/**
	 * Get quality tier color
	 */
	UFUNCTION(BlueprintCallable, Category = "Inspection")
	FLinearColor GetQualityColor(EItemQuality Quality) const;

	// ============================================================================
	// Critical Crafts
	// ============================================================================

	/**
	 * Roll for critical craft
	 */
	UFUNCTION(BlueprintCallable, Category = "Critical")
	bool RollCriticalCraft(FName RecipeID) const;

	/**
	 * Get critical craft chance
	 */
	UFUNCTION(BlueprintCallable, Category = "Critical")
	float GetCriticalCraftChance(FName RecipeID) const;

	/**
	 * Get critical craft bonus quality tiers
	 */
	UFUNCTION(BlueprintCallable, Category = "Critical")
	int32 GetCriticalQualityBonus() const;

	// ============================================================================
	// Integration
	// ============================================================================

	/**
	 * Set skill system reference
	 */
	UFUNCTION(BlueprintCallable, Category = "Integration")
	void SetSkillSystem(UOdysseyCraftingSkillSystem* NewSkillSystem);

	/**
	 * Set crafting manager reference
	 */
	UFUNCTION(BlueprintCallable, Category = "Integration")
	void SetCraftingManager(UOdysseyCraftingManager* NewManager);

	// ============================================================================
	// Events
	// ============================================================================

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnQualityRolled OnQualityRolled;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCriticalCraft OnCriticalCraft;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnLegendaryCreated OnLegendaryCreated;

protected:
	// ============================================================================
	// Configuration
	// ============================================================================

	/**
	 * Quality tier configurations
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	TArray<FQualityTierConfig> QualityTiers;

	/**
	 * Equipment effects by quality
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	TArray<FQualityEquipmentEffect> EquipmentEffects;

	/**
	 * Market demand by quality
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	TArray<FQualityMarketDemand> MarketDemands;

	/**
	 * Base quality variance (randomness factor)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	float BaseQualityVariance;

	/**
	 * Critical craft base chance
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	float BaseCriticalChance;

	/**
	 * Quality tiers gained on critical
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	int32 CriticalQualityBonus;

	// ============================================================================
	// Runtime State
	// ============================================================================

	/**
	 * Active temporary bonuses
	 */
	TMap<FName, TPair<float, float>> TemporaryBonuses; // ID -> (Modifier, RemainingTime)

	/**
	 * Cached skill system
	 */
	UPROPERTY()
	UOdysseyCraftingSkillSystem* SkillSystem;

	/**
	 * Cached crafting manager
	 */
	UPROPERTY()
	UOdysseyCraftingManager* CraftingManager;

	// ============================================================================
	// Internal Methods
	// ============================================================================

	/**
	 * Initialize default tier configurations
	 */
	void InitializeDefaultTiers();

	/**
	 * Initialize default equipment effects
	 */
	void InitializeDefaultEquipmentEffects();

	/**
	 * Initialize default market demands
	 */
	void InitializeDefaultMarketDemands();

	/**
	 * Calculate base quality score
	 */
	float CalculateBaseQualityScore(FName RecipeID) const;

	/**
	 * Apply modifiers to base score
	 */
	float ApplyModifiers(float BaseScore, const TArray<FQualityModifier>& Modifiers) const;

	/**
	 * Get temporary bonus total
	 */
	float GetTemporaryBonusTotal() const;

	/**
	 * Update temporary bonuses
	 */
	void UpdateTemporaryBonuses(float DeltaTime);
};
