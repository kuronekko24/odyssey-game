#include "OdysseyTradingComponent.h"
#include "OdysseyInventoryComponent.h"
#include "OdysseyCharacter.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"

UOdysseyTradingComponent::UOdysseyTradingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Market settings
	MarketUpdateInterval = 30.0f; // Update prices every 30 seconds
	PriceFluctuationRange = 0.15f; // ±15% price fluctuation
	SellPriceMultiplier = 0.8f; // Players get 80% of buy price when selling

	// Trading statistics
	TotalOMENEarned = 0;
	TotalOMENSpent = 0;
	MarketUpdateTimer = 0.0f;

	InventoryComponent = nullptr;
}

void UOdysseyTradingComponent::BeginPlay()
{
	Super::BeginPlay();

	// Try to find inventory component
	if (!InventoryComponent)
	{
		InventoryComponent = GetOwner()->FindComponentByClass<UOdysseyInventoryComponent>();
	}

	// Initialize market prices
	InitializeMarketPrices();

	// Give player some starting OMEN for demo
	if (InventoryComponent)
	{
		InventoryComponent->AddResource(EResourceType::OMEN, 50);
	}

	UE_LOG(LogTemp, Warning, TEXT("Trading component initialized with %d market prices"),
		CurrentMarketPrices.Num());
}

void UOdysseyTradingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update market prices periodically
	MarketUpdateTimer += DeltaTime;
	if (MarketUpdateTimer >= MarketUpdateInterval)
	{
		UpdateMarketPrices();
		MarketUpdateTimer = 0.0f;
	}
}

bool UOdysseyTradingComponent::SellResource(EResourceType ResourceType, int32 Quantity)
{
	if (!CanSellResource(ResourceType, Quantity))
		return false;

	int32 TotalPrice = GetSellPrice(ResourceType, Quantity);

	// Remove resources from inventory
	if (!InventoryComponent->RemoveResource(ResourceType, Quantity))
		return false;

	// Add OMEN to inventory
	InventoryComponent->AddResource(EResourceType::OMEN, TotalPrice);

	// Update statistics
	TotalOMENEarned += TotalPrice;

	OnResourceSold(ResourceType, Quantity, TotalPrice);

	UE_LOG(LogTemp, Warning, TEXT("Sold %d %s for %d OMEN"),
		Quantity, *InventoryComponent->GetResourceName(ResourceType), TotalPrice);

	return true;
}

bool UOdysseyTradingComponent::BuyResource(EResourceType ResourceType, int32 Quantity)
{
	if (!CanBuyResource(ResourceType, Quantity))
		return false;

	int32 TotalPrice = GetBuyPrice(ResourceType, Quantity);

	// Remove OMEN from inventory
	if (!InventoryComponent->RemoveResource(EResourceType::OMEN, TotalPrice))
		return false;

	// Add resources to inventory
	InventoryComponent->AddResource(ResourceType, Quantity);

	// Update statistics
	TotalOMENSpent += TotalPrice;

	OnResourceBought(ResourceType, Quantity, TotalPrice);

	UE_LOG(LogTemp, Warning, TEXT("Bought %d %s for %d OMEN"),
		Quantity, *InventoryComponent->GetResourceName(ResourceType), TotalPrice);

	return true;
}

int32 UOdysseyTradingComponent::GetSellPrice(EResourceType ResourceType, int32 Quantity) const
{
	FCurrentMarketPrice MarketPrice = GetMarketPrice(ResourceType);
	return MarketPrice.SellPrice * Quantity;
}

int32 UOdysseyTradingComponent::GetBuyPrice(EResourceType ResourceType, int32 Quantity) const
{
	FCurrentMarketPrice MarketPrice = GetMarketPrice(ResourceType);
	return MarketPrice.BuyPrice * Quantity;
}

bool UOdysseyTradingComponent::CanSellResource(EResourceType ResourceType, int32 Quantity) const
{
	if (!InventoryComponent)
		return false;

	FCurrentMarketPrice MarketPrice = GetMarketPrice(ResourceType);
	if (!MarketPrice.bCanSell)
		return false;

	return InventoryComponent->HasResource(ResourceType, Quantity);
}

bool UOdysseyTradingComponent::CanBuyResource(EResourceType ResourceType, int32 Quantity) const
{
	if (!InventoryComponent)
		return false;

	FCurrentMarketPrice MarketPrice = GetMarketPrice(ResourceType);
	if (!MarketPrice.bCanBuy)
		return false;

	int32 TotalPrice = GetBuyPrice(ResourceType, Quantity);
	return HasOMEN(TotalPrice);
}

