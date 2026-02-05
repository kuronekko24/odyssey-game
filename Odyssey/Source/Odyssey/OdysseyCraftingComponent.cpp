#include "OdysseyCraftingComponent.h"
#include "OdysseyInventoryComponent.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"

UOdysseyCraftingComponent::UOdysseyCraftingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Default crafting settings
	MaxCraftingSlots = 3;
	CraftingLevel = 1;
	CraftingExperience = 0;
	CraftingSpeedMultiplier = 1.0f;
	CraftingSuccessBonus = 0.0f;
	InventoryComponent = nullptr;
}

void UOdysseyCraftingComponent::BeginPlay()
{
	Super::BeginPlay();

	// Try to find inventory component on the same actor
	if (!InventoryComponent)
	{
		InventoryComponent = GetOwner()->FindComponentByClass<UOdysseyInventoryComponent>();
	}

	if (!InventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("Crafting component could not find inventory component"));
	}

	UE_LOG(LogTemp, Warning, TEXT("Crafting component initialized. Level: %d, Speed: %f"),
		CraftingLevel, CraftingSpeedMultiplier);
}

void UOdysseyCraftingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Process active crafting operations
	if (CraftingQueue.Num() > 0)
	{
		ProcessCraftingQueue(DeltaTime);
	}
}

TArray<FName> UOdysseyCraftingComponent::GetAvailableRecipes() const
{
	TArray<FName> AvailableRecipes;

	if (RecipeDataTable)
	{
		TArray<FName> AllRowNames = RecipeDataTable->GetRowNames();
		for (const FName& RowName : AllRowNames)
		{
			if (IsRecipeUnlocked(RowName))
			{
				AvailableRecipes.Add(RowName);
			}
		}
	}

	return AvailableRecipes;
}

FCraftingRecipe UOdysseyCraftingComponent::GetRecipe(FName RecipeID) const
{
	if (RecipeDataTable)
	{
		FString ContextString = TEXT("Recipe Lookup");
		FCraftingRecipe* Recipe = RecipeDataTable->FindRow<FCraftingRecipe>(RecipeID, ContextString);
		if (Recipe)
		{
			return *Recipe;
		}
	}

	// Return default recipe if not found
	return FCraftingRecipe();
}

bool UOdysseyCraftingComponent::IsRecipeUnlocked(FName RecipeID) const
{
	FCraftingRecipe Recipe = GetRecipe(RecipeID);
	return Recipe.bIsUnlocked && CraftingLevel >= Recipe.RequiredCraftingLevel;
}

bool UOdysseyCraftingComponent::CanCraftRecipe(FName RecipeID, int32 Quantity) const
{
	if (!InventoryComponent)
		return false;

	FCraftingRecipe Recipe = GetRecipe(RecipeID);

	if (!IsRecipeUnlocked(RecipeID))
		return false;

	// Check if we have enough ingredients
	for (const FCraftingIngredient& Ingredient : Recipe.Ingredients)
	{
		int32 RequiredAmount = Ingredient.Amount * Quantity;
		if (!InventoryComponent->HasResource(Ingredient.ResourceType, RequiredAmount))
		{
			return false;
		}
	}

	// Check if crafting queue has space
	if (CraftingQueue.Num() >= MaxCraftingSlots)
		return false;

	return true;
}

bool UOdysseyCraftingComponent::StartCrafting(FName RecipeID, int32 Quantity)
{
	if (!CanCraftRecipe(RecipeID, Quantity))
		return false;

	FCraftingRecipe Recipe = GetRecipe(RecipeID);

	// Consume ingredients
	if (!ConsumeIngredients(Recipe, Quantity))
		return false;

	// Add to crafting queue
	FCraftingQueue QueueItem;
	QueueItem.RecipeID = RecipeID;
	QueueItem.Quantity = Quantity;
	QueueItem.RemainingTime = CalculateActualCraftingTime(Recipe.CraftingTime) * Quantity;
	QueueItem.State = ECraftingState::Crafting;

	CraftingQueue.Add(QueueItem);

	OnCraftingStarted(RecipeID, Quantity);

	UE_LOG(LogTemp, Warning, TEXT("Started crafting %s x%d. Time: %f seconds"),
		*Recipe.RecipeName, Quantity, QueueItem.RemainingTime);

	return true;
}

