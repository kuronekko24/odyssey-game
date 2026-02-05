#include "OdysseyInventoryComponent.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"

UOdysseyInventoryComponent::UOdysseyInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	MaxCapacity = 20; // Default inventory size
	Inventory.Reserve(MaxCapacity);
}

void UOdysseyInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize with empty inventory
	Inventory.Reset();

	UE_LOG(LogTemp, Warning, TEXT("Inventory component initialized with capacity: %d"), MaxCapacity);
}

bool UOdysseyInventoryComponent::AddResource(EResourceType ResourceType, int32 Amount)
{
	if (ResourceType == EResourceType::None || Amount <= 0)
		return false;

	int32 RemainingAmount = Amount;

	// Try to add to existing stacks first
	for (FResourceStack& Stack : Inventory)
	{
		if (Stack.ResourceType == ResourceType && !Stack.IsFull())
		{
			RemainingAmount = Stack.AddAmount(RemainingAmount);
			if (RemainingAmount == 0)
				break;
		}
	}

	// Create new stacks if needed and if we have space
	FResourceData ResourceData = GetResourceData(ResourceType);
	while (RemainingAmount > 0 && GetUsedSlots() < MaxCapacity)
	{
		FResourceStack NewStack(ResourceType, 0, ResourceData.MaxStackSize);
		int32 AddedAmount = NewStack.AddAmount(RemainingAmount);
		RemainingAmount -= AddedAmount;

		if (NewStack.Amount > 0)
		{
			Inventory.Add(NewStack);
		}
	}

	// Check if we successfully added the full amount
	bool bFullyAdded = (RemainingAmount == 0);

	if (Amount > RemainingAmount) // Some amount was added
	{
		OnResourceAdded(ResourceType, Amount - RemainingAmount);
		OnInventoryChanged();

		UE_LOG(LogTemp, Verbose, TEXT("Added %d %s to inventory"),
			Amount - RemainingAmount, *GetResourceName(ResourceType));
	}

	if (RemainingAmount > 0)
	{
		OnInventoryFull();
		UE_LOG(LogTemp, Warning, TEXT("Inventory full! Could not add %d %s"),
			RemainingAmount, *GetResourceName(ResourceType));
	}

	return bFullyAdded;
}

bool UOdysseyInventoryComponent::RemoveResource(EResourceType ResourceType, int32 Amount)
{
	if (ResourceType == EResourceType::None || Amount <= 0)
		return false;

	if (!HasResource(ResourceType, Amount))
		return false;

	int32 RemainingToRemove = Amount;

	// Remove from stacks (iterate backwards to safely remove empty stacks)
	for (int32 i = Inventory.Num() - 1; i >= 0 && RemainingToRemove > 0; i--)
	{
		FResourceStack& Stack = Inventory[i];
		if (Stack.ResourceType == ResourceType)
		{
			int32 RemoveAmount = FMath::Min(RemainingToRemove, Stack.Amount);
			Stack.Amount -= RemoveAmount;
			RemainingToRemove -= RemoveAmount;

			if (Stack.IsEmpty())
			{
				Inventory.RemoveAt(i);
			}
		}
	}

	OnResourceRemoved(ResourceType, Amount);
	OnInventoryChanged();

	UE_LOG(LogTemp, Verbose, TEXT("Removed %d %s from inventory"),
		Amount, *GetResourceName(ResourceType));

	return true;
}

bool UOdysseyInventoryComponent::HasResource(EResourceType ResourceType, int32 Amount) const
{
	return GetResourceAmount(ResourceType) >= Amount;
}

int32 UOdysseyInventoryComponent::GetResourceAmount(EResourceType ResourceType) const
{
	int32 TotalAmount = 0;

	for (const FResourceStack& Stack : Inventory)
	{
		if (Stack.ResourceType == ResourceType)
		{
			TotalAmount += Stack.Amount;
		}
	}

	return TotalAmount;
}

void UOdysseyInventoryComponent::ClearInventory()
{
	Inventory.Empty();
	OnInventoryChanged();

	UE_LOG(LogTemp, Warning, TEXT("Inventory cleared"));
}

