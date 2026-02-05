#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "OdysseyInventoryComponent.h"
#include "OdysseyTradingComponent.generated.h"

USTRUCT(BlueprintType, meta = (DataTable))
struct ODYSSEY_API FMarketPriceData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	EResourceType ResourceType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	int32 BasePrice;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	float PriceVolatility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	int32 MinPrice;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	int32 MaxPrice;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	bool bCanBuy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	bool bCanSell;

	FMarketPriceData()
	{
		ResourceType = EResourceType::None;
		BasePrice = 1;
		PriceVolatility = 0.1f;
		MinPrice = 1;
		MaxPrice = 100;
		bCanBuy = false;
		bCanSell = true;
	}
};

USTRUCT(BlueprintType)
struct ODYSSEY_API FCurrentMarketPrice
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Market")
	EResourceType ResourceType;

	UPROPERTY(BlueprintReadOnly, Category = "Market")
	int32 BuyPrice;

	UPROPERTY(BlueprintReadOnly, Category = "Market")
	int32 SellPrice;

	UPROPERTY(BlueprintReadOnly, Category = "Market")
	bool bCanBuy;

	UPROPERTY(BlueprintReadOnly, Category = "Market")
	bool bCanSell;

	FCurrentMarketPrice()
	{
		ResourceType = EResourceType::None;
		BuyPrice = 0;
		SellPrice = 0;
		bCanBuy = false;
		bCanSell = false;
	}
};

USTRUCT(BlueprintType, meta = (DataTable))
struct ODYSSEY_API FUpgradeData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FString UpgradeName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	int32 OMENCost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FString UpgradeCategory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	float MiningPowerIncrease;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	float MiningSpeedIncrease;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	int32 InventoryCapacityIncrease;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	float CraftingSpeedIncrease;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	int32 MaxPurchases;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	bool bIsUnlocked;

	FUpgradeData()
	{
		UpgradeName = TEXT("Unknown Upgrade");
		Description = TEXT("No description");
		OMENCost = 100;
		UpgradeCategory = TEXT("General");
		MiningPowerIncrease = 0.0f;
		MiningSpeedIncrease = 0.0f;
		InventoryCapacityIncrease = 0;
		CraftingSpeedIncrease = 0.0f;
		MaxPurchases = 1;
		bIsUnlocked = true;
	}
};

USTRUCT(BlueprintType)
struct ODYSSEY_API FPurchasedUpgrade
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Purchased Upgrade")
	FName UpgradeID;

	UPROPERTY(BlueprintReadOnly, Category = "Purchased Upgrade")
	int32 PurchaseCount;

	FPurchasedUpgrade()
	{
		UpgradeID = NAME_None;
		PurchaseCount = 0;
	}

	FPurchasedUpgrade(FName ID, int32 Count)
	{
		UpgradeID = ID;
		PurchaseCount = Count;
	}
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UOdysseyTradingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdysseyTradingComponent();

protected:
	// Trading data tables
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trading")
	UDataTable* MarketPriceDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trading")
	UDataTable* UpgradeDataTable;

	// Current market state
	UPROPERTY(BlueprintReadOnly, Category = "Market")
	TArray<FCurrentMarketPrice> CurrentMarketPrices;

	// Market settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	float MarketUpdateInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	float PriceFluctuationRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	float SellPriceMultiplier;

	// Player trading history
	UPROPERTY(BlueprintReadOnly, Category = "Trading History")
	TArray<FPurchasedUpgrade> PurchasedUpgrades;

	UPROPERTY(BlueprintReadOnly, Category = "Trading History")
	int32 TotalOMENEarned;

	UPROPERTY(BlueprintReadOnly, Category = "Trading History")
	int32 TotalOMENSpent;

	// Component references
	UPROPERTY(BlueprintReadOnly, Category = "Components")
	UOdysseyInventoryComponent* InventoryComponent;

	// Timer for market updates
	float MarketUpdateTimer;

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Market operations
	UFUNCTION(BlueprintCallable, Category = "Trading")
	bool SellResource(EResourceType ResourceType, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "Trading")
	bool BuyResource(EResourceType ResourceType, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "Trading")
	int32 GetSellPrice(EResourceType ResourceType, int32 Quantity) const;

	UFUNCTION(BlueprintCallable, Category = "Trading")
	int32 GetBuyPrice(EResourceType ResourceType, int32 Quantity) const;

	UFUNCTION(BlueprintCallable, Category = "Trading")
	bool CanSellResource(EResourceType ResourceType, int32 Quantity) const;

	UFUNCTION(BlueprintCallable, Category = "Trading")
	bool CanBuyResource(EResourceType ResourceType, int32 Quantity) const;

	// Market info
	UFUNCTION(BlueprintCallable, Category = "Market")
	TArray<FCurrentMarketPrice> GetCurrentMarketPrices() const { return CurrentMarketPrices; }

	UFUNCTION(BlueprintCallable, Category = "Market")
	FCurrentMarketPrice GetMarketPrice(EResourceType ResourceType) const;

	UFUNCTION(BlueprintCallable, Category = "Market")
	void UpdateMarketPrices();

	// Upgrade system
	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	TArray<FName> GetAvailableUpgrades() const;

	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	FUpgradeData GetUpgradeData(FName UpgradeID) const;

	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	bool CanPurchaseUpgrade(FName UpgradeID) const;

	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	bool PurchaseUpgrade(FName UpgradeID);

	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	int32 GetUpgradePurchaseCount(FName UpgradeID) const;

	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	bool IsUpgradeMaxed(FName UpgradeID) const;

	// OMEN currency
	UFUNCTION(BlueprintCallable, Category = "Currency")
	int32 GetOMENAmount() const;

	UFUNCTION(BlueprintCallable, Category = "Currency")
	bool HasOMEN(int32 Amount) const;

	UFUNCTION(BlueprintCallable, Category = "Currency")
	bool AddOMEN(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Currency")
	bool SpendOMEN(int32 Amount);

	// Trading statistics
	UFUNCTION(BlueprintCallable, Category = "Statistics")
	int32 GetTotalOMENEarned() const { return TotalOMENEarned; }

	UFUNCTION(BlueprintCallable, Category = "Statistics")
	int32 GetTotalOMENSpent() const { return TotalOMENSpent; }

	UFUNCTION(BlueprintCallable, Category = "Statistics")
	int32 GetNetOMEN() const { return TotalOMENEarned - TotalOMENSpent; }

	// Component setup
	UFUNCTION(BlueprintCallable, Category = "Setup")
	void SetInventoryComponent(UOdysseyInventoryComponent* NewInventory);

	// Events
	UFUNCTION(BlueprintImplementableEvent, Category = "Trading Events")
	void OnResourceSold(EResourceType ResourceType, int32 Quantity, int32 OMENReceived);

	UFUNCTION(BlueprintImplementableEvent, Category = "Trading Events")
	void OnResourceBought(EResourceType ResourceType, int32 Quantity, int32 OMENSpent);

	UFUNCTION(BlueprintImplementableEvent, Category = "Trading Events")
	void OnUpgradePurchased(FName UpgradeID, int32 OMENSpent);

	UFUNCTION(BlueprintImplementableEvent, Category = "Trading Events")
	void OnMarketPricesUpdated();

private:
	void InitializeMarketPrices();
	void ApplyUpgradeEffects(const FUpgradeData& Upgrade);
	int32 CalculateCurrentPrice(const FMarketPriceData& BaseData, bool bForSelling) const;
};