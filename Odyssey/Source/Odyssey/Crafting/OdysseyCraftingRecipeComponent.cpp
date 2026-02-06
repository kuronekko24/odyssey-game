// OdysseyCraftingRecipeComponent.cpp
// Implementation of dynamic recipe system

#include "OdysseyCraftingRecipeComponent.h"
#include "OdysseyCraftingSkillSystem.h"
#include "OdysseyCraftingManager.h"

UOdysseyCraftingRecipeComponent::UOdysseyCraftingRecipeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.5f; // Update research twice per second

	MaxConcurrentResearch = 3;
	BaseExperimentationChance = 0.15f;
	ResearchSpeedMultiplier = 1.0f;

	SkillSystem = nullptr;
	CraftingManager = nullptr;
}

void UOdysseyCraftingRecipeComponent::BeginPlay()
{
	Super::BeginPlay();

	LoadBlueprints();
	LoadVariations();
}

void UOdysseyCraftingRecipeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ProcessActiveResearch(DeltaTime);
}

// ============================================================================
// Recipe Variation System
// ============================================================================

TArray<FRecipeVariation> UOdysseyCraftingRecipeComponent::GetRecipeVariations(FName RecipeID) const
{
	TArray<FRecipeVariation> Variations;

	if (!VariationDataTable)
	{
		return Variations;
	}

	TArray<FName> RowNames = VariationDataTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		FString RowString = RowName.ToString();
		if (RowString.StartsWith(RecipeID.ToString()))
		{
			FRecipeVariation* Variation = VariationDataTable->FindRow<FRecipeVariation>(RowName, TEXT("GetRecipeVariations"));
			if (Variation)
			{
				// Check if discovered
				const TArray<FName>* DiscoveredList = DiscoveredVariations.Find(RecipeID);
				Variation->bIsDiscovered = DiscoveredList && DiscoveredList->Contains(Variation->VariationID);
				Variations.Add(*Variation);
			}
		}
	}

	return Variations;
}

FRecipeVariation UOdysseyCraftingRecipeComponent::GetVariation(FName RecipeID, FName VariationID) const
{
	FRecipeVariation* Variation = FindVariationData(RecipeID, VariationID);
	if (Variation)
	{
		FRecipeVariation Result = *Variation;
		const TArray<FName>* DiscoveredList = DiscoveredVariations.Find(RecipeID);
		Result.bIsDiscovered = DiscoveredList && DiscoveredList->Contains(VariationID);
		return Result;
	}
	return FRecipeVariation();
}

bool UOdysseyCraftingRecipeComponent::IsVariationDiscovered(FName RecipeID, FName VariationID) const
{
	const TArray<FName>* DiscoveredList = DiscoveredVariations.Find(RecipeID);
	return DiscoveredList && DiscoveredList->Contains(VariationID);
}

bool UOdysseyCraftingRecipeComponent::DiscoverVariation(FName RecipeID, FName VariationID)
{
	if (IsVariationDiscovered(RecipeID, VariationID))
	{
		return false;
	}

	FRecipeVariation* Variation = FindVariationData(RecipeID, VariationID);
	if (!Variation)
	{
		return false;
	}

	// Check skill requirements
	if (SkillSystem && Variation->RequiredSkillLevel > 0)
	{
		// Get primary skill for recipe category
		if (CraftingManager)
		{
			FAdvancedCraftingRecipe Recipe = CraftingManager->GetRecipe(RecipeID);
			for (const auto& SkillReq : Recipe.RequiredSkillLevels)
			{
				if (SkillSystem->GetSkillLevel(SkillReq.Key) < Variation->RequiredSkillLevel)
				{
					return false;
				}
			}
		}
	}

	TArray<FName>& DiscoveredList = DiscoveredVariations.FindOrAdd(RecipeID);
	DiscoveredList.Add(VariationID);

	OnRecipeDiscovered.Broadcast(RecipeID, VariationID);

	return true;
}

