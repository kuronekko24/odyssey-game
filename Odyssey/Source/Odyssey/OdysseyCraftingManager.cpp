// OdysseyCraftingManager.cpp
// Implementation of the Master Crafting Controller

#include "OdysseyCraftingManager.h"
#include "OdysseyCraftingRecipeComponent.h"
#include "OdysseyAutomationNetworkSystem.h"
#include "OdysseyQualityControlSystem.h"
#include "OdysseyCraftingSkillSystem.h"
#include "OdysseyTradingComponent.h"

UOdysseyCraftingManager::UOdysseyCraftingManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.0f;

	MaxGlobalConcurrentJobs = 10;
	JobUpdateFrequency = 0.1f; // 10 updates per second for mobile
	JobBatchSize = 5;
	bEnableDebugLogging = false;
	TimeSinceLastUpdate = 0.0f;

	InventoryComponent = nullptr;
	TradingComponent = nullptr;
	RecipeComponent = nullptr;
	AutomationSystem = nullptr;
	QualitySystem = nullptr;
	SkillSystem = nullptr;
}

void UOdysseyCraftingManager::BeginPlay()
{
	Super::BeginPlay();

	InitializeSubsystems();
	LoadRecipes();

	// Auto-find inventory component on owner
	if (!InventoryComponent)
	{
		InventoryComponent = GetOwner()->FindComponentByClass<UOdysseyInventoryComponent>();
	}

	// Auto-find trading component
	if (!TradingComponent)
	{
		TradingComponent = GetOwner()->FindComponentByClass<UOdysseyTradingComponent>();
	}
}

void UOdysseyCraftingManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clean up any active jobs
	for (FCraftingJob& Job : ActiveJobs)
	{
		if (Job.State == ECraftingState::Crafting)
		{
			Job.State = ECraftingState::Idle;
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UOdysseyCraftingManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TimeSinceLastUpdate += DeltaTime;

	// Mobile optimization: batch process jobs at fixed intervals
	if (TimeSinceLastUpdate >= JobUpdateFrequency)
	{
		ProcessActiveJobs(TimeSinceLastUpdate);
		TimeSinceLastUpdate = 0.0f;
	}
}

void UOdysseyCraftingManager::InitializeSubsystems()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Create recipe component
	RecipeComponent = Owner->FindComponentByClass<UOdysseyCraftingRecipeComponent>();
	if (!RecipeComponent)
	{
		RecipeComponent = NewObject<UOdysseyCraftingRecipeComponent>(Owner, TEXT("CraftingRecipeComponent"));
		if (RecipeComponent)
		{
			RecipeComponent->RegisterComponent();
		}
	}

	// Create automation system
	AutomationSystem = Owner->FindComponentByClass<UOdysseyAutomationNetworkSystem>();
	if (!AutomationSystem)
	{
		AutomationSystem = NewObject<UOdysseyAutomationNetworkSystem>(Owner, TEXT("AutomationNetworkSystem"));
		if (AutomationSystem)
		{
			AutomationSystem->RegisterComponent();
			AutomationSystem->SetCraftingManager(this);
		}
	}

	// Create quality control system
	QualitySystem = Owner->FindComponentByClass<UOdysseyQualityControlSystem>();
	if (!QualitySystem)
	{
		QualitySystem = NewObject<UOdysseyQualityControlSystem>(Owner, TEXT("QualityControlSystem"));
		if (QualitySystem)
		{
			QualitySystem->RegisterComponent();
		}
	}

	// Create skill system
	SkillSystem = Owner->FindComponentByClass<UOdysseyCraftingSkillSystem>();
	if (!SkillSystem)
	{
		SkillSystem = NewObject<UOdysseyCraftingSkillSystem>(Owner, TEXT("CraftingSkillSystem"));
		if (SkillSystem)
		{
			SkillSystem->RegisterComponent();
		}
	}
}

void UOdysseyCraftingManager::LoadRecipes()
{
	if (!RecipeDataTable)
	{
		return;
	}

	// Load recipes and determine initial unlocks
	TArray<FName> RowNames = RecipeDataTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		FAdvancedCraftingRecipe* Recipe = RecipeDataTable->FindRow<FAdvancedCraftingRecipe>(RowName, TEXT("LoadRecipes"));
		if (Recipe)
		{
			// Auto-unlock basic recipes
			if (Recipe->RequiredTier == ECraftingTier::Primitive || Recipe->RequiredTier == ECraftingTier::Basic)
			{
				if (Recipe->RequiredSkillLevels.Num() == 0 || Recipe->PrerequisiteRecipes.Num() == 0)
				{
					UnlockedRecipes.Add(Recipe->RecipeID);
				}
			}
		}
	}
}