FCurrentMarketPrice UOdysseyTradingComponent::GetMarketPrice(EResourceType ResourceType) const
{
	for (const FCurrentMarketPrice& Price : CurrentMarketPrices)
	{
		if (Price.ResourceType == ResourceType)
		{
			return Price;
		}
	}

	// Return default if not found
	FCurrentMarketPrice DefaultPrice;
	DefaultPrice.ResourceType = ResourceType;
	return DefaultPrice;
}

void UOdysseyTradingComponent::UpdateMarketPrices()
{
	if (!MarketPriceDataTable)
		return;

	for (FCurrentMarketPrice& Price : CurrentMarketPrices)
	{
		// Find base data for this resource
		FString ContextString = TEXT("Market Price Update");
		FName RowName = FName(*FString::FromInt((int32)Price.ResourceType));

		FMarketPriceData* BaseData = MarketPriceDataTable->FindRow<FMarketPriceData>(RowName, ContextString);
		if (BaseData)
		{
			Price.BuyPrice = CalculateCurrentPrice(*BaseData, false);
			Price.SellPrice = CalculateCurrentPrice(*BaseData, true);
		}
	}

	OnMarketPricesUpdated();

	UE_LOG(LogTemp, Verbose, TEXT("Market prices updated"));
}

TArray<FName> UOdysseyTradingComponent::GetAvailableUpgrades() const
{
	TArray<FName> AvailableUpgrades;

	if (UpgradeDataTable)
	{
		TArray<FName> AllRowNames = UpgradeDataTable->GetRowNames();
		for (const FName& RowName : AllRowNames)
		{
			FUpgradeData UpgradeData = GetUpgradeData(RowName);
			if (UpgradeData.bIsUnlocked && !IsUpgradeMaxed(RowName))
			{
				AvailableUpgrades.Add(RowName);
			}
		}
	}

	return AvailableUpgrades;
}

FUpgradeData UOdysseyTradingComponent::GetUpgradeData(FName UpgradeID) const
{
	if (UpgradeDataTable)
	{
		FString ContextString = TEXT("Upgrade Data Lookup");
		FUpgradeData* Upgrade = UpgradeDataTable->FindRow<FUpgradeData>(UpgradeID, ContextString);
		if (Upgrade)
		{
			return *Upgrade;
		}
	}

	return FUpgradeData(); // Return default if not found
}

bool UOdysseyTradingComponent::CanPurchaseUpgrade(FName UpgradeID) const
{
	FUpgradeData Upgrade = GetUpgradeData(UpgradeID);

	if (!Upgrade.bIsUnlocked || IsUpgradeMaxed(UpgradeID))
		return false;

	return HasOMEN(Upgrade.OMENCost);
}