FAdvancedCraftingRecipe UOdysseyCraftingRecipeComponent::GetEffectiveRecipe(FName RecipeID, FName VariationID) const
{
	if (!CraftingManager)
	{
		return FAdvancedCraftingRecipe();
	}

	FAdvancedCraftingRecipe BaseRecipe = CraftingManager->GetRecipe(RecipeID);
	if (BaseRecipe.RecipeID.IsNone())
	{
		return BaseRecipe;
	}

	// If no variation or variation not discovered, return base
	if (VariationID.IsNone() || !IsVariationDiscovered(RecipeID, VariationID))
	{
		return BaseRecipe;
	}

	FRecipeVariation Variation = GetVariation(RecipeID, VariationID);
	if (Variation.VariationID.IsNone())
	{
		return BaseRecipe;
	}

	// Apply variation modifications
	FAdvancedCraftingRecipe ModifiedRecipe = BaseRecipe;

	// Replace ingredients if specified
	if (Variation.AlternativeIngredients.Num() > 0)
	{
		ModifiedRecipe.PrimaryIngredients = Variation.AlternativeIngredients;
	}

	// Replace outputs if specified
	if (Variation.ModifiedOutputs.Num() > 0)
	{
		ModifiedRecipe.PrimaryOutputs = Variation.ModifiedOutputs;
	}

	// Apply modifiers
	ModifiedRecipe.BaseCraftingTime *= Variation.TimeModifier;
	ModifiedRecipe.BaseQualityChance *= Variation.QualityModifier;

	// Update recipe name to indicate variation
	ModifiedRecipe.RecipeName = FString::Printf(TEXT("%s (%s)"), *BaseRecipe.RecipeName, *Variation.VariationName);

	return ModifiedRecipe;
}

FName UOdysseyCraftingRecipeComponent::AttemptExperimentation(FName RecipeID, const TArray<FCraftingIngredient>& ExperimentalIngredients)
{
	if (ExperimentalIngredients.Num() == 0)
	{
		return NAME_None;
	}

	// Calculate hash to track experimentation history
	uint32 ExperimentHash = CalculateExperimentationHash(ExperimentalIngredients);

	// Check if we've already tried this combination
	TSet<uint32>& History = ExperimentationHistory.FindOrAdd(RecipeID);
	if (History.Contains(ExperimentHash))
	{
		return NAME_None; // Already tried this combination
	}
	History.Add(ExperimentHash);

	// Calculate discovery chance
	float DiscoveryChance = BaseExperimentationChance;

	// Skill bonus
	if (SkillSystem)
	{
		float SkillBonus = SkillSystem->GetSkillLevel(FName(TEXT("Research"))) * 0.02f;
		DiscoveryChance += SkillBonus;
	}

	// More ingredients = higher chance
	DiscoveryChance += ExperimentalIngredients.Num() * 0.05f;

	// Roll for discovery
	float Roll = FMath::FRand();
	if (Roll > DiscoveryChance)
	{
		return NAME_None;
	}

	// Find a matching undiscovered variation
	TArray<FRecipeVariation> Variations = GetRecipeVariations(RecipeID);
	for (const FRecipeVariation& Variation : Variations)
	{
		if (!Variation.bIsDiscovered)
		{
			// Check if experimental ingredients match this variation
			bool bMatches = true;
			for (const FCraftingIngredient& AltIngredient : Variation.AlternativeIngredients)
			{
				bool bFound = false;
				for (const FCraftingIngredient& ExpIngredient : ExperimentalIngredients)
				{
					if (ExpIngredient.ResourceType == AltIngredient.ResourceType)
					{
						bFound = true;
						break;
					}
				}
				if (!bFound)
				{
					bMatches = false;
					break;
				}
			}

			if (bMatches || FMath::FRand() < 0.3f) // 30% chance to discover anyway
			{
				DiscoverVariation(RecipeID, Variation.VariationID);
				return Variation.VariationID;
			}
		}
	}

	return NAME_None;
}

// ============================================================================
// Blueprint Research System
// ============================================================================