// ============================================================================
// Core Crafting Operations
// ============================================================================

FGuid UOdysseyCraftingManager::StartCraftingJob(FName RecipeID, int32 Quantity, FName FacilityID, int32 Priority)
{
	// Validate recipe
	FAdvancedCraftingRecipe Recipe = GetRecipe(RecipeID);
	if (Recipe.RecipeID.IsNone())
	{
		if (bEnableDebugLogging)
		{
			UE_LOG(LogTemp, Warning, TEXT("CraftingManager: Recipe not found: %s"), *RecipeID.ToString());
		}
		return FGuid();
	}

	// Check if recipe is unlocked
	if (!IsRecipeUnlocked(RecipeID))
	{
		if (bEnableDebugLogging)
		{
			UE_LOG(LogTemp, Warning, TEXT("CraftingManager: Recipe not unlocked: %s"), *RecipeID.ToString());
		}
		return FGuid();
	}

	// Validate requirements
	if (!ValidateRecipeRequirements(Recipe, Quantity, FacilityID))
	{
		return FGuid();
	}

	// Check global job limit
	if (ActiveJobs.Num() >= MaxGlobalConcurrentJobs)
	{
		if (bEnableDebugLogging)
		{
			UE_LOG(LogTemp, Warning, TEXT("CraftingManager: Max concurrent jobs reached"));
		}
		return FGuid();
	}

	// Consume ingredients
	if (!ConsumeJobIngredients(Recipe, Quantity))
	{
		if (bEnableDebugLogging)
		{
			UE_LOG(LogTemp, Warning, TEXT("CraftingManager: Failed to consume ingredients for: %s"), *RecipeID.ToString());
		}
		return FGuid();
	}

	// Create job
	FCraftingJob NewJob;
	NewJob.JobID = FGuid::NewGuid();
	NewJob.RecipeID = RecipeID;
	NewJob.Quantity = Quantity;
	NewJob.CompletedQuantity = 0;
	NewJob.Progress = 0.0f;
	NewJob.TotalTime = CalculateCraftingTime(RecipeID, Quantity, FacilityID);
	NewJob.RemainingTime = NewJob.TotalTime;
	NewJob.State = ECraftingState::Crafting;
	NewJob.StationID = FacilityID;
	NewJob.Priority = Priority;
	NewJob.bIsAutomated = false;

	// Determine target quality
	NewJob.TargetQuality = CalculateExpectedQuality(RecipeID, FacilityID);

	ActiveJobs.Add(NewJob);
	SortJobsByPriority();

	// Broadcast event
	OnCraftingJobStarted.Broadcast(NewJob.JobID, RecipeID, Quantity);

	if (bEnableDebugLogging)
	{
		UE_LOG(LogTemp, Log, TEXT("CraftingManager: Started job %s for recipe %s x%d"), 
			*NewJob.JobID.ToString(), *RecipeID.ToString(), Quantity);
	}

	return NewJob.JobID;
}

bool UOdysseyCraftingManager::CancelCraftingJob(FGuid JobID, bool bRefundMaterials)
{
	int32 JobIndex = INDEX_NONE;
	for (int32 i = 0; i < ActiveJobs.Num(); ++i)
	{
		if (ActiveJobs[i].JobID == JobID)
		{
			JobIndex = i;
			break;
		}
	}

	if (JobIndex == INDEX_NONE)
	{
		return false;
	}

	FCraftingJob& Job = ActiveJobs[JobIndex];
	FName RecipeID = Job.RecipeID;

	// Refund materials if requested
	if (bRefundMaterials && InventoryComponent)
	{
		FAdvancedCraftingRecipe Recipe = GetRecipe(RecipeID);
		int32 RemainingQuantity = Job.Quantity - Job.CompletedQuantity;
		
		// Partial refund based on progress
		float RefundMultiplier = 1.0f - Job.Progress;
		
		for (const FCraftingIngredient& Ingredient : Recipe.PrimaryIngredients)
		{
			int32 RefundAmount = FMath::FloorToInt(Ingredient.Amount * RemainingQuantity * RefundMultiplier);
			if (RefundAmount > 0)
			{
				InventoryComponent->AddResource(Ingredient.ResourceType, RefundAmount);
			}
		}
	}

	ActiveJobs.RemoveAt(JobIndex);

	OnCraftingJobCancelled.Broadcast(JobID, RecipeID);

	return true;
}