bool UOdysseyCraftingComponent::CancelCrafting(int32 QueueIndex)
{
	if (QueueIndex < 0 || QueueIndex >= CraftingQueue.Num())
		return false;

	FCraftingQueue& QueueItem = CraftingQueue[QueueIndex];
	FCraftingRecipe Recipe = GetRecipe(QueueItem.RecipeID);

	// Refund ingredients (partial refund based on remaining time)
	float RefundPercent = QueueItem.RemainingTime / (CalculateActualCraftingTime(Recipe.CraftingTime) * QueueItem.Quantity);
	RefundPercent = FMath::Clamp(RefundPercent, 0.0f, 1.0f);

	for (const FCraftingIngredient& Ingredient : Recipe.Ingredients)
	{
		int32 RefundAmount = FMath::RoundToInt(Ingredient.Amount * QueueItem.Quantity * RefundPercent);
		if (RefundAmount > 0 && InventoryComponent)
		{
			InventoryComponent->AddResource(Ingredient.ResourceType, RefundAmount);
		}
	}

	OnCraftingCancelled(QueueItem.RecipeID);

	CraftingQueue.RemoveAt(QueueIndex);

	UE_LOG(LogTemp, Warning, TEXT("Cancelled crafting %s. Refunded %f%% of materials"),
		*Recipe.RecipeName, RefundPercent * 100.0f);

	return true;
}

void UOdysseyCraftingComponent::ClearCraftingQueue()
{
	// Cancel all items in queue with refunds
	for (int32 i = CraftingQueue.Num() - 1; i >= 0; i--)
	{
		CancelCrafting(i);
	}
}

bool UOdysseyCraftingComponent::InstantCraftRecipe(FName RecipeID, int32 Quantity)
{
	if (!CanCraftRecipe(RecipeID, Quantity))
		return false;

	FCraftingRecipe Recipe = GetRecipe(RecipeID);

	// Consume ingredients
	if (!ConsumeIngredients(Recipe, Quantity))
		return false;

	// Immediately produce outputs
	ProduceOutputs(Recipe, Quantity);

	// Award experience
	AddCraftingExperience(Recipe.ExperienceReward * Quantity);

	OnCraftingCompleted(RecipeID, Quantity, true);

	UE_LOG(LogTemp, Warning, TEXT("Instant crafted %s x%d"), *Recipe.RecipeName, Quantity);

	return true;
}

bool UOdysseyCraftingComponent::IsCrafting() const
{
	return CraftingQueue.Num() > 0;
}

float UOdysseyCraftingComponent::GetTotalCraftingTime() const
{
	float TotalTime = 0.0f;
	for (const FCraftingQueue& Item : CraftingQueue)
	{
		TotalTime += Item.RemainingTime;
	}
	return TotalTime;
}

void UOdysseyCraftingComponent::AddCraftingExperience(int32 Experience)
{
	CraftingExperience += Experience;

	UE_LOG(LogTemp, Verbose, TEXT("Added %d crafting experience. Total: %d"), Experience, CraftingExperience);

	// Check for level up
	while (CanLevelUp())
	{
		LevelUpCrafting();
	}
}

int32 UOdysseyCraftingComponent::GetExperienceToNextLevel() const
{
	int32 RequiredExp = CalculateExperienceRequirement(CraftingLevel + 1);
	return FMath::Max(0, RequiredExp - CraftingExperience);
}

bool UOdysseyCraftingComponent::CanLevelUp() const
{
	return CraftingExperience >= CalculateExperienceRequirement(CraftingLevel + 1);
}