TArray<FCraftingBlueprint> UOdysseyCraftingRecipeComponent::GetAllBlueprints() const
{
	TArray<FCraftingBlueprint> AllBlueprints;

	if (BlueprintDataTable)
	{
		TArray<FName> RowNames = BlueprintDataTable->GetRowNames();
		for (const FName& RowName : RowNames)
		{
			FCraftingBlueprint* Blueprint = BlueprintDataTable->FindRow<FCraftingBlueprint>(RowName, TEXT("GetAllBlueprints"));
			if (Blueprint)
			{
				Blueprint->bIsResearched = ResearchedBlueprints.Contains(Blueprint->BlueprintID);
				AllBlueprints.Add(*Blueprint);
			}
		}
	}

	// Add custom blueprints
	for (const auto& Pair : CustomBlueprints)
	{
		FCraftingBlueprint Blueprint = Pair.Value;
		Blueprint.bIsResearched = ResearchedBlueprints.Contains(Blueprint.BlueprintID);
		AllBlueprints.Add(Blueprint);
	}

	return AllBlueprints;
}

FCraftingBlueprint UOdysseyCraftingRecipeComponent::GetBlueprint(FName BlueprintID) const
{
	// Check custom blueprints first
	const FCraftingBlueprint* CustomBlueprint = CustomBlueprints.Find(BlueprintID);
	if (CustomBlueprint)
	{
		FCraftingBlueprint Result = *CustomBlueprint;
		Result.bIsResearched = ResearchedBlueprints.Contains(BlueprintID);
		return Result;
	}

	// Check data table
	if (BlueprintDataTable)
	{
		FCraftingBlueprint* Blueprint = BlueprintDataTable->FindRow<FCraftingBlueprint>(BlueprintID, TEXT("GetBlueprint"));
		if (Blueprint)
		{
			FCraftingBlueprint Result = *Blueprint;
			Result.bIsResearched = ResearchedBlueprints.Contains(BlueprintID);
			return Result;
		}
	}

	return FCraftingBlueprint();
}

bool UOdysseyCraftingRecipeComponent::IsBlueprintResearched(FName BlueprintID) const
{
	return ResearchedBlueprints.Contains(BlueprintID);
}

bool UOdysseyCraftingRecipeComponent::CanResearchBlueprint(FName BlueprintID) const
{
	if (ResearchedBlueprints.Contains(BlueprintID))
	{
		return false; // Already researched
	}

	FCraftingBlueprint Blueprint = GetBlueprint(BlueprintID);
	if (Blueprint.BlueprintID.IsNone())
	{
		return false;
	}

	// Check prerequisites
	for (const FName& PrereqID : Blueprint.PrerequisiteBlueprints)
	{
		if (!ResearchedBlueprints.Contains(PrereqID))
		{
			return false;
		}
	}

	// Check skill requirements
	if (SkillSystem)
	{
		for (const auto& SkillReq : Blueprint.RequiredSkillLevels)
		{
			if (SkillSystem->GetSkillLevel(SkillReq.Key) < SkillReq.Value)
			{
				return false;
			}
		}
	}

	// Check concurrent research limit
	if (ActiveResearchProjects.Num() >= MaxConcurrentResearch)
	{
		return false;
	}

	return true;
}

bool UOdysseyCraftingRecipeComponent::StartBlueprintResearch(FName BlueprintID)
{
	if (!CanResearchBlueprint(BlueprintID))
	{
		return false;
	}

	FCraftingBlueprint Blueprint = GetBlueprint(BlueprintID);

	FBlueprintResearchProgress NewResearch;
	NewResearch.BlueprintID = BlueprintID;
	NewResearch.Progress = 0.0f;
	NewResearch.RemainingTime = Blueprint.ResearchTime / ResearchSpeedMultiplier;
	NewResearch.bIsPaused = false;

	ActiveResearchProjects.Add(NewResearch);

	return true;
}

bool UOdysseyCraftingRecipeComponent::CancelBlueprintResearch(FName BlueprintID)
{
	for (int32 i = 0; i < ActiveResearchProjects.Num(); ++i)
	{
		if (ActiveResearchProjects[i].BlueprintID == BlueprintID)
		{
			ActiveResearchProjects.RemoveAt(i);
			return true;
		}
	}
	return false;
}

bool UOdysseyCraftingRecipeComponent::SetResearchPaused(FName BlueprintID, bool bPaused)
{
	for (FBlueprintResearchProgress& Research : ActiveResearchProjects)
	{
		if (Research.BlueprintID == BlueprintID)
		{
			Research.bIsPaused = bPaused;
			return true;
		}
	}
	return false;
}