bool UOdysseyCraftingManager::PauseCraftingJob(FGuid JobID)
{
	for (FCraftingJob& Job : ActiveJobs)
	{
		if (Job.JobID == JobID && Job.State == ECraftingState::Crafting)
		{
			Job.State = ECraftingState::Idle;
			return true;
		}
	}
	return false;
}

bool UOdysseyCraftingManager::ResumeCraftingJob(FGuid JobID)
{
	for (FCraftingJob& Job : ActiveJobs)
	{
		if (Job.JobID == JobID && Job.State == ECraftingState::Idle)
		{
			Job.State = ECraftingState::Crafting;
			return true;
		}
	}
	return false;
}

TArray<FCraftedItem> UOdysseyCraftingManager::InstantCraft(FName RecipeID, int32 Quantity)
{
	TArray<FCraftedItem> Results;

	FAdvancedCraftingRecipe Recipe = GetRecipe(RecipeID);
	if (Recipe.RecipeID.IsNone())
	{
		return Results;
	}

	if (!CanCraftRecipe(RecipeID, Quantity))
	{
		return Results;
	}

	// Consume ingredients
	if (!ConsumeJobIngredients(Recipe, Quantity))
	{
		return Results;
	}

	// Determine quality and produce outputs
	EItemQuality Quality = DetermineOutputQuality(Recipe, NAME_None);
	Results = ProduceJobOutputs(Recipe, Quantity, Quality);

	// Update statistics
	UpdateStatistics(Results, true);

	// Award experience if skill system exists
	if (SkillSystem)
	{
		for (const auto& SkillExp : Recipe.SkillExperienceRewards)
		{
			SkillSystem->AddSkillExperience(SkillExp.Key, SkillExp.Value * Quantity);
		}
	}

	return Results;
}

bool UOdysseyCraftingManager::CanCraftRecipe(FName RecipeID, int32 Quantity, FName FacilityID) const
{
	FAdvancedCraftingRecipe Recipe = GetRecipe(RecipeID);
	if (Recipe.RecipeID.IsNone())
	{
		return false;
	}

	if (!IsRecipeUnlocked(RecipeID))
	{
		return false;
	}

	return ValidateRecipeRequirements(Recipe, Quantity, FacilityID);
}

TArray<FName> UOdysseyCraftingManager::GetCraftableRecipes(FName FacilityID) const
{
	TArray<FName> CraftableRecipes;

	for (const FName& RecipeID : UnlockedRecipes)
	{
		if (CanCraftRecipe(RecipeID, 1, FacilityID))
		{
			CraftableRecipes.Add(RecipeID);
		}
	}

	return CraftableRecipes;
}

// ============================================================================
// Recipe Management
// ============================================================================

FAdvancedCraftingRecipe UOdysseyCraftingManager::GetRecipe(FName RecipeID) const
{
	if (RecipeDataTable)
	{
		FAdvancedCraftingRecipe* Recipe = RecipeDataTable->FindRow<FAdvancedCraftingRecipe>(RecipeID, TEXT("GetRecipe"));
		if (Recipe)
		{
			return *Recipe;
		}
	}

	return FAdvancedCraftingRecipe();
}

TArray<FAdvancedCraftingRecipe> UOdysseyCraftingManager::GetAllRecipes() const
{
	TArray<FAdvancedCraftingRecipe> AllRecipes;

	if (RecipeDataTable)
	{
		TArray<FName> RowNames = RecipeDataTable->GetRowNames();
		for (const FName& RowName : RowNames)
		{
			FAdvancedCraftingRecipe* Recipe = RecipeDataTable->FindRow<FAdvancedCraftingRecipe>(RowName, TEXT("GetAllRecipes"));
			if (Recipe)
			{
				AllRecipes.Add(*Recipe);
			}
		}
	}

	return AllRecipes;
}

TArray<FAdvancedCraftingRecipe> UOdysseyCraftingManager::GetRecipesByCategory(EItemCategory Category) const
{
	TArray<FAdvancedCraftingRecipe> CategoryRecipes;

	TArray<FAdvancedCraftingRecipe> AllRecipes = GetAllRecipes();
	for (const FAdvancedCraftingRecipe& Recipe : AllRecipes)
	{
		if (Recipe.OutputCategory == Category)
		{
			CategoryRecipes.Add(Recipe);
		}
	}

	return CategoryRecipes;
}

bool UOdysseyCraftingManager::IsRecipeUnlocked(FName RecipeID) const
{
	return UnlockedRecipes.Contains(RecipeID);
}

