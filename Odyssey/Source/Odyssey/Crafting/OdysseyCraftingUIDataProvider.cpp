// OdysseyCraftingUIDataProvider.cpp
// Mobile-optimized UI data provider implementation
// Batches expensive calculations to avoid per-frame overhead

#include "OdysseyCraftingUIDataProvider.h"
#include "OdysseyCraftingManager.h"
#include "OdysseyCraftingRecipeComponent.h"
#include "OdysseyQualityControlSystem.h"
#include "OdysseyCraftingSkillSystem.h"
#include "OdysseyInventoryComponent.h"

UOdysseyCraftingUIDataProvider::UOdysseyCraftingUIDataProvider()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.0f;

	RefreshInterval = 0.5f; // Refresh twice per second for mobile battery savings
	MaxRecipesPerRefresh = 20;
	TimeSinceLastRefresh = 0.0f;

	CraftingManager = nullptr;
	RecipeComponent = nullptr;
	QualitySystem = nullptr;
	SkillSystem = nullptr;
}

void UOdysseyCraftingUIDataProvider::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (Owner)
	{
		CraftingManager = Owner->FindComponentByClass<UOdysseyCraftingManager>();
		if (CraftingManager)
		{
			RecipeComponent = CraftingManager->GetRecipeComponent();
			QualitySystem = CraftingManager->GetQualitySystem();
			SkillSystem = CraftingManager->GetSkillSystem();
		}
	}

	// Initial data population
	ForceRefresh();
}

void UOdysseyCraftingUIDataProvider::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TimeSinceLastRefresh += DeltaTime;

	if (TimeSinceLastRefresh >= RefreshInterval)
	{
		// Job data refreshes every tick (progress changes)
		RefreshJobData();

		// Recipe and skill data can refresh less frequently
		RefreshRecipeData();
		RefreshSkillData();

		TimeSinceLastRefresh = 0.0f;

		OnCraftingUIDataUpdated.Broadcast();
	}
}

// ============================================================================
// Recipe Display Data
// ============================================================================

TArray<FRecipeDisplayData> UOdysseyCraftingUIDataProvider::GetAllRecipeDisplayData() const
{
	TArray<FRecipeDisplayData> AllData;
	for (const auto& Pair : CachedRecipeData)
	{
		AllData.Add(Pair.Value);
	}
	return AllData;
}

TArray<FRecipeDisplayData> UOdysseyCraftingUIDataProvider::GetRecipeDisplayDataByCategory(EItemCategory Category) const
{
	TArray<FRecipeDisplayData> Filtered;
	for (const auto& Pair : CachedRecipeData)
	{
		if (Pair.Value.Category == Category)
		{
			Filtered.Add(Pair.Value);
		}
	}
	return Filtered;
}

FRecipeDisplayData UOdysseyCraftingUIDataProvider::GetRecipeDisplayData(FName RecipeID) const
{
	const FRecipeDisplayData* Data = CachedRecipeData.Find(RecipeID);
	return Data ? *Data : FRecipeDisplayData();
}

TArray<FRecipeDisplayData> UOdysseyCraftingUIDataProvider::GetCraftableRecipeDisplayData() const
{
	TArray<FRecipeDisplayData> Craftable;
	for (const auto& Pair : CachedRecipeData)
	{
		if (Pair.Value.bCanCraftNow)
		{
			Craftable.Add(Pair.Value);
		}
	}
	return Craftable;
}

// ============================================================================
// Job Display Data
// ============================================================================

TArray<FJobDisplayData> UOdysseyCraftingUIDataProvider::GetActiveJobDisplayData() const
{
	return CachedJobData;
}

FJobDisplayData UOdysseyCraftingUIDataProvider::GetJobDisplayData(FGuid JobID) const
{
	for (const FJobDisplayData& Data : CachedJobData)
	{
		if (Data.JobID == JobID)
		{
			return Data;
		}
	}
	return FJobDisplayData();
}

// ============================================================================
// Skill Display Data
// ============================================================================

TArray<FSkillDisplayData> UOdysseyCraftingUIDataProvider::GetAllSkillDisplayData() const
{
	return CachedSkillData;
}

TArray<FSkillDisplayData> UOdysseyCraftingUIDataProvider::GetSkillDisplayDataByCategory(ECraftingSkillCategory Category) const
{
	TArray<FSkillDisplayData> Filtered;
	for (const FSkillDisplayData& Data : CachedSkillData)
	{
		if (Data.Category == Category)
		{
			Filtered.Add(Data);
		}
	}
	return Filtered;
}

FString UOdysseyCraftingUIDataProvider::GetSkillPointsText() const
{
	if (!SkillSystem)
	{
		return TEXT("Skill Points: 0");
	}

	FSkillPointAllocation Allocation = SkillSystem->GetSkillPointAllocation();
	return FString::Printf(TEXT("Skill Points: %d / %d"), Allocation.AvailableSkillPoints, Allocation.TotalSkillPoints);
}

// ============================================================================
// Utility
// ============================================================================