FBlueprintResearchProgress UOdysseyCraftingRecipeComponent::GetResearchProgress(FName BlueprintID) const
{
	for (const FBlueprintResearchProgress& Research : ActiveResearchProjects)
	{
		if (Research.BlueprintID == BlueprintID)
		{
			return Research;
		}
	}
	return FBlueprintResearchProgress();
}

TArray<FBlueprintResearchProgress> UOdysseyCraftingRecipeComponent::GetActiveResearch() const
{
	return ActiveResearchProjects;
}

bool UOdysseyCraftingRecipeComponent::AddBlueprint(const FCraftingBlueprint& Blueprint)
{
	if (Blueprint.BlueprintID.IsNone())
	{
		return false;
	}

	if (CustomBlueprints.Contains(Blueprint.BlueprintID))
	{
		return false;
	}

	CustomBlueprints.Add(Blueprint.BlueprintID, Blueprint);
	return true;
}

// ============================================================================
// Efficiency Calculations
// ============================================================================

FRecipeEfficiencyModifiers UOdysseyCraftingRecipeComponent::CalculateEfficiencyModifiers(FName RecipeID, FName FacilityID) const
{
	FRecipeEfficiencyModifiers Modifiers;

	// Skill bonuses
	if (SkillSystem)
	{
		Modifiers.SkillSpeedBonus = SkillSystem->GetCraftingSpeedBonus();
		Modifiers.SkillQualityBonus = SkillSystem->GetCraftingQualityBonus();
	}

	// Facility bonuses
	if (CraftingManager && !FacilityID.IsNone())
	{
		FCraftingFacility Facility = CraftingManager->GetFacility(FacilityID);
		Modifiers.FacilitySpeedBonus = Facility.SpeedMultiplier - 1.0f;
		Modifiers.FacilityQualityBonus = Facility.QualityBonus;
	}

	// Material efficiency from skills
	Modifiers.MaterialEfficiency = GetMaterialEfficiency(RecipeID);
	Modifiers.BonusOutputChance = GetBonusOutputChance(RecipeID);
	Modifiers.CriticalCraftChance = GetCriticalCraftChance(RecipeID);

	return Modifiers;
}

float UOdysseyCraftingRecipeComponent::GetMaterialEfficiency(FName RecipeID) const
{
	float Efficiency = 1.0f;

	if (SkillSystem)
	{
		// Material efficiency skill reduces input requirements
		int32 EfficiencySkill = SkillSystem->GetSkillLevel(FName(TEXT("MaterialEfficiency")));
		Efficiency -= EfficiencySkill * 0.02f; // 2% reduction per level

		// Cap at 50% reduction
		Efficiency = FMath::Max(Efficiency, 0.5f);
	}

	return Efficiency;
}

float UOdysseyCraftingRecipeComponent::GetBonusOutputChance(FName RecipeID) const
{
	float BonusChance = 0.0f;

	if (SkillSystem)
	{
		// Bonus output chance from crafting mastery
		int32 MasteryLevel = SkillSystem->GetSkillLevel(FName(TEXT("CraftingMastery")));
		BonusChance += MasteryLevel * 0.03f; // 3% per level
	}

	return FMath::Min(BonusChance, 0.5f); // Cap at 50%
}

float UOdysseyCraftingRecipeComponent::GetCriticalCraftChance(FName RecipeID) const
{
	float CritChance = 0.05f; // Base 5%

	if (SkillSystem)
	{
		// Critical craft chance from precision skill
		int32 PrecisionLevel = SkillSystem->GetSkillLevel(FName(TEXT("Precision")));
		CritChance += PrecisionLevel * 0.02f; // 2% per level
	}

	return FMath::Min(CritChance, 0.25f); // Cap at 25%
}