bool UOdysseyCraftingManager::UnlockRecipe(FName RecipeID, FName UnlockSource)
{
	if (UnlockedRecipes.Contains(RecipeID))
	{
		return false; // Already unlocked
	}

	FAdvancedCraftingRecipe Recipe = GetRecipe(RecipeID);
	if (Recipe.RecipeID.IsNone())
	{
		return false;
	}

	// Check prerequisites
	for (const FName& PrereqID : Recipe.PrerequisiteRecipes)
	{
		if (!UnlockedRecipes.Contains(PrereqID))
		{
			return false;
		}
	}

	UnlockedRecipes.Add(RecipeID);

	// Broadcast unlock event
	FName SkillName = UnlockSource.IsNone() ? FName(TEXT("Discovery")) : UnlockSource;
	int32 Level = SkillSystem ? SkillSystem->GetSkillLevel(SkillName) : 1;
	OnRecipeUnlocked.Broadcast(RecipeID, SkillName, Level);

	// Auto-unlock dependent recipes that now meet requirements
	TArray<FAdvancedCraftingRecipe> AllRecipes = GetAllRecipes();
	for (const FAdvancedCraftingRecipe& OtherRecipe : AllRecipes)
	{
		if (OtherRecipe.PrerequisiteRecipes.Contains(RecipeID))
		{
			// Check if all prerequisites are now met
			bool bAllPrereqsMet = true;
			for (const FName& PrereqID : OtherRecipe.PrerequisiteRecipes)
			{
				if (!UnlockedRecipes.Contains(PrereqID))
				{
					bAllPrereqsMet = false;
					break;
				}
			}

			if (bAllPrereqsMet)
			{
				UnlockRecipe(OtherRecipe.RecipeID, UnlockSource);
			}
		}
	}

	return true;
}

TArray<FName> UOdysseyCraftingManager::GetProductionChain(FName RecipeID) const
{
	TArray<FName> Chain;
	TSet<FName> Visited;

	// Build chain recursively
	TFunction<void(FName)> BuildChain = [&](FName CurrentID)
	{
		if (Visited.Contains(CurrentID))
		{
			return;
		}
		Visited.Add(CurrentID);

		FAdvancedCraftingRecipe Recipe = GetRecipe(CurrentID);
		if (Recipe.RecipeID.IsNone())
		{
			return;
		}

		// First add prerequisites
		for (const FName& PrereqID : Recipe.PrerequisiteRecipes)
		{
			BuildChain(PrereqID);
		}

		// Then add this recipe
		Chain.Add(CurrentID);
	};

	BuildChain(RecipeID);
	return Chain;
}

TArray<FCraftingIngredient> UOdysseyCraftingManager::CalculateChainMaterials(FName RecipeID, int32 Quantity) const
{
	TMap<EResourceType, int32> TotalMaterials;

	TArray<FName> Chain = GetProductionChain(RecipeID);
	for (const FName& ChainRecipeID : Chain)
	{
		FAdvancedCraftingRecipe Recipe = GetRecipe(ChainRecipeID);
		int32 RequiredQuantity = (ChainRecipeID == RecipeID) ? Quantity : 1;

		for (const FCraftingIngredient& Ingredient : Recipe.PrimaryIngredients)
		{
			int32& Total = TotalMaterials.FindOrAdd(Ingredient.ResourceType);
			Total += Ingredient.Amount * RequiredQuantity;
		}
	}

	// Convert to array
	TArray<FCraftingIngredient> Result;
	for (const auto& Pair : TotalMaterials)
	{
		FCraftingIngredient Ingredient;
		Ingredient.ResourceType = Pair.Key;
		Ingredient.Amount = Pair.Value;
		Result.Add(Ingredient);
	}

	return Result;
}

// ============================================================================
// Facility Management
// ============================================================================

bool UOdysseyCraftingManager::RegisterFacility(const FCraftingFacility& Facility)
{
	if (Facility.FacilityID.IsNone())
	{
		return false;
	}

	if (Facilities.Contains(Facility.FacilityID))
	{
		return false;
	}

	Facilities.Add(Facility.FacilityID, Facility);
	return true;
}

bool UOdysseyCraftingManager::UnregisterFacility(FName FacilityID)
{
	if (!Facilities.Contains(FacilityID))
	{
		return false;
	}

	// Cancel all jobs at this facility
	for (int32 i = ActiveJobs.Num() - 1; i >= 0; --i)
	{
		if (ActiveJobs[i].StationID == FacilityID)
		{
			CancelCraftingJob(ActiveJobs[i].JobID, true);
		}
	}

	Facilities.Remove(FacilityID);
	return true;
}

FCraftingFacility UOdysseyCraftingManager::GetFacility(FName FacilityID) const
{
	const FCraftingFacility* Facility = Facilities.Find(FacilityID);
	return Facility ? *Facility : FCraftingFacility();
}

