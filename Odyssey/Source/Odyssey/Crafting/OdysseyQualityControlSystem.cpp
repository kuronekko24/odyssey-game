// OdysseyQualityControlSystem.cpp
// Implementation of quality system for item value differentiation

#include "OdysseyQualityControlSystem.h"
#include "OdysseyCraftingSkillSystem.h"
#include "OdysseyCraftingManager.h"

UOdysseyQualityControlSystem::UOdysseyQualityControlSystem()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 1.0f; // Update bonuses once per second

	BaseQualityVariance = 0.15f;
	BaseCriticalChance = 0.05f;
	CriticalQualityBonus = 1;

	SkillSystem = nullptr;
	CraftingManager = nullptr;
}

void UOdysseyQualityControlSystem::BeginPlay()
{
	Super::BeginPlay();

	InitializeDefaultTiers();
	InitializeDefaultEquipmentEffects();
	InitializeDefaultMarketDemands();
}

void UOdysseyQualityControlSystem::InitializeDefaultTiers()
{
	if (QualityTiers.Num() > 0)
	{
		return; // Already configured
	}

	// Scrap
	{
		FQualityTierConfig Tier;
		Tier.Quality = EItemQuality::Scrap;
		Tier.MinScore = 0.0f;
		Tier.MaxScore = 0.15f;
		Tier.ValueMultiplier = 0.25f;
		Tier.StatBonus = -0.2f;
		Tier.TierColor = FLinearColor(0.4f, 0.4f, 0.4f);
		QualityTiers.Add(Tier);
	}

	// Common
	{
		FQualityTierConfig Tier;
		Tier.Quality = EItemQuality::Common;
		Tier.MinScore = 0.15f;
		Tier.MaxScore = 0.35f;
		Tier.ValueMultiplier = 1.0f;
		Tier.StatBonus = 0.0f;
		Tier.TierColor = FLinearColor::White;
		QualityTiers.Add(Tier);
	}

	// Standard
	{
		FQualityTierConfig Tier;
		Tier.Quality = EItemQuality::Standard;
		Tier.MinScore = 0.35f;
		Tier.MaxScore = 0.55f;
		Tier.ValueMultiplier = 1.5f;
		Tier.StatBonus = 0.1f;
		Tier.TierColor = FLinearColor(0.2f, 0.8f, 0.2f);
		QualityTiers.Add(Tier);
	}

	// Quality
	{
		FQualityTierConfig Tier;
		Tier.Quality = EItemQuality::Quality;
		Tier.MinScore = 0.55f;
		Tier.MaxScore = 0.72f;
		Tier.ValueMultiplier = 2.5f;
		Tier.StatBonus = 0.2f;
		Tier.TierColor = FLinearColor(0.2f, 0.5f, 1.0f);
		QualityTiers.Add(Tier);
	}

	// Superior
	{
		FQualityTierConfig Tier;
		Tier.Quality = EItemQuality::Superior;
		Tier.MinScore = 0.72f;
		Tier.MaxScore = 0.85f;
		Tier.ValueMultiplier = 4.0f;
		Tier.StatBonus = 0.35f;
		Tier.TierColor = FLinearColor(0.6f, 0.2f, 0.8f);
		QualityTiers.Add(Tier);
	}

	// Masterwork
	{
		FQualityTierConfig Tier;
		Tier.Quality = EItemQuality::Masterwork;
		Tier.MinScore = 0.85f;
		Tier.MaxScore = 0.95f;
		Tier.ValueMultiplier = 8.0f;
		Tier.StatBonus = 0.5f;
		Tier.TierColor = FLinearColor(1.0f, 0.6f, 0.0f);
		QualityTiers.Add(Tier);
	}

	// Legendary
	{
		FQualityTierConfig Tier;
		Tier.Quality = EItemQuality::Legendary;
		Tier.MinScore = 0.95f;
		Tier.MaxScore = 1.0f;
		Tier.ValueMultiplier = 20.0f;
		Tier.StatBonus = 0.75f;
		Tier.TierColor = FLinearColor(1.0f, 0.85f, 0.0f);
		QualityTiers.Add(Tier);
	}
}