TArray<FCraftingIngredient> UOdysseyCraftingRecipeComponent::GetEfficientIngredients(FName RecipeID, int32 Quantity) const
{
	TArray<FCraftingIngredient> EfficientIngredients;

	if (!CraftingManager)
	{
		return EfficientIngredients;
	}

	FAdvancedCraftingRecipe Recipe = CraftingManager->GetRecipe(RecipeID);
	float Efficiency = GetMaterialEfficiency(RecipeID);

	for (const FCraftingIngredient& BaseIngredient : Recipe.PrimaryIngredients)
	{
		FCraftingIngredient EfficientIngredient = BaseIngredient;
		EfficientIngredient.Amount = FMath::CeilToInt(BaseIngredient.Amount * Quantity * Efficiency);
		EfficientIngredients.Add(EfficientIngredient);
	}

	return EfficientIngredients;
}

// ============================================================================
// Recipe Information
// ============================================================================

float UOdysseyCraftingRecipeComponent::GetRecipeDifficulty(FName RecipeID) const
{
	if (!CraftingManager)
	{
		return 1.0f;
	}

	FAdvancedCraftingRecipe Recipe = CraftingManager->GetRecipe(RecipeID);

	// Calculate difficulty based on multiple factors
	float Difficulty = 0.0f;

	// Tier contribution
	Difficulty += static_cast<float>(Recipe.RequiredTier) * 0.15f;

	// Skill requirements
	for (const auto& SkillReq : Recipe.RequiredSkillLevels)
	{
		Difficulty += SkillReq.Value * 0.05f;
	}

	// Ingredient complexity
	Difficulty += Recipe.PrimaryIngredients.Num() * 0.1f;

	// Chain depth
	Difficulty += Recipe.ChainDepth * 0.08f;

	return FMath::Clamp(Difficulty, 0.1f, 1.0f);
}

float UOdysseyCraftingRecipeComponent::GetRecipeProfitMargin(FName RecipeID) const
{
	// This would integrate with the economy system
	// Placeholder implementation
	if (!CraftingManager)
	{
		return 0.0f;
	}

	FAdvancedCraftingRecipe Recipe = CraftingManager->GetRecipe(RecipeID);

	// Basic profit estimation based on tier and complexity
	float BaseProfit = 10.0f;
	BaseProfit += static_cast<float>(Recipe.RequiredTier) * 5.0f;
	BaseProfit += Recipe.ChainDepth * 3.0f;

	return BaseProfit;
}

int32 UOdysseyCraftingRecipeComponent::GetRecommendedSkillLevel(FName RecipeID) const
{
	if (!CraftingManager)
	{
		return 1;
	}

	FAdvancedCraftingRecipe Recipe = CraftingManager->GetRecipe(RecipeID);

	int32 MaxRequired = 1;
	for (const auto& SkillReq : Recipe.RequiredSkillLevels)
	{
		MaxRequired = FMath::Max(MaxRequired, SkillReq.Value);
	}

	// Recommend 2 levels above minimum for comfortable crafting
	return MaxRequired + 2;
}

TArray<FName> UOdysseyCraftingRecipeComponent::GetRecipesUsingIngredient(EResourceType ResourceType) const
{
	TArray<FName> MatchingRecipes;

	if (!CraftingManager)
	{
		return MatchingRecipes;
	}

	TArray<FAdvancedCraftingRecipe> AllRecipes = CraftingManager->GetAllRecipes();
	for (const FAdvancedCraftingRecipe& Recipe : AllRecipes)
	{
		for (const FCraftingIngredient& Ingredient : Recipe.PrimaryIngredients)
		{
			if (Ingredient.ResourceType == ResourceType)
			{
				MatchingRecipes.Add(Recipe.RecipeID);
				break;
			}
		}
	}

	return MatchingRecipes;
}

TArray<FName> UOdysseyCraftingRecipeComponent::GetRecipesProducingOutput(EResourceType ResourceType) const
{
	TArray<FName> MatchingRecipes;

	if (!CraftingManager)
	{
		return MatchingRecipes;
	}

	TArray<FAdvancedCraftingRecipe> AllRecipes = CraftingManager->GetAllRecipes();
	for (const FAdvancedCraftingRecipe& Recipe : AllRecipes)
	{
		for (const FCraftingOutput& Output : Recipe.PrimaryOutputs)
		{
			if (Output.ResourceType == ResourceType)
			{
				MatchingRecipes.Add(Recipe.RecipeID);
				break;
			}
		}
	}

	return MatchingRecipes;
}