TArray<FCraftingFacility> UOdysseyCraftingManager::GetAllFacilities() const
{
	TArray<FCraftingFacility> Result;
	for (const auto& Pair : Facilities)
	{
		Result.Add(Pair.Value);
	}
	return Result;
}

bool UOdysseyCraftingManager::SetFacilityOnlineStatus(FName FacilityID, bool bOnline)
{
	FCraftingFacility* Facility = Facilities.Find(FacilityID);
	if (!Facility)
	{
		return false;
	}

	bool bPreviousStatus = Facility->bIsOnline;
	Facility->bIsOnline = bOnline;

	if (bPreviousStatus != bOnline)
	{
		OnFacilityStatusChanged.Broadcast(FacilityID, bOnline);
	}

	return true;
}

bool UOdysseyCraftingManager::UpgradeFacility(FName FacilityID)
{
	FCraftingFacility* Facility = Facilities.Find(FacilityID);
	if (!Facility)
	{
		return false;
	}

	// Check if max tier
	if (Facility->Tier == ECraftingTier::Quantum)
	{
		return false;
	}

	// Upgrade tier
	Facility->Tier = static_cast<ECraftingTier>(static_cast<uint8>(Facility->Tier) + 1);
	Facility->Level++;
	
	// Apply tier bonuses
	Facility->SpeedMultiplier *= 1.15f;
	Facility->QualityBonus += 0.05f;
	Facility->MaxConcurrentJobs++;

	return true;
}

FName UOdysseyCraftingManager::GetBestFacilityForRecipe(FName RecipeID) const
{
	FAdvancedCraftingRecipe Recipe = GetRecipe(RecipeID);
	if (Recipe.RecipeID.IsNone())
	{
		return NAME_None;
	}

	FName BestFacility = NAME_None;
	float BestScore = -1.0f;

	for (const auto& Pair : Facilities)
	{
		const FCraftingFacility& Facility = Pair.Value;

		if (!Facility.bIsOnline)
		{
			continue;
		}

		// Check tier requirement
		if (static_cast<uint8>(Facility.Tier) < static_cast<uint8>(Recipe.RequiredTier))
		{
			continue;
		}

		// Check category support
		if (!Facility.SupportedCategories.Contains(Recipe.OutputCategory))
		{
			continue;
		}

		// Check if facility has capacity
		int32 ActiveJobsCount = 0;
		for (const FCraftingJob& Job : ActiveJobs)
		{
			if (Job.StationID == Facility.FacilityID)
			{
				ActiveJobsCount++;
			}
		}

		if (ActiveJobsCount >= Facility.MaxConcurrentJobs)
		{
			continue;
		}

		// Calculate facility score
		float Score = Facility.SpeedMultiplier * 0.4f + Facility.QualityBonus * 0.4f + Facility.EnergyEfficiency * 0.2f;

		if (Score > BestScore)
		{
			BestScore = Score;
			BestFacility = Facility.FacilityID;
		}
	}

	return BestFacility;
}

// ============================================================================
// Job Queue Management
// ============================================================================

TArray<FCraftingJob> UOdysseyCraftingManager::GetActiveJobs() const
{
	return ActiveJobs;
}

TArray<FCraftingJob> UOdysseyCraftingManager::GetJobsForFacility(FName FacilityID) const
{
	TArray<FCraftingJob> FacilityJobs;
	for (const FCraftingJob& Job : ActiveJobs)
	{
		if (Job.StationID == FacilityID)
		{
			FacilityJobs.Add(Job);
		}
	}
	return FacilityJobs;
}

FCraftingJob UOdysseyCraftingManager::GetJob(FGuid JobID) const
{
	for (const FCraftingJob& Job : ActiveJobs)
	{
		if (Job.JobID == JobID)
		{
			return Job;
		}
	}
	return FCraftingJob();
}

bool UOdysseyCraftingManager::SetJobPriority(FGuid JobID, int32 NewPriority)
{
	for (FCraftingJob& Job : ActiveJobs)
	{
		if (Job.JobID == JobID)
		{
			Job.Priority = NewPriority;
			SortJobsByPriority();
			return true;
		}
	}
	return false;
}

float UOdysseyCraftingManager::GetJobEstimatedCompletion(FGuid JobID) const
{
	for (const FCraftingJob& Job : ActiveJobs)
	{
		if (Job.JobID == JobID)
		{
			return Job.RemainingTime;
		}
	}
	return 0.0f;
}