void UOdysseyQualityControlSystem::InitializeDefaultEquipmentEffects()
{
	if (EquipmentEffects.Num() > 0)
	{
		return;
	}

	for (int32 i = 0; i <= static_cast<int32>(EItemQuality::Legendary); ++i)
	{
		FQualityEquipmentEffect Effect;
		Effect.Quality = static_cast<EItemQuality>(i);

		float Tier = static_cast<float>(i);
		Effect.DamageMultiplier = 0.7f + (Tier * 0.15f);
		Effect.DefenseMultiplier = 0.7f + (Tier * 0.15f);
		Effect.DurabilityMultiplier = 0.5f + (Tier * 0.2f);
		Effect.BonusSlots = FMath::FloorToInt(Tier / 2.0f);

		EquipmentEffects.Add(Effect);
	}
}

void UOdysseyQualityControlSystem::InitializeDefaultMarketDemands()
{
	if (MarketDemands.Num() > 0)
	{
		return;
	}

	for (int32 i = 0; i <= static_cast<int32>(EItemQuality::Legendary); ++i)
	{
		FQualityMarketDemand Demand;
		Demand.Quality = static_cast<EItemQuality>(i);

		float Tier = static_cast<float>(i);
		Demand.DemandMultiplier = 1.0f + (Tier * 0.3f);
		Demand.PriceMultiplier = FMath::Pow(1.5f, Tier);
		Demand.SupplyScarcity = FMath::Pow(2.0f, Tier);

		MarketDemands.Add(Demand);
	}
}

// ============================================================================
// Quality Calculation
// ============================================================================

EItemQuality UOdysseyQualityControlSystem::RollQuality(FName RecipeID, FName FacilityID)
{
	FQualityRollResult Result = RollQualityDetailed(RecipeID, FacilityID);
	return Result.ResultQuality;
}

FQualityRollResult UOdysseyQualityControlSystem::RollQualityDetailed(FName RecipeID, FName FacilityID)
{
	FQualityRollResult Result;

	// Calculate base score
	Result.BaseScore = CalculateBaseQualityScore(RecipeID);

	// Get all modifiers
	Result.AppliedModifiers = CalculateAllModifiers(RecipeID, FacilityID);

	// Apply modifiers
	Result.FinalScore = ApplyModifiers(Result.BaseScore, Result.AppliedModifiers);

	// Add randomness
	float Variance = FMath::FRandRange(-BaseQualityVariance, BaseQualityVariance);
	Result.FinalScore = FMath::Clamp(Result.FinalScore + Variance, 0.0f, 1.0f);

	// Check for critical craft
	Result.bWasCritical = RollCriticalCraft(RecipeID);
	if (Result.bWasCritical)
	{
		// Boost quality score on critical
		Result.FinalScore = FMath::Min(Result.FinalScore + 0.2f, 1.0f);
	}

	// Determine quality tier
	Result.ResultQuality = GetQualityTierFromScore(Result.FinalScore);

	// Apply critical bonus to quality tier
	if (Result.bWasCritical)
	{
		int32 CurrentTier = static_cast<int32>(Result.ResultQuality);
		int32 NewTier = FMath::Min(CurrentTier + CriticalQualityBonus, static_cast<int32>(EItemQuality::Legendary));
		Result.ResultQuality = static_cast<EItemQuality>(NewTier);

		OnCriticalCraft.Broadcast(RecipeID, Result.ResultQuality, Result.FinalScore);
	}

	// Get value and stat multipliers
	FQualityTierConfig TierConfig = GetQualityTierConfig(Result.ResultQuality);
	Result.ValueMultiplier = TierConfig.ValueMultiplier;
	Result.StatBonus = TierConfig.StatBonus;

	// Broadcast result
	OnQualityRolled.Broadcast(Result, RecipeID);

	// Special notification for legendary
	if (Result.ResultQuality == EItemQuality::Legendary)
	{
		FCraftedItem LegendaryItem;
		LegendaryItem.Quality = EItemQuality::Legendary;
		LegendaryItem.QualityMultiplier = Result.ValueMultiplier;
		OnLegendaryCreated.Broadcast(RecipeID, LegendaryItem);
	}

	return Result;
}

EItemQuality UOdysseyQualityControlSystem::CalculateExpectedQuality(FName RecipeID, FName FacilityID) const
{
	float BaseScore = CalculateBaseQualityScore(RecipeID);
	TArray<FQualityModifier> Modifiers = CalculateAllModifiers(RecipeID, FacilityID);
	float FinalScore = ApplyModifiers(BaseScore, Modifiers);

	return GetQualityTierFromScore(FinalScore);
}

