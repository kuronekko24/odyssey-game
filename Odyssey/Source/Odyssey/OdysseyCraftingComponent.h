#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "OdysseyInventoryComponent.h"
#include "OdysseyCraftingComponent.generated.h"

USTRUCT(BlueprintType)
struct ODYSSEY_API FCraftingIngredient
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	EResourceType ResourceType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	int32 Amount;

	FCraftingIngredient()
	{
		ResourceType = EResourceType::None;
		Amount = 1;
	}

	FCraftingIngredient(EResourceType Type, int32 Quantity)
	{
		ResourceType = Type;
		Amount = Quantity;
	}
};

USTRUCT(BlueprintType)
struct ODYSSEY_API FCraftingOutput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	EResourceType ResourceType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	int32 Amount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	float SuccessChance;

	FCraftingOutput()
	{
		ResourceType = EResourceType::None;
		Amount = 1;
		SuccessChance = 1.0f;
	}

	FCraftingOutput(EResourceType Type, int32 Quantity, float Chance = 1.0f)
	{
		ResourceType = Type;
		Amount = Quantity;
		SuccessChance = Chance;
	}
};

USTRUCT(BlueprintType, meta = (DataTable))
struct ODYSSEY_API FCraftingRecipe : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe")
	FString RecipeName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe")
	TArray<FCraftingIngredient> Ingredients;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe")
	TArray<FCraftingOutput> Outputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe")
	float CraftingTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe")
	int32 RequiredCraftingLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe")
	bool bIsUnlocked;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe")
	int32 ExperienceReward;

	FCraftingRecipe()
	{
		RecipeName = TEXT("Unknown Recipe");
		Description = TEXT("No description available");
		CraftingTime = 1.0f;
		RequiredCraftingLevel = 1;
		bIsUnlocked = true;
		ExperienceReward = 10;
	}
};

UENUM(BlueprintType)
enum class ECraftingState : uint8
{
	Idle,
	Crafting,
	Completed,
	Failed
};

USTRUCT(BlueprintType)
struct ODYSSEY_API FCraftingQueue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Queue")
	FName RecipeID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Queue")
	int32 Quantity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Queue")
	float RemainingTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Queue")
	ECraftingState State;

	FCraftingQueue()
	{
		RecipeID = NAME_None;
		Quantity = 1;
		RemainingTime = 0.0f;
		State = ECraftingState::Idle;
	}
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UOdysseyCraftingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdysseyCraftingComponent();

protected:
	// Crafting system data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	UDataTable* RecipeDataTable;

	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	TArray<FCraftingQueue> CraftingQueue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	int32 MaxCraftingSlots;

	// Player crafting stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Stats")
	int32 CraftingLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Stats")
	int32 CraftingExperience;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Stats")
	float CraftingSpeedMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Stats")
	float CraftingSuccessBonus;

	// Reference to inventory
	UPROPERTY(BlueprintReadOnly, Category = "Components")
	UOdysseyInventoryComponent* InventoryComponent;

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Recipe management
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	TArray<FName> GetAvailableRecipes() const;

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	FCraftingRecipe GetRecipe(FName RecipeID) const;

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool IsRecipeUnlocked(FName RecipeID) const;

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool CanCraftRecipe(FName RecipeID, int32 Quantity = 1) const;

	// Crafting operations
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool StartCrafting(FName RecipeID, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool CancelCrafting(int32 QueueIndex);

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void ClearCraftingQueue();

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool InstantCraftRecipe(FName RecipeID, int32 Quantity = 1);

	// Queue management
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	TArray<FCraftingQueue> GetCraftingQueue() const { return CraftingQueue; }

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	int32 GetQueueSize() const { return CraftingQueue.Num(); }

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool IsCrafting() const;

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	float GetTotalCraftingTime() const;

	// Crafting stats
	UFUNCTION(BlueprintCallable, Category = "Crafting Stats")
	void AddCraftingExperience(int32 Experience);

	UFUNCTION(BlueprintCallable, Category = "Crafting Stats")
	int32 GetExperienceToNextLevel() const;

	UFUNCTION(BlueprintCallable, Category = "Crafting Stats")
	bool CanLevelUp() const;

	UFUNCTION(BlueprintCallable, Category = "Crafting Stats")
	void LevelUpCrafting();

	// Utility functions
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void SetInventoryComponent(UOdysseyInventoryComponent* NewInventory);

	// Events
	UFUNCTION(BlueprintImplementableEvent, Category = "Crafting Events")
	void OnCraftingStarted(FName RecipeID, int32 Quantity);

	UFUNCTION(BlueprintImplementableEvent, Category = "Crafting Events")
	void OnCraftingCompleted(FName RecipeID, int32 Quantity, bool bWasSuccessful);

	UFUNCTION(BlueprintImplementableEvent, Category = "Crafting Events")
	void OnCraftingCancelled(FName RecipeID);

	UFUNCTION(BlueprintImplementableEvent, Category = "Crafting Events")
	void OnCraftingLevelUp(int32 NewLevel);

	UFUNCTION(BlueprintImplementableEvent, Category = "Crafting Events")
	void OnRecipeUnlocked(FName RecipeID);

protected:
	void ProcessCraftingQueue(float DeltaTime);
	void CompleteCraftingItem(int32 QueueIndex);
	bool ConsumeIngredients(const FCraftingRecipe& Recipe, int32 Quantity);
	void ProduceOutputs(const FCraftingRecipe& Recipe, int32 Quantity);
	float CalculateActualCraftingTime(float BaseTime) const;
	float CalculateSuccessChance(float BaseChance) const;
	int32 CalculateExperienceRequirement(int32 Level) const;
};