float UOdysseyCraftingManager::GetTotalQueueTime() const
{
	float TotalTime = 0.0f;
	for (const FCraftingJob& Job : ActiveJobs)
	{
		TotalTime += Job.RemainingTime;
	}
	return TotalTime;
}

// ============================================================================
// Quality and Crafting Calculations
// ============================================================================

EItemQuality UOdysseyCraftingManager::CalculateExpectedQuality(FName RecipeID, FName FacilityID) const
{
	if (QualitySystem)
	{
		return QualitySystem->CalculateExpectedQuality(RecipeID, FacilityID);
	}

	// Default quality calculation
	FAdvancedCraftingRecipe Recipe = GetRecipe(RecipeID);
	float QualityScore = Recipe.BaseQualityChance;

	// Apply facility bonus
	if (!FacilityID.IsNone())
	{
		FCraftingFacility Facility = GetFacility(FacilityID);
		QualityScore += Facility.QualityBonus;
	}

	// Apply skill bonus
	if (SkillSystem && Recipe.bQualityAffectedBySkill)
	{
		float SkillBonus = SkillSystem->GetCraftingQualityBonus();
		QualityScore += SkillBonus;
	}

	// Determine quality tier
	if (QualityScore >= 0.95f) return EItemQuality::Legendary;
	if (QualityScore >= 0.85f) return EItemQuality::Masterwork;
	if (QualityScore >= 0.70f) return EItemQuality::Superior;
	if (QualityScore >= 0.55f) return EItemQuality::Quality;
	if (QualityScore >= 0.40f) return EItemQuality::Standard;
	if (QualityScore >= 0.20f) return EItemQuality::Common;
	return EItemQuality::Scrap;
}

float UOdysseyCraftingManager::CalculateCraftingTime(FName RecipeID, int32 Quantity, FName FacilityID) const
{
	FAdvancedCraftingRecipe Recipe = GetRecipe(RecipeID);
	float BaseTime = Recipe.BaseCraftingTime * Quantity;

	// Apply facility speed bonus
	if (!FacilityID.IsNone())
	{
		FCraftingFacility Facility = GetFacility(FacilityID);
		if (Facility.SpeedMultiplier > 0)
		{
			BaseTime /= Facility.SpeedMultiplier;
		}
	}

	// Apply skill speed bonus
	if (SkillSystem)
	{
		float SkillSpeedBonus = SkillSystem->GetCraftingSpeedBonus();
		BaseTime *= (1.0f - SkillSpeedBonus);
	}

	return FMath::Max(BaseTime, 0.5f); // Minimum 0.5 second crafting time
}

int32 UOdysseyCraftingManager::CalculateEnergyCost(FName RecipeID, int32 Quantity, FName FacilityID) const
{
	FAdvancedCraftingRecipe Recipe = GetRecipe(RecipeID);
	float EnergyCost = static_cast<float>(Recipe.EnergyCost * Quantity);

	// Apply facility efficiency
	if (!FacilityID.IsNone())
	{
		FCraftingFacility Facility = GetFacility(FacilityID);
		EnergyCost *= Facility.EnergyEfficiency;
	}

	return FMath::CeilToInt(EnergyCost);
}

float UOdysseyCraftingManager::CalculateSuccessChance(FName RecipeID, FName FacilityID) const
{
	FAdvancedCraftingRecipe Recipe = GetRecipe(RecipeID);
	float SuccessChance = 0.9f; // Base 90% success

	// Apply skill bonus
	if (SkillSystem)
	{
		SuccessChance += SkillSystem->GetCraftingSuccessBonus();
	}

	// Apply facility bonus
	if (!FacilityID.IsNone())
	{
		FCraftingFacility Facility = GetFacility(FacilityID);
		SuccessChance += Facility.QualityBonus * 0.1f;
	}

	return FMath::Clamp(SuccessChance, 0.1f, 1.0f);
}

// ============================================================================
// Component Integration
// ============================================================================

void UOdysseyCraftingManager::SetInventoryComponent(UOdysseyInventoryComponent* Inventory)
{
	InventoryComponent = Inventory;
}

void UOdysseyCraftingManager::SetTradingComponent(UOdysseyTradingComponent* Trading)
{
	TradingComponent = Trading;
}

// ============================================================================
// Statistics
// ============================================================================

void UOdysseyCraftingManager::ResetStatistics()
{
	Statistics = FCraftingStatistics();
}

// ============================================================================
// Internal Methods
// ============================================================================