FString UOdysseyCraftingUIDataProvider::FormatTimeRemaining(float Seconds)
{
	if (Seconds <= 0.0f)
	{
		return TEXT("Complete");
	}

	int32 TotalSeconds = FMath::CeilToInt(Seconds);

	if (TotalSeconds < 60)
	{
		return FString::Printf(TEXT("%ds"), TotalSeconds);
	}

	int32 Minutes = TotalSeconds / 60;
	int32 RemainingSeconds = TotalSeconds % 60;

	if (Minutes < 60)
	{
		return FString::Printf(TEXT("%dm %02ds"), Minutes, RemainingSeconds);
	}

	int32 Hours = Minutes / 60;
	int32 RemainingMinutes = Minutes % 60;

	return FString::Printf(TEXT("%dh %02dm"), Hours, RemainingMinutes);
}

void UOdysseyCraftingUIDataProvider::ForceRefresh()
{
	RefreshRecipeData();
	RefreshJobData();
	RefreshSkillData();

	OnCraftingUIDataUpdated.Broadcast();
}

// ============================================================================
// Internal Refresh Methods
// ============================================================================

void UOdysseyCraftingUIDataProvider::RefreshRecipeData()
{
	if (!CraftingManager)
	{
		return;
	}

	TArray<FAdvancedCraftingRecipe> AllRecipes = CraftingManager->GetAllRecipes();

	int32 ProcessedCount = 0;
	for (const FAdvancedCraftingRecipe& Recipe : AllRecipes)
	{
		if (ProcessedCount >= MaxRecipesPerRefresh)
		{
			break; // Mobile optimization: process in batches
		}

		CachedRecipeData.Add(Recipe.RecipeID, BuildRecipeDisplayData(Recipe));
		ProcessedCount++;
	}
}

void UOdysseyCraftingUIDataProvider::RefreshJobData()
{
	CachedJobData.Empty();

	if (!CraftingManager)
	{
		return;
	}

	TArray<FCraftingJob> Jobs = CraftingManager->GetActiveJobs();
	for (const FCraftingJob& Job : Jobs)
	{
		CachedJobData.Add(BuildJobDisplayData(Job));
	}
}

void UOdysseyCraftingUIDataProvider::RefreshSkillData()
{
	CachedSkillData.Empty();

	if (!SkillSystem)
	{
		return;
	}

	TArray<FCraftingSkill> AllSkills = SkillSystem->GetAllSkills();
	for (const FCraftingSkill& Skill : AllSkills)
	{
		CachedSkillData.Add(BuildSkillDisplayData(Skill));
	}
}

FRecipeDisplayData UOdysseyCraftingUIDataProvider::BuildRecipeDisplayData(const FAdvancedCraftingRecipe& Recipe) const
{
	FRecipeDisplayData Data;
	Data.RecipeID = Recipe.RecipeID;
	Data.DisplayName = Recipe.RecipeName;
	Data.Description = Recipe.Description;
	Data.Category = Recipe.OutputCategory;
	Data.RequiredTier = Recipe.RequiredTier;
	Data.bIsUnlocked = CraftingManager->IsRecipeUnlocked(Recipe.RecipeID);
	Data.bCanCraftNow = CraftingManager->CanCraftRecipe(Recipe.RecipeID, 1);
	Data.CraftingTime = CraftingManager->CalculateCraftingTime(Recipe.RecipeID, 1);
	Data.EnergyCost = CraftingManager->CalculateEnergyCost(Recipe.RecipeID, 1);
	Data.SuccessChance = CraftingManager->CalculateSuccessChance(Recipe.RecipeID);

	// Quality info
	Data.ExpectedQuality = CraftingManager->CalculateExpectedQuality(Recipe.RecipeID);
	if (QualitySystem)
	{
		Data.QualityColor = QualitySystem->GetQualityColor(Data.ExpectedQuality);
	}

	// Difficulty
	if (RecipeComponent)
	{
		Data.DifficultyRating = RecipeComponent->GetRecipeDifficulty(Recipe.RecipeID);
	}

	// Material check
	UOdysseyInventoryComponent* Inventory = CraftingManager->GetOwner()->FindComponentByClass<UOdysseyInventoryComponent>();
	Data.bHasMaterials = true;

	Data.IngredientStatusLines.Empty();
	for (const FCraftingIngredient& Ingredient : Recipe.PrimaryIngredients)
	{
		int32 Available = 0;
		FString ResourceName = FString::Printf(TEXT("Resource_%d"), static_cast<int32>(Ingredient.ResourceType));

		if (Inventory)
		{
			Available = Inventory->GetResourceAmount(Ingredient.ResourceType);
			ResourceName = Inventory->GetResourceName(Ingredient.ResourceType);
		}

		bool bHasEnough = Available >= Ingredient.Amount;
		if (!bHasEnough)
		{
			Data.bHasMaterials = false;
		}

		FString StatusLine = FString::Printf(TEXT("%s: %d / %d %s"),
			*ResourceName, Available, Ingredient.Amount,
			bHasEnough ? TEXT("[OK]") : TEXT("[NEED]"));
		Data.IngredientStatusLines.Add(StatusLine);
	}

	// Skill check
	Data.bHasSkills = true;
	if (SkillSystem)
	{
		for (const auto& SkillReq : Recipe.RequiredSkillLevels)
		{
			if (SkillSystem->GetSkillLevel(SkillReq.Key) < SkillReq.Value)
			{
				Data.bHasSkills = false;
				break;
			}
		}
	}

	// Output preview
	Data.OutputPreviewLines.Empty();
	for (const FCraftingOutput& Output : Recipe.PrimaryOutputs)
	{
		FString OutputName = FString::Printf(TEXT("Resource_%d"), static_cast<int32>(Output.ResourceType));
		if (Inventory)
		{
			OutputName = Inventory->GetResourceName(Output.ResourceType);
		}

		FString OutputLine = FString::Printf(TEXT("%s x%d"), *OutputName, Output.Amount);
		if (Output.SuccessChance < 1.0f)
		{
			OutputLine += FString::Printf(TEXT(" (%d%% chance)"), FMath::RoundToInt(Output.SuccessChance * 100.0f));
		}
		Data.OutputPreviewLines.Add(OutputLine);
	}

	return Data;
}