void UOdysseyCraftingComponent::LevelUpCrafting()
{
	if (!CanLevelUp())
		return;

	CraftingLevel++;

	// Improve crafting stats with each level
	CraftingSpeedMultiplier += 0.05f; // 5% faster crafting per level
	CraftingSuccessBonus += 0.02f;    // 2% higher success rate per level

	OnCraftingLevelUp(CraftingLevel);

	UE_LOG(LogTemp, Warning, TEXT("Crafting level increased to %d! Speed: %f, Success Bonus: %f"),
		CraftingLevel, CraftingSpeedMultiplier, CraftingSuccessBonus);
}

void UOdysseyCraftingComponent::SetInventoryComponent(UOdysseyInventoryComponent* NewInventory)
{
	InventoryComponent = NewInventory;
}

void UOdysseyCraftingComponent::ProcessCraftingQueue(float DeltaTime)
{
	for (int32 i = CraftingQueue.Num() - 1; i >= 0; i--)
	{
		FCraftingQueue& QueueItem = CraftingQueue[i];

		if (QueueItem.State == ECraftingState::Crafting)
		{
			QueueItem.RemainingTime -= DeltaTime;

			if (QueueItem.RemainingTime <= 0.0f)
			{
				CompleteCraftingItem(i);
			}
		}
	}
}

void UOdysseyCraftingComponent::CompleteCraftingItem(int32 QueueIndex)
{
	if (QueueIndex < 0 || QueueIndex >= CraftingQueue.Num())
		return;

	FCraftingQueue& QueueItem = CraftingQueue[QueueIndex];
	FCraftingRecipe Recipe = GetRecipe(QueueItem.RecipeID);

	// Produce outputs
	ProduceOutputs(Recipe, QueueItem.Quantity);

	// Award experience
	AddCraftingExperience(Recipe.ExperienceReward * QueueItem.Quantity);

	OnCraftingCompleted(QueueItem.RecipeID, QueueItem.Quantity, true);

	UE_LOG(LogTemp, Warning, TEXT("Completed crafting %s x%d"), *Recipe.RecipeName, QueueItem.Quantity);

	// Remove from queue
	CraftingQueue.RemoveAt(QueueIndex);
}

bool UOdysseyCraftingComponent::ConsumeIngredients(const FCraftingRecipe& Recipe, int32 Quantity)
{
	if (!InventoryComponent)
		return false;

	// Check if we have all ingredients first
	for (const FCraftingIngredient& Ingredient : Recipe.Ingredients)
	{
		int32 RequiredAmount = Ingredient.Amount * Quantity;
		if (!InventoryComponent->HasResource(Ingredient.ResourceType, RequiredAmount))
		{
			return false;
		}
	}

	// Consume ingredients
	for (const FCraftingIngredient& Ingredient : Recipe.Ingredients)
	{
		int32 ConsumeAmount = Ingredient.Amount * Quantity;
		if (!InventoryComponent->RemoveResource(Ingredient.ResourceType, ConsumeAmount))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to consume ingredient: %d"), (int32)Ingredient.ResourceType);
			return false;
		}
	}

	return true;
}

void UOdysseyCraftingComponent::ProduceOutputs(const FCraftingRecipe& Recipe, int32 Quantity)
{
	if (!InventoryComponent)
		return;

	for (const FCraftingOutput& Output : Recipe.Outputs)
	{
		// Calculate success chance
		float FinalSuccessChance = CalculateSuccessChance(Output.SuccessChance);

		for (int32 i = 0; i < Quantity; i++)
		{
			// Roll for success
			if (FMath::RandRange(0.0f, 1.0f) <= FinalSuccessChance)
			{
				InventoryComponent->AddResource(Output.ResourceType, Output.Amount);
			}
		}
	}
}

float UOdysseyCraftingComponent::CalculateActualCraftingTime(float BaseTime) const
{
	return BaseTime / CraftingSpeedMultiplier;
}

float UOdysseyCraftingComponent::CalculateSuccessChance(float BaseChance) const
{
	return FMath::Clamp(BaseChance + CraftingSuccessBonus, 0.0f, 1.0f);
}

int32 UOdysseyCraftingComponent::CalculateExperienceRequirement(int32 Level) const
{
	// Exponential XP curve: Level^2 * 100
	return Level * Level * 100;
}