void UOdysseyCraftingManager::ProcessActiveJobs(float DeltaTime)
{
	int32 ProcessedCount = 0;

	for (int32 i = 0; i < ActiveJobs.Num() && ProcessedCount < JobBatchSize; ++i)
	{
		FCraftingJob& Job = ActiveJobs[i];

		if (Job.State != ECraftingState::Crafting)
		{
			continue;
		}

		ProcessedCount++;

		// Update job progress
		Job.RemainingTime -= DeltaTime;
		Job.Progress = 1.0f - (Job.RemainingTime / Job.TotalTime);

		// Broadcast progress
		OnCraftingJobProgress.Broadcast(Job.JobID, Job.Progress);

		// Check completion
		if (Job.RemainingTime <= 0.0f)
		{
			CompleteJob(i);
			--i; // Adjust index since job was removed
		}
	}
}

void UOdysseyCraftingManager::CompleteJob(int32 JobIndex)
{
	if (JobIndex < 0 || JobIndex >= ActiveJobs.Num())
	{
		return;
	}

	FCraftingJob& Job = ActiveJobs[JobIndex];
	FAdvancedCraftingRecipe Recipe = GetRecipe(Job.RecipeID);

	// Determine final quality
	EItemQuality FinalQuality = DetermineOutputQuality(Recipe, Job.StationID);
	OnQualityDetermined.Broadcast(Job.JobID, FinalQuality);

	// Produce outputs
	TArray<FCraftedItem> ProducedItems = ProduceJobOutputs(Recipe, Job.Quantity, FinalQuality);
	Job.ProducedItems = ProducedItems;

	// Update statistics
	bool bSuccess = ProducedItems.Num() > 0;
	UpdateStatistics(ProducedItems, bSuccess);

	// Award experience
	if (SkillSystem)
	{
		for (const auto& SkillExp : Recipe.SkillExperienceRewards)
		{
			// Bonus XP for higher quality
			float QualityMultiplier = 1.0f + (static_cast<float>(FinalQuality) * 0.1f);
			int32 TotalXP = FMath::CeilToInt(SkillExp.Value * Job.Quantity * QualityMultiplier);
			SkillSystem->AddSkillExperience(SkillExp.Key, TotalXP);
		}
	}

	// Mark completed
	Job.State = ECraftingState::Completed;
	Job.CompletedQuantity = Job.Quantity;

	// Broadcast completion
	OnCraftingJobCompleted.Broadcast(Job.JobID, ProducedItems, bSuccess);

	// Remove from active jobs
	ActiveJobs.RemoveAt(JobIndex);

	if (bEnableDebugLogging)
	{
		UE_LOG(LogTemp, Log, TEXT("CraftingManager: Completed job %s, produced %d items at %s quality"),
			*Job.JobID.ToString(), ProducedItems.Num(), *UEnum::GetValueAsString(FinalQuality));
	}
}

bool UOdysseyCraftingManager::ConsumeJobIngredients(const FAdvancedCraftingRecipe& Recipe, int32 Quantity)
{
	if (!InventoryComponent)
	{
		return false;
	}

	// Verify all ingredients are available
	for (const FCraftingIngredient& Ingredient : Recipe.PrimaryIngredients)
	{
		int32 Required = Ingredient.Amount * Quantity;
		if (!InventoryComponent->HasResource(Ingredient.ResourceType, Required))
		{
			return false;
		}
	}

	// Consume ingredients
	for (const FCraftingIngredient& Ingredient : Recipe.PrimaryIngredients)
	{
		int32 Required = Ingredient.Amount * Quantity;
		InventoryComponent->RemoveResource(Ingredient.ResourceType, Required);
	}

	return true;
}