FJobDisplayData UOdysseyCraftingUIDataProvider::BuildJobDisplayData(const FCraftingJob& Job) const
{
	FJobDisplayData Data;
	Data.JobID = Job.JobID;
	Data.Quantity = Job.Quantity;
	Data.CompletedQuantity = Job.CompletedQuantity;
	Data.Progress = Job.Progress;
	Data.TimeRemainingText = FormatTimeRemaining(Job.RemainingTime);

	// Get recipe name
	if (CraftingManager)
	{
		FAdvancedCraftingRecipe Recipe = CraftingManager->GetRecipe(Job.RecipeID);
		Data.RecipeName = Recipe.RecipeName;
	}

	// Status
	switch (Job.State)
	{
	case ECraftingState::Crafting:
		Data.StatusText = TEXT("Crafting...");
		Data.StatusColor = FLinearColor(0.2f, 0.8f, 0.2f); // Green
		Data.bCanCancel = true;
		Data.bCanPause = true;
		break;
	case ECraftingState::Idle:
		Data.StatusText = TEXT("Paused");
		Data.StatusColor = FLinearColor(1.0f, 0.8f, 0.0f); // Yellow
		Data.bCanCancel = true;
		Data.bCanPause = false;
		break;
	case ECraftingState::Completed:
		Data.StatusText = TEXT("Complete!");
		Data.StatusColor = FLinearColor(0.2f, 0.5f, 1.0f); // Blue
		Data.bCanCancel = false;
		Data.bCanPause = false;
		break;
	case ECraftingState::Failed:
		Data.StatusText = TEXT("Failed");
		Data.StatusColor = FLinearColor(1.0f, 0.2f, 0.2f); // Red
		Data.bCanCancel = false;
		Data.bCanPause = false;
		break;
	default:
		Data.StatusText = TEXT("Unknown");
		Data.StatusColor = FLinearColor::White;
		break;
	}

	return Data;
}

FSkillDisplayData UOdysseyCraftingUIDataProvider::BuildSkillDisplayData(const FCraftingSkill& Skill) const
{
	FSkillDisplayData Data;
	Data.SkillID = Skill.SkillID;
	Data.SkillName = Skill.SkillName;
	Data.Description = Skill.Description;
	Data.Category = Skill.Category;
	Data.CurrentLevel = Skill.CurrentLevel;
	Data.MaxLevel = Skill.MaxLevel;

	if (Skill.ExperienceToNextLevel > 0 && Skill.CurrentLevel < Skill.MaxLevel)
	{
		Data.ExperienceProgress = static_cast<float>(Skill.CurrentExperience) / Skill.ExperienceToNextLevel;
	}
	else
	{
		Data.ExperienceProgress = 1.0f;
	}

	if (SkillSystem)
	{
		Data.bIsUnlocked = SkillSystem->IsSkillUnlocked(Skill.SkillID);
		Data.bCanUnlock = SkillSystem->CanUnlockSkill(Skill.SkillID);
	}

	// Build bonus description
	TArray<FString> BonusParts;
	if (Skill.SpeedBonusPerLevel > 0.0f)
	{
		BonusParts.Add(FString::Printf(TEXT("+%.0f%% Speed/Lvl"), Skill.SpeedBonusPerLevel * 100.0f));
	}
	if (Skill.QualityBonusPerLevel > 0.0f)
	{
		BonusParts.Add(FString::Printf(TEXT("+%.1f%% Quality/Lvl"), Skill.QualityBonusPerLevel * 100.0f));
	}
	if (Skill.MaterialEfficiencyPerLevel > 0.0f)
	{
		BonusParts.Add(FString::Printf(TEXT("-%.0f%% Materials/Lvl"), Skill.MaterialEfficiencyPerLevel * 100.0f));
	}

	Data.BonusDescription = FString::Join(BonusParts, TEXT(", "));

	return Data;
}