EItemQuality UOdysseyQualityControlSystem::GetQualityTierFromScore(float Score) const
{
	for (const FQualityTierConfig& Tier : QualityTiers)
	{
		if (Score >= Tier.MinScore && Score < Tier.MaxScore)
		{
			return Tier.Quality;
		}
	}

	// Default to highest tier if score >= 1.0
	if (Score >= 0.95f)
	{
		return EItemQuality::Legendary;
	}

	return EItemQuality::Common;
}

FQualityTierConfig UOdysseyQualityControlSystem::GetQualityTierConfig(EItemQuality Quality) const
{
	for (const FQualityTierConfig& Tier : QualityTiers)
	{
		if (Tier.Quality == Quality)
		{
			return Tier;
		}
	}

	return FQualityTierConfig();
}

// ============================================================================
// Modifier Management
// ============================================================================

TArray<FQualityModifier> UOdysseyQualityControlSystem::CalculateAllModifiers(FName RecipeID, FName FacilityID) const
{
	TArray<FQualityModifier> Modifiers;

	// Skill modifier
	float SkillMod = GetSkillQualityModifier();
	if (SkillMod != 0.0f)
	{
		Modifiers.Add(FQualityModifier(EQualityModifierSource::Skill, TEXT("Crafting Skill"), SkillMod));
	}

	// Facility modifier
	float FacilityMod = GetFacilityQualityBonus(FacilityID);
	if (FacilityMod != 0.0f)
	{
		Modifiers.Add(FQualityModifier(EQualityModifierSource::Facility, TEXT("Facility Bonus"), FacilityMod));
	}

	// Temporary bonuses
	float TempBonus = GetTemporaryBonusTotal();
	if (TempBonus != 0.0f)
	{
		Modifiers.Add(FQualityModifier(EQualityModifierSource::Catalyst, TEXT("Temporary Bonuses"), TempBonus));
	}

	return Modifiers;
}

float UOdysseyQualityControlSystem::GetSkillQualityModifier() const
{
	if (SkillSystem)
	{
		return SkillSystem->GetCraftingQualityBonus();
	}
	return 0.0f;
}

float UOdysseyQualityControlSystem::GetFacilityQualityBonus(FName FacilityID) const
{
	if (CraftingManager && !FacilityID.IsNone())
	{
		FCraftingFacility Facility = CraftingManager->GetFacility(FacilityID);
		return Facility.QualityBonus;
	}
	return 0.0f;
}

float UOdysseyQualityControlSystem::GetMaterialQualityBonus(const TArray<FCraftedItem>& InputMaterials) const
{
	if (InputMaterials.Num() == 0)
	{
		return 0.0f;
	}

	float TotalQualityScore = 0.0f;
	for (const FCraftedItem& Material : InputMaterials)
	{
		// Convert quality tier to score
		float MaterialScore = static_cast<float>(Material.Quality) / 6.0f;
		TotalQualityScore += MaterialScore;
	}

	float AverageQuality = TotalQualityScore / InputMaterials.Num();

	// Materials affect quality by up to 20%
	return (AverageQuality - 0.5f) * 0.4f;
}

void UOdysseyQualityControlSystem::AddTemporaryBonus(FName BonusID, float Modifier, float Duration)
{
	TemporaryBonuses.Add(BonusID, TPair<float, float>(Modifier, Duration));
}

void UOdysseyQualityControlSystem::RemoveTemporaryBonus(FName BonusID)
{
	TemporaryBonuses.Remove(BonusID);
}

// ============================================================================
// Value Calculation
// ============================================================================

float UOdysseyQualityControlSystem::GetQualityValueMultiplier(EItemQuality Quality) const
{
	FQualityTierConfig Config = GetQualityTierConfig(Quality);
	return Config.ValueMultiplier;
}

int32 UOdysseyQualityControlSystem::CalculateItemValue(const FCraftedItem& Item, int32 BaseValue) const
{
	float Multiplier = GetQualityValueMultiplier(Item.Quality);

	// Apply additional item-specific multipliers
	Multiplier *= Item.QualityMultiplier;

	// Apply scarcity bonus
	float Scarcity = GetScarcityBonus(Item.Quality);
	Multiplier *= (1.0f + Scarcity * 0.1f);

	return FMath::CeilToInt(BaseValue * Multiplier);
}

FQualityMarketDemand UOdysseyQualityControlSystem::GetMarketDemand(EItemQuality Quality) const
{
	for (const FQualityMarketDemand& Demand : MarketDemands)
	{
		if (Demand.Quality == Quality)
		{
			return Demand;
		}
	}

	return FQualityMarketDemand();
}