TArray<FCraftedItem> UOdysseyCraftingManager::ProduceJobOutputs(const FAdvancedCraftingRecipe& Recipe, int32 Quantity, EItemQuality Quality)
{
	TArray<FCraftedItem> ProducedItems;

	for (const FCraftingOutput& Output : Recipe.PrimaryOutputs)
	{
		// Check success chance
		float Roll = FMath::FRand();
		if (Roll > Output.SuccessChance)
		{
			continue;
		}

		FCraftedItem NewItem;
		NewItem.ItemID = FName(*FString::Printf(TEXT("%s_%s"), *Recipe.RecipeID.ToString(), *FGuid::NewGuid().ToString().Left(8)));
		NewItem.ResourceType = Output.ResourceType;
		NewItem.Category = Recipe.OutputCategory;
		NewItem.Quality = Quality;
		NewItem.Quantity = Output.Amount * Quantity;
		NewItem.QualityMultiplier = 1.0f + (static_cast<float>(Quality) * 0.15f);
		NewItem.CraftedTime = FDateTime::Now();

		// Add to inventory
		if (InventoryComponent)
		{
			InventoryComponent->AddResource(Output.ResourceType, NewItem.Quantity);
		}

		ProducedItems.Add(NewItem);
	}

	// Check for bonus outputs
	if (Recipe.BonusOutputChance > 0.0f)
	{
		float Roll = FMath::FRand();
		if (Roll <= Recipe.BonusOutputChance)
		{
			for (const FCraftingOutput& BonusOutput : Recipe.BonusOutputs)
			{
				FCraftedItem BonusItem;
				BonusItem.ItemID = FName(*FString::Printf(TEXT("BONUS_%s_%s"), *Recipe.RecipeID.ToString(), *FGuid::NewGuid().ToString().Left(8)));
				BonusItem.ResourceType = BonusOutput.ResourceType;
				BonusItem.Category = Recipe.OutputCategory;
				BonusItem.Quality = Quality;
				BonusItem.Quantity = BonusOutput.Amount;
				BonusItem.QualityMultiplier = 1.0f + (static_cast<float>(Quality) * 0.15f);
				BonusItem.CraftedTime = FDateTime::Now();

				if (InventoryComponent)
				{
					InventoryComponent->AddResource(BonusOutput.ResourceType, BonusItem.Quantity);
				}

				ProducedItems.Add(BonusItem);
			}
		}
	}

	return ProducedItems;
}

EItemQuality UOdysseyCraftingManager::DetermineOutputQuality(const FAdvancedCraftingRecipe& Recipe, FName FacilityID)
{
	if (QualitySystem)
	{
		return QualitySystem->RollQuality(Recipe.RecipeID, FacilityID);
	}

	// Fallback quality determination
	return CalculateExpectedQuality(Recipe.RecipeID, FacilityID);
}

FCraftingFacility* UOdysseyCraftingManager::FindFacilityForJob(FCraftingJob& Job)
{
	if (!Job.StationID.IsNone())
	{
		return Facilities.Find(Job.StationID);
	}

	// Find best available facility
	FName BestFacility = GetBestFacilityForRecipe(Job.RecipeID);
	if (!BestFacility.IsNone())
	{
		Job.StationID = BestFacility;
		return Facilities.Find(BestFacility);
	}

	return nullptr;
}

bool UOdysseyCraftingManager::ValidateRecipeRequirements(const FAdvancedCraftingRecipe& Recipe, int32 Quantity, FName FacilityID) const
{
	// Check tier requirement
	if (!FacilityID.IsNone())
	{
		FCraftingFacility Facility = GetFacility(FacilityID);
		if (static_cast<uint8>(Facility.Tier) < static_cast<uint8>(Recipe.RequiredTier))
		{
			return false;
		}

		if (!Facility.bIsOnline)
		{
			return false;
		}
	}

	// Check skill requirements
	if (SkillSystem)
	{
		for (const auto& SkillReq : Recipe.RequiredSkillLevels)
		{
			if (SkillSystem->GetSkillLevel(SkillReq.Key) < SkillReq.Value)
			{
				return false;
			}
		}
	}

	// Check ingredient availability
	if (InventoryComponent)
	{
		for (const FCraftingIngredient& Ingredient : Recipe.PrimaryIngredients)
		{
			int32 Required = Ingredient.Amount * Quantity;
			if (!InventoryComponent->HasResource(Ingredient.ResourceType, Required))
			{
				return false;
			}
		}
	}

	return true;
}

void UOdysseyCraftingManager::UpdateStatistics(const TArray<FCraftedItem>& ProducedItems, bool bSuccess)
{
	if (bSuccess)
	{
		Statistics.SuccessfulCrafts++;
	}
	else
	{
		Statistics.FailedCrafts++;
	}

	for (const FCraftedItem& Item : ProducedItems)
	{
		Statistics.TotalItemsCrafted += Item.Quantity;

		int32& QualityCount = Statistics.ItemsByQuality.FindOrAdd(Item.Quality);
		QualityCount += Item.Quantity;

		int32& CategoryCount = Statistics.ItemsByCategory.FindOrAdd(Item.Category);
		CategoryCount += Item.Quantity;

		if (Item.Quality == EItemQuality::Masterwork)
		{
			Statistics.MasterworkItemsCreated += Item.Quantity;
		}
		else if (Item.Quality == EItemQuality::Legendary)
		{
			Statistics.LegendaryItemsCreated += Item.Quantity;
		}
	}
}

void UOdysseyCraftingManager::SortJobsByPriority()
{
	ActiveJobs.Sort([](const FCraftingJob& A, const FCraftingJob& B)
	{
		return A.Priority > B.Priority;
	});
}
