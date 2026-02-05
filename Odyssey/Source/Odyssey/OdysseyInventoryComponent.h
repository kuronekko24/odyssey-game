#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "OdysseyInventoryComponent.generated.h"

UENUM(BlueprintType)
enum class EResourceType : uint8
{
	None = 0,
	Silicate = 1,
	Carbon = 2,
	RefinedSilicate = 10,
	RefinedCarbon = 11,
	CompositeMaterial = 20,
	OMEN = 100 // Currency
};

USTRUCT(BlueprintType)
struct ODYSSEY_API FResourceStack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	EResourceType ResourceType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	int32 Amount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	int32 MaxStackSize;

	FResourceStack()
	{
		ResourceType = EResourceType::None;
		Amount = 0;
		MaxStackSize = 100;
	}

	FResourceStack(EResourceType Type, int32 Quantity, int32 MaxStack = 100)
	{
		ResourceType = Type;
		Amount = Quantity;
		MaxStackSize = MaxStack;
	}

	bool CanAddAmount(int32 AmountToAdd) const
	{
		return (Amount + AmountToAdd) <= MaxStackSize;
	}

	int32 AddAmount(int32 AmountToAdd)
	{
		int32 CanAdd = FMath::Min(AmountToAdd, MaxStackSize - Amount);
		Amount += CanAdd;
		return AmountToAdd - CanAdd; // Return overflow
	}

	bool IsEmpty() const { return Amount <= 0; }
	bool IsFull() const { return Amount >= MaxStackSize; }
};

USTRUCT(BlueprintType, meta = (DataTable))
struct ODYSSEY_API FResourceData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	EResourceType ResourceType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	int32 MaxStackSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	int32 BaseValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	bool bIsCraftable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	bool bIsSellable;

	FResourceData()
	{
		ResourceType = EResourceType::None;
		Name = TEXT("Unknown");
		Description = TEXT("Unknown resource");
		MaxStackSize = 100;
		BaseValue = 1;
		bIsCraftable = false;
		bIsSellable = true;
	}
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UOdysseyInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdysseyInventoryComponent();

protected:
	// Inventory data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FResourceStack> Inventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 MaxCapacity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	UDataTable* ResourceDataTable;

protected:
	virtual void BeginPlay() override;

public:
	// Inventory management
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddResource(EResourceType ResourceType, int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveResource(EResourceType ResourceType, int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool HasResource(EResourceType ResourceType, int32 Amount) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetResourceAmount(EResourceType ResourceType) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetUsedSlots() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetAvailableSlots() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool IsInventoryFull() const;

	// Inventory access
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<FResourceStack> GetInventory() const { return Inventory; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetMaxCapacity(int32 NewCapacity);

	// Resource data lookup
	UFUNCTION(BlueprintCallable, Category = "Resource Data")
	FResourceData GetResourceData(EResourceType ResourceType) const;

	UFUNCTION(BlueprintCallable, Category = "Resource Data")
	FString GetResourceName(EResourceType ResourceType) const;

	UFUNCTION(BlueprintCallable, Category = "Resource Data")
	int32 GetResourceValue(EResourceType ResourceType) const;

	// Events
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Events")
	void OnInventoryChanged();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Events")
	void OnResourceAdded(EResourceType ResourceType, int32 Amount);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Events")
	void OnResourceRemoved(EResourceType ResourceType, int32 Amount);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Events")
	void OnInventoryFull();

private:
	void SortInventory();
	int32 FindResourceStack(EResourceType ResourceType) const;
	void CleanupEmptyStacks();
};