float UOdysseyQualityControlSystem::GetScarcityBonus(EItemQuality Quality) const
{
	FQualityMarketDemand Demand = GetMarketDemand(Quality);
	return Demand.SupplyScarcity - 1.0f;
}

// ============================================================================
// Equipment Effects
// ============================================================================

FQualityEquipmentEffect UOdysseyQualityControlSystem::GetEquipmentEffects(EItemQuality Quality) const
{
	for (const FQualityEquipmentEffect& Effect : EquipmentEffects)
	{
		if (Effect.Quality == Quality)
		{
			return Effect;
		}
	}

	return FQualityEquipmentEffect();
}

void UOdysseyQualityControlSystem::ApplyQualityToWeaponStats(EItemQuality Quality, float& Damage, float& FireRate, float& Range)
{
	FQualityEquipmentEffect Effect = GetEquipmentEffects(Quality);
	FQualityTierConfig Config = GetQualityTierConfig(Quality);

	Damage *= Effect.DamageMultiplier;
	FireRate *= (1.0f + Config.StatBonus * 0.5f);
	Range *= (1.0f + Config.StatBonus * 0.3f);
}

void UOdysseyQualityControlSystem::ApplyQualityToArmorStats(EItemQuality Quality, float& Defense, float& Durability, float& Weight)
{
	FQualityEquipmentEffect Effect = GetEquipmentEffects(Quality);
	FQualityTierConfig Config = GetQualityTierConfig(Quality);

	Defense *= Effect.DefenseMultiplier;
	Durability *= Effect.DurabilityMultiplier;
	Weight *= (1.0f - Config.StatBonus * 0.2f); // Higher quality = lighter
}

void UOdysseyQualityControlSystem::ApplyQualityToModuleStats(EItemQuality Quality, float& Efficiency, float& PowerDraw, int32& BonusCapacity)
{
	FQualityEquipmentEffect Effect = GetEquipmentEffects(Quality);
	FQualityTierConfig Config = GetQualityTierConfig(Quality);

	Efficiency *= (1.0f + Config.StatBonus);
	PowerDraw *= (1.0f - Config.StatBonus * 0.3f); // Higher quality = more efficient
	BonusCapacity += Effect.BonusSlots;
}

// ============================================================================
// Item Inspection
// ============================================================================

FQualityInspection UOdysseyQualityControlSystem::InspectItem(const FCraftedItem& Item) const
{
	FQualityInspection Inspection;

	Inspection.Quality = Item.Quality;
	Inspection.QualityScore = Item.QualityMultiplier;
	Inspection.CraftDate = Item.CraftedTime;
	Inspection.Authenticity = VerifyAuthenticity(Item);

	// Estimate value
	Inspection.EstimatedValue = CalculateItemValue(Item, 100);

	// Generate quality notes
	FQualityTierConfig Config = GetQualityTierConfig(Item.Quality);

	if (Item.Quality >= EItemQuality::Masterwork)
	{
		Inspection.QualityNotes.Add(TEXT("Exceptional craftsmanship"));
		Inspection.QualityNotes.Add(TEXT("Highly sought after by collectors"));
	}
	else if (Item.Quality >= EItemQuality::Superior)
	{
		Inspection.QualityNotes.Add(TEXT("Above average quality"));
		Inspection.QualityNotes.Add(TEXT("Premium market value"));
	}
	else if (Item.Quality <= EItemQuality::Scrap)
	{
		Inspection.QualityNotes.Add(TEXT("Poor quality, may be salvaged for materials"));
	}

	if (Item.Durability > 90.0f)
	{
		Inspection.QualityNotes.Add(TEXT("Excellent condition"));
	}
	else if (Item.Durability < 50.0f)
	{
		Inspection.QualityNotes.Add(TEXT("Showing signs of wear"));
	}

	return Inspection;
}

float UOdysseyQualityControlSystem::VerifyAuthenticity(const FCraftedItem& Item) const
{
	// Simple authenticity check based on item data consistency
	float Authenticity = 1.0f;

	// Check for valid crafter ID
	if (Item.CrafterID.IsNone())
	{
		Authenticity -= 0.2f;
	}

	// Check for valid craft time
	if (Item.CraftedTime > FDateTime::Now())
	{
		Authenticity -= 0.5f;
	}

	// Check quality multiplier consistency
	FQualityTierConfig Config = GetQualityTierConfig(Item.Quality);
	float ExpectedMultiplier = Config.ValueMultiplier;
	if (FMath::Abs(Item.QualityMultiplier - ExpectedMultiplier) > 0.5f)
	{
		Authenticity -= 0.3f;
	}

	return FMath::Max(Authenticity, 0.0f);
}