// ============================================================================
// Integration
// ============================================================================

void UOdysseyCraftingRecipeComponent::SetSkillSystem(UOdysseyCraftingSkillSystem* NewSkillSystem)
{
	SkillSystem = NewSkillSystem;
}

void UOdysseyCraftingRecipeComponent::SetCraftingManager(UOdysseyCraftingManager* NewManager)
{
	CraftingManager = NewManager;
}

// ============================================================================
// Internal Methods
// ============================================================================

void UOdysseyCraftingRecipeComponent::ProcessActiveResearch(float DeltaTime)
{
	for (int32 i = ActiveResearchProjects.Num() - 1; i >= 0; --i)
	{
		FBlueprintResearchProgress& Research = ActiveResearchProjects[i];

		if (Research.bIsPaused)
		{
			continue;
		}

		FCraftingBlueprint Blueprint = GetBlueprint(Research.BlueprintID);
		float TotalTime = Blueprint.ResearchTime / ResearchSpeedMultiplier;

		Research.RemainingTime -= DeltaTime;
		Research.Progress = 1.0f - (Research.RemainingTime / TotalTime);

		OnResearchProgress.Broadcast(Research.BlueprintID, Research.Progress);

		if (Research.RemainingTime <= 0.0f)
		{
			CompleteBlueprintResearch(i);
		}
	}
}

void UOdysseyCraftingRecipeComponent::CompleteBlueprintResearch(int32 ResearchIndex)
{
	if (ResearchIndex < 0 || ResearchIndex >= ActiveResearchProjects.Num())
	{
		return;
	}

	FBlueprintResearchProgress& Research = ActiveResearchProjects[ResearchIndex];
	FName BlueprintID = Research.BlueprintID;

	FCraftingBlueprint Blueprint = GetBlueprint(BlueprintID);

	// Mark as researched
	ResearchedBlueprints.Add(BlueprintID);

	// Unlock recipes
	if (CraftingManager)
	{
		for (const FName& RecipeID : Blueprint.UnlockedRecipes)
		{
			CraftingManager->UnlockRecipe(RecipeID, BlueprintID);
		}
	}

	// Broadcast completion
	OnBlueprintResearched.Broadcast(BlueprintID, Blueprint.UnlockedRecipes);

	// Remove from active research
	ActiveResearchProjects.RemoveAt(ResearchIndex);
}

void UOdysseyCraftingRecipeComponent::LoadBlueprints()
{
	if (!BlueprintDataTable)
	{
		return;
	}

	// Auto-research basic blueprints
	TArray<FName> RowNames = BlueprintDataTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		FCraftingBlueprint* Blueprint = BlueprintDataTable->FindRow<FCraftingBlueprint>(RowName, TEXT("LoadBlueprints"));
		if (Blueprint && Blueprint->PrerequisiteBlueprints.Num() == 0 && Blueprint->RequiredSkillLevels.Num() == 0)
		{
			ResearchedBlueprints.Add(Blueprint->BlueprintID);
		}
	}
}

void UOdysseyCraftingRecipeComponent::LoadVariations()
{
	// Variations are loaded on-demand from data table
}

uint32 UOdysseyCraftingRecipeComponent::CalculateExperimentationHash(const TArray<FCraftingIngredient>& Ingredients) const
{
	uint32 Hash = 0;
	for (const FCraftingIngredient& Ingredient : Ingredients)
	{
		Hash ^= GetTypeHash(Ingredient.ResourceType);
		Hash = (Hash << 5) | (Hash >> 27); // Rotate
		Hash ^= Ingredient.Amount;
	}
	return Hash;
}

FRecipeVariation* UOdysseyCraftingRecipeComponent::FindVariationData(FName RecipeID, FName VariationID) const
{
	if (!VariationDataTable)
	{
		return nullptr;
	}

	// Variation row names are expected to be RecipeID_VariationID
	FName RowName = FName(*FString::Printf(TEXT("%s_%s"), *RecipeID.ToString(), *VariationID.ToString()));
	return VariationDataTable->FindRow<FRecipeVariation>(RowName, TEXT("FindVariationData"));
}