bool UOdysseyTradingComponent::PurchaseUpgrade(FName UpgradeID)
{
	if (!CanPurchaseUpgrade(UpgradeID))
		return false;

	FUpgradeData Upgrade = GetUpgradeData(UpgradeID);

	// Spend OMEN
	if (!SpendOMEN(Upgrade.OMENCost))
		return false;

	// Track purchase
	bool bFound = false;
	for (FPurchasedUpgrade& PurchasedUpgrade : PurchasedUpgrades)
	{
		if (PurchasedUpgrade.UpgradeID == UpgradeID)
		{
			PurchasedUpgrade.PurchaseCount++;
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		PurchasedUpgrades.Add(FPurchasedUpgrade(UpgradeID, 1));
	}

	// Apply upgrade effects
	ApplyUpgradeEffects(Upgrade);

	OnUpgradePurchased(UpgradeID, Upgrade.OMENCost);

	UE_LOG(LogTemp, Warning, TEXT("Purchased upgrade: %s for %d OMEN"),
		*Upgrade.UpgradeName, Upgrade.OMENCost);

	return true;
}

int32 UOdysseyTradingComponent::GetUpgradePurchaseCount(FName UpgradeID) const
{
	for (const FPurchasedUpgrade& PurchasedUpgrade : PurchasedUpgrades)
	{
		if (PurchasedUpgrade.UpgradeID == UpgradeID)
		{
			return PurchasedUpgrade.PurchaseCount;
		}
	}
	return 0;
}

bool UOdysseyTradingComponent::IsUpgradeMaxed(FName UpgradeID) const
{
	FUpgradeData Upgrade = GetUpgradeData(UpgradeID);
	int32 PurchaseCount = GetUpgradePurchaseCount(UpgradeID);

	return PurchaseCount >= Upgrade.MaxPurchases;
}

int32 UOdysseyTradingComponent::GetOMENAmount() const
{
	if (InventoryComponent)
	{
		return InventoryComponent->GetResourceAmount(EResourceType::OMEN);
	}
	return 0;
}

bool UOdysseyTradingComponent::HasOMEN(int32 Amount) const
{
	return GetOMENAmount() >= Amount;
}

bool UOdysseyTradingComponent::AddOMEN(int32 Amount)
{
	if (InventoryComponent)
	{
		return InventoryComponent->AddResource(EResourceType::OMEN, Amount);
	}
	return false;
}

bool UOdysseyTradingComponent::SpendOMEN(int32 Amount)
{
	if (InventoryComponent && HasOMEN(Amount))
	{
		return InventoryComponent->RemoveResource(EResourceType::OMEN, Amount);
	}
	return false;
}

void UOdysseyTradingComponent::SetInventoryComponent(UOdysseyInventoryComponent* NewInventory)
{
	InventoryComponent = NewInventory;
}

void UOdysseyTradingComponent::InitializeMarketPrices()
{
	if (!MarketPriceDataTable)
	{
		// Create default prices for demo
		TArray<EResourceType> DefaultResources = {
			EResourceType::Silicate,
			EResourceType::Carbon,
			EResourceType::RefinedSilicate,
			EResourceType::RefinedCarbon,
			EResourceType::CompositeMaterial
		};

		for (EResourceType ResourceType : DefaultResources)
		{
			FCurrentMarketPrice Price;
			Price.ResourceType = ResourceType;
			Price.bCanSell = true;
			Price.bCanBuy = false; // Players can only sell for now

			switch (ResourceType)
			{
				case EResourceType::Silicate:
					Price.BuyPrice = 3;
					Price.SellPrice = 2;
					break;
				case EResourceType::Carbon:
					Price.BuyPrice = 4;
					Price.SellPrice = 3;
					break;
				case EResourceType::RefinedSilicate:
					Price.BuyPrice = 12;
					Price.SellPrice = 10;
					break;
				case EResourceType::RefinedCarbon:
					Price.BuyPrice = 18;
					Price.SellPrice = 15;
					break;
				case EResourceType::CompositeMaterial:
					Price.BuyPrice = 40;
					Price.SellPrice = 35;
					break;
			}

			CurrentMarketPrices.Add(Price);
		}
		return;
	}

	// Load from data table
	TArray<FName> RowNames = MarketPriceDataTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		FString ContextString = TEXT("Market Price Initialization");
		FMarketPriceData* BaseData = MarketPriceDataTable->FindRow<FMarketPriceData>(RowName, ContextString);
		if (BaseData)
		{
			FCurrentMarketPrice Price;
			Price.ResourceType = BaseData->ResourceType;
			Price.BuyPrice = CalculateCurrentPrice(*BaseData, false);
			Price.SellPrice = CalculateCurrentPrice(*BaseData, true);
			Price.bCanBuy = BaseData->bCanBuy;
			Price.bCanSell = BaseData->bCanSell;

			CurrentMarketPrices.Add(Price);
		}
	}
}

void UOdysseyTradingComponent::ApplyUpgradeEffects(const FUpgradeData& Upgrade)
{
	// Apply upgrade effects to the character
	AOdysseyCharacter* Character = Cast<AOdysseyCharacter>(GetOwner());
	if (!Character)
		return;

	if (Upgrade.MiningPowerIncrease > 0.0f)
	{
		Character->UpgradeMiningPower(Upgrade.MiningPowerIncrease);
	}

	if (Upgrade.MiningSpeedIncrease > 0.0f)
	{
		Character->UpgradeMiningSpeed(Upgrade.MiningSpeedIncrease);
	}

	if (Upgrade.InventoryCapacityIncrease > 0)
	{
		Character->UpgradeInventoryCapacity(Upgrade.InventoryCapacityIncrease);
	}

	if (Upgrade.CraftingSpeedIncrease > 0.0f)
	{
		// Apply to crafting component if available
		if (Character->GetCraftingComponent())
		{
			Character->GetCraftingComponent()->CraftingSpeedMultiplier += Upgrade.CraftingSpeedIncrease;
		}
	}
}

int32 UOdysseyTradingComponent::CalculateCurrentPrice(const FMarketPriceData& BaseData, bool bForSelling) const
{
	// Add some randomness to prices
	float Fluctuation = FMath::RandRange(-PriceFluctuationRange, PriceFluctuationRange);
	float PriceMultiplier = 1.0f + Fluctuation;

	if (bForSelling)
	{
		PriceMultiplier *= SellPriceMultiplier; // Players get less when selling
	}

	int32 CalculatedPrice = FMath::RoundToInt(BaseData.BasePrice * PriceMultiplier);
	return FMath::Clamp(CalculatedPrice, BaseData.MinPrice, BaseData.MaxPrice);
}