FString UOdysseyQualityControlSystem::GetQualityDisplayName(EItemQuality Quality) const
{
	switch (Quality)
	{
	case EItemQuality::Scrap:       return TEXT("Scrap");
	case EItemQuality::Common:      return TEXT("Common");
	case EItemQuality::Standard:    return TEXT("Standard");
	case EItemQuality::Quality:     return TEXT("Quality");
	case EItemQuality::Superior:    return TEXT("Superior");
	case EItemQuality::Masterwork:  return TEXT("Masterwork");
	case EItemQuality::Legendary:   return TEXT("Legendary");
	default:                        return TEXT("Unknown");
	}
}

FLinearColor UOdysseyQualityControlSystem::GetQualityColor(EItemQuality Quality) const
{
	FQualityTierConfig Config = GetQualityTierConfig(Quality);
	return Config.TierColor;
}

// ============================================================================
// Critical Crafts
// ============================================================================

bool UOdysseyQualityControlSystem::RollCriticalCraft(FName RecipeID) const
{
	float CritChance = GetCriticalCraftChance(RecipeID);
	return FMath::FRand() < CritChance;
}

float UOdysseyQualityControlSystem::GetCriticalCraftChance(FName RecipeID) const
{
	float CritChance = BaseCriticalChance;

	// Add skill bonus
	if (SkillSystem)
	{
		float SkillBonus = SkillSystem->GetSkillLevel(FName(TEXT("Precision"))) * 0.02f;
		CritChance += SkillBonus;
	}

	return FMath::Min(CritChance, 0.25f);
}

int32 UOdysseyQualityControlSystem::GetCriticalQualityBonus() const
{
	return CriticalQualityBonus;
}

// ============================================================================
// Integration
// ============================================================================

void UOdysseyQualityControlSystem::SetSkillSystem(UOdysseyCraftingSkillSystem* NewSkillSystem)
{
	SkillSystem = NewSkillSystem;
}

void UOdysseyQualityControlSystem::SetCraftingManager(UOdysseyCraftingManager* NewManager)
{
	CraftingManager = NewManager;
}

// ============================================================================
// Internal Methods
// ============================================================================

float UOdysseyQualityControlSystem::CalculateBaseQualityScore(FName RecipeID) const
{
	// Base score starts at 0.35 (common-standard range)
	float BaseScore = 0.35f;

	// Recipe complexity affects base quality
	if (CraftingManager)
	{
		FAdvancedCraftingRecipe Recipe = CraftingManager->GetRecipe(RecipeID);
		BaseScore += Recipe.BaseQualityChance * 0.3f;
	}

	return BaseScore;
}

float UOdysseyQualityControlSystem::ApplyModifiers(float BaseScore, const TArray<FQualityModifier>& Modifiers) const
{
	float AdditiveTotal = 0.0f;
	float MultiplicativeTotal = 1.0f;

	for (const FQualityModifier& Mod : Modifiers)
	{
		if (Mod.bIsMultiplicative)
		{
			MultiplicativeTotal *= (1.0f + Mod.Modifier);
		}
		else
		{
			AdditiveTotal += Mod.Modifier;
		}
	}

	float FinalScore = (BaseScore + AdditiveTotal) * MultiplicativeTotal;
	return FMath::Clamp(FinalScore, 0.0f, 1.0f);
}

float UOdysseyQualityControlSystem::GetTemporaryBonusTotal() const
{
	float Total = 0.0f;
	for (const auto& Pair : TemporaryBonuses)
	{
		Total += Pair.Value.Key;
	}
	return Total;
}

void UOdysseyQualityControlSystem::UpdateTemporaryBonuses(float DeltaTime)
{
	TArray<FName> ExpiredBonuses;

	for (auto& Pair : TemporaryBonuses)
	{
		Pair.Value.Value -= DeltaTime;
		if (Pair.Value.Value <= 0.0f)
		{
			ExpiredBonuses.Add(Pair.Key);
		}
	}

	for (const FName& BonusID : ExpiredBonuses)
	{
		TemporaryBonuses.Remove(BonusID);
	}
}