int32 UOdysseyInventoryComponent::GetUsedSlots() const
{
	return Inventory.Num();
}

int32 UOdysseyInventoryComponent::GetAvailableSlots() const
{
	return MaxCapacity - GetUsedSlots();
}

bool UOdysseyInventoryComponent::IsInventoryFull() const
{
	return GetUsedSlots() >= MaxCapacity;
}

void UOdysseyInventoryComponent::SetMaxCapacity(int32 NewCapacity)
{
	MaxCapacity = FMath::Max(1, NewCapacity);

	// Remove excess items if capacity was reduced
	while (Inventory.Num() > MaxCapacity)
	{
		Inventory.RemoveAt(Inventory.Num() - 1);
	}

	OnInventoryChanged();

	UE_LOG(LogTemp, Warning, TEXT("Inventory capacity changed to: %d"), MaxCapacity);
}

FResourceData UOdysseyInventoryComponent::GetResourceData(EResourceType ResourceType) const
{
	if (ResourceDataTable)
	{
		FString ContextString = TEXT("Resource Data Lookup");
		FName RowName = FName(*FString::FromInt((int32)ResourceType));

		FResourceData* ResourceData = ResourceDataTable->FindRow<FResourceData>(RowName, ContextString);
		if (ResourceData)
		{
			return *ResourceData;
		}
	}

	// Return default data if not found
	FResourceData DefaultData;
	DefaultData.ResourceType = ResourceType;

	switch (ResourceType)
	{
		case EResourceType::Silicate:
			DefaultData.Name = TEXT("Silicate");
			DefaultData.Description = TEXT("Raw silicate ore");
			DefaultData.BaseValue = 2;
			break;
		case EResourceType::Carbon:
			DefaultData.Name = TEXT("Carbon");
			DefaultData.Description = TEXT("Raw carbon deposits");
			DefaultData.BaseValue = 3;
			break;
		case EResourceType::RefinedSilicate:
			DefaultData.Name = TEXT("Refined Silicate");
			DefaultData.Description = TEXT("Processed silicate material");
			DefaultData.BaseValue = 8;
			break;
		case EResourceType::RefinedCarbon:
			DefaultData.Name = TEXT("Refined Carbon");
			DefaultData.Description = TEXT("Processed carbon material");
			DefaultData.BaseValue = 12;
			break;
		case EResourceType::CompositeMaterial:
			DefaultData.Name = TEXT("Composite Material");
			DefaultData.Description = TEXT("Advanced composite material");
			DefaultData.BaseValue = 25;
			break;
		case EResourceType::OMEN:
			DefaultData.Name = TEXT("OMEN");
			DefaultData.Description = TEXT("Galactic currency");
			DefaultData.MaxStackSize = 10000;
			DefaultData.BaseValue = 1;
			break;
		default:
			DefaultData.Name = TEXT("Unknown");
			break;
	}

	return DefaultData;
}

FString UOdysseyInventoryComponent::GetResourceName(EResourceType ResourceType) const
{
	return GetResourceData(ResourceType).Name;
}

int32 UOdysseyInventoryComponent::GetResourceValue(EResourceType ResourceType) const
{
	return GetResourceData(ResourceType).BaseValue;
}

void UOdysseyInventoryComponent::SortInventory()
{
	Inventory.Sort([](const FResourceStack& A, const FResourceStack& B) {
		return (int32)A.ResourceType < (int32)B.ResourceType;
	});
}

int32 UOdysseyInventoryComponent::FindResourceStack(EResourceType ResourceType) const
{
	for (int32 i = 0; i < Inventory.Num(); i++)
	{
		if (Inventory[i].ResourceType == ResourceType)
		{
			return i;
		}
	}
	return -1;
}

void UOdysseyInventoryComponent::CleanupEmptyStacks()
{
	for (int32 i = Inventory.Num() - 1; i >= 0; i--)
	{
		if (Inventory[i].IsEmpty())
		{
			Inventory.RemoveAt(i);
		}
	}
}