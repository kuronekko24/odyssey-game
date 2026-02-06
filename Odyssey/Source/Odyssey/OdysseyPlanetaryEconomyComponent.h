// OdysseyPlanetaryEconomyComponent.h
// Planetary economic specialization and trade goods system
// Part of the Odyssey Procedural Planet & Resource Generation System

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OdysseyInventoryComponent.h"
#include "OdysseyBiomeDefinitionSystem.h"
#include "OdysseyPlanetGenerator.h"
#include "OdysseyPlanetaryEconomyComponent.generated.h"

/**
 * Enum for economic specialization types
 */
UENUM(BlueprintType)
enum class EEconomicSpecialization : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	Mining = 1 UMETA(DisplayName = "Mining"),
	Agriculture = 2 UMETA(DisplayName = "Agriculture"),
	Manufacturing = 3 UMETA(DisplayName = "Manufacturing"),
	Technology = 4 UMETA(DisplayName = "Technology"),
	Trade = 5 UMETA(DisplayName = "Trade Hub"),
	Research = 6 UMETA(DisplayName = "Research"),
	Military = 7 UMETA(DisplayName = "Military"),
	Tourism = 8 UMETA(DisplayName = "Tourism"),
	Energy = 9 UMETA(DisplayName = "Energy Production")
};

/**
 * Struct for trade good definition
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FTradeGood
{
	GENERATED_BODY()

	// Unique identifier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade Good")
	FName GoodID;

	// Display name
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade Good")
	FText DisplayName;

	// Description
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade Good")
	FText Description;

	// Base value in OMEN
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade Good")
	int32 BaseValue;

	// Volume per unit (for cargo capacity)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade Good")
	float VolumePerUnit;

	// Associated resource type (if any)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade Good")
	EResourceType SourceResource;

	// Required specialization to produce
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade Good")
	EEconomicSpecialization ProducingSpecialization;

	// Consuming specializations (demand sources)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade Good")
	TArray<EEconomicSpecialization> ConsumingSpecializations;

	// Legality status (0 = legal everywhere, 1 = restricted, 2 = contraband)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade Good", meta = (ClampMin = "0", ClampMax = "2"))
	int32 LegalityStatus;

	// Perishability (0 = non-perishable, 1 = slow decay, 2 = fast decay)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade Good", meta = (ClampMin = "0", ClampMax = "2"))
	int32 Perishability;

	FTradeGood()
		: GoodID(NAME_None)
		, DisplayName(FText::FromString(TEXT("Unknown Good")))
		, Description(FText::FromString(TEXT("")))
		, BaseValue(10)
		, VolumePerUnit(1.0f)
		, SourceResource(EResourceType::None)
		, ProducingSpecialization(EEconomicSpecialization::None)
		, LegalityStatus(0)
		, Perishability(0)
	{
	}
};

/**
 * Struct for planetary production data
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FPlanetaryProduction
{
	GENERATED_BODY()

	// Trade good being produced
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
	FName GoodID;

	// Production rate per day
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
	int32 ProductionRate;

	// Current stockpile
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
	int32 CurrentStock;

	// Maximum storage capacity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
	int32 MaxStorage;

	// Production efficiency (0.0 - 2.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float Efficiency;

	// Is production active
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
	bool bIsActive;

	FPlanetaryProduction()
		: GoodID(NAME_None)
		, ProductionRate(10)
		, CurrentStock(0)
		, MaxStorage(1000)
		, Efficiency(1.0f)
		, bIsActive(true)
	{
	}
};

/**
 * Struct for planetary consumption data
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FPlanetaryConsumption
{
	GENERATED_BODY()

	// Trade good being consumed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumption")
	FName GoodID;

	// Consumption rate per day
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumption")
	int32 ConsumptionRate;

	// Current demand (stockpile needed)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumption")
	int32 CurrentDemand;

	// Current stockpile
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumption")
	int32 CurrentStock;

	// Urgency level (0 = low, 1 = medium, 2 = critical)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumption", meta = (ClampMin = "0", ClampMax = "2"))
	int32 Urgency;

	FPlanetaryConsumption()
		: GoodID(NAME_None)
		, ConsumptionRate(5)
		, CurrentDemand(50)
		, CurrentStock(25)
		, Urgency(0)
	{
	}
};

/**
 * Struct for market price data at a specific location
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FPlanetaryMarketPrice
{
	GENERATED_BODY()

	// Trade good
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	FName GoodID;

	// Current buy price (what players pay)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	int32 BuyPrice;

	// Current sell price (what players receive)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	int32 SellPrice;

	// Available quantity to buy
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	int32 AvailableQuantity;

	// Demand quantity (how much the market wants)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	int32 DemandQuantity;

	// Price trend (-1 = falling, 0 = stable, 1 = rising)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	int32 PriceTrend;

	// Last update timestamp
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	float LastUpdateTime;

	FPlanetaryMarketPrice()
		: GoodID(NAME_None)
		, BuyPrice(10)
		, SellPrice(8)
		, AvailableQuantity(100)
		, DemandQuantity(50)
		, PriceTrend(0)
		, LastUpdateTime(0.0f)
	{
	}
};

/**
 * Struct for economic relationship between planets
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FEconomicRelationship
{
	GENERATED_BODY()

	// Partner planet ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economic Relationship")
	int32 PartnerPlanetID;

	// Relationship strength (0.0 - 1.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economic Relationship", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RelationshipStrength;

	// Trade agreement active
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economic Relationship")
	bool bHasTradeAgreement;

	// Tariff rate (0.0 - 0.5)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economic Relationship", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float TariffRate;

	// Primary export good to this partner
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economic Relationship")
	FName PrimaryExportGood;

	// Primary import good from this partner
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economic Relationship")
	FName PrimaryImportGood;

	// Trade volume last period
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economic Relationship")
	int32 TradeVolume;

	FEconomicRelationship()
		: PartnerPlanetID(0)
		, RelationshipStrength(0.5f)
		, bHasTradeAgreement(false)
		, TariffRate(0.1f)
		, PrimaryExportGood(NAME_None)
		, PrimaryImportGood(NAME_None)
		, TradeVolume(0)
	{
	}
};

/**
 * UOdysseyPlanetaryEconomyComponent
 * 
 * Manages the economic aspects of a planet including specialization,
 * production, consumption, market prices, and trade relationships.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UOdysseyPlanetaryEconomyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdysseyPlanetaryEconomyComponent();

protected:
	// Planet identification
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	int32 PlanetID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	FText PlanetName;

	// Economic specialization
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
	EEconomicSpecialization PrimarySpecialization;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
	EEconomicSpecialization SecondarySpecialization;

	// Economic metrics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy", meta = (ClampMin = "0", ClampMax = "100"))
	int32 WealthLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy", meta = (ClampMin = "0", ClampMax = "100"))
	int32 DevelopmentLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
	int32 Population;

	// Production and consumption
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
	TArray<FPlanetaryProduction> Productions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumption")
	TArray<FPlanetaryConsumption> Consumptions;

	// Market data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	TArray<FPlanetaryMarketPrice> MarketPrices;

	// Economic relationships
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relationships")
	TArray<FEconomicRelationship> EconomicRelationships;

	// Trade good definitions (shared reference)
	UPROPERTY()
	TMap<FName, FTradeGood> TradeGoodDefinitions;

	// Economic update settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float MarketUpdateInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float PriceVolatility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float DemandMultiplier;

	// Timer
	float MarketUpdateTimer;

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Initialization
	UFUNCTION(BlueprintCallable, Category = "Economy Setup")
	void InitializeFromPlanetData(const FGeneratedPlanetData& PlanetData, int32 Seed);

	UFUNCTION(BlueprintCallable, Category = "Economy Setup")
	void InitializeTradeGoods();

	// Specialization
	UFUNCTION(BlueprintCallable, Category = "Economy")
	EEconomicSpecialization GetPrimarySpecialization() const { return PrimarySpecialization; }

	UFUNCTION(BlueprintCallable, Category = "Economy")
	EEconomicSpecialization GetSecondarySpecialization() const { return SecondarySpecialization; }

	UFUNCTION(BlueprintCallable, Category = "Economy")
	void SetSpecializations(EEconomicSpecialization Primary, EEconomicSpecialization Secondary);

	UFUNCTION(BlueprintCallable, Category = "Economy")
	bool HasSpecialization(EEconomicSpecialization Specialization) const;

	// Production
	UFUNCTION(BlueprintCallable, Category = "Production")
	TArray<FPlanetaryProduction> GetProductions() const { return Productions; }

	UFUNCTION(BlueprintCallable, Category = "Production")
	bool IsProducing(FName GoodID) const;

	UFUNCTION(BlueprintCallable, Category = "Production")
	int32 GetProductionStock(FName GoodID) const;

	UFUNCTION(BlueprintCallable, Category = "Production")
	void AddProduction(const FPlanetaryProduction& Production);

	UFUNCTION(BlueprintCallable, Category = "Production")
	void RemoveProduction(FName GoodID);

	// Consumption
	UFUNCTION(BlueprintCallable, Category = "Consumption")
	TArray<FPlanetaryConsumption> GetConsumptions() const { return Consumptions; }

	UFUNCTION(BlueprintCallable, Category = "Consumption")
	bool IsConsuming(FName GoodID) const;

	UFUNCTION(BlueprintCallable, Category = "Consumption")
	int32 GetConsumptionDemand(FName GoodID) const;

	UFUNCTION(BlueprintCallable, Category = "Consumption")
	void AddConsumption(const FPlanetaryConsumption& Consumption);

	// Market operations
	UFUNCTION(BlueprintCallable, Category = "Market")
	FPlanetaryMarketPrice GetMarketPrice(FName GoodID) const;

	UFUNCTION(BlueprintCallable, Category = "Market")
	TArray<FPlanetaryMarketPrice> GetAllMarketPrices() const { return MarketPrices; }

	UFUNCTION(BlueprintCallable, Category = "Market")
	int32 GetBuyPrice(FName GoodID) const;

	UFUNCTION(BlueprintCallable, Category = "Market")
	int32 GetSellPrice(FName GoodID) const;

	UFUNCTION(BlueprintCallable, Category = "Market")
	bool CanBuyGood(FName GoodID, int32 Quantity) const;

	UFUNCTION(BlueprintCallable, Category = "Market")
	bool CanSellGood(FName GoodID, int32 Quantity) const;

	UFUNCTION(BlueprintCallable, Category = "Market")
	bool ExecuteBuy(FName GoodID, int32 Quantity, int32& OutTotalCost);

	UFUNCTION(BlueprintCallable, Category = "Market")
	bool ExecuteSell(FName GoodID, int32 Quantity, int32& OutTotalRevenue);

	UFUNCTION(BlueprintCallable, Category = "Market")
	void UpdateMarketPrices();

	// Trade good info
	UFUNCTION(BlueprintCallable, Category = "Trade Goods")
	FTradeGood GetTradeGoodInfo(FName GoodID) const;

	UFUNCTION(BlueprintCallable, Category = "Trade Goods")
	TArray<FName> GetAllTradeGoodIDs() const;

	UFUNCTION(BlueprintCallable, Category = "Trade Goods")
	TArray<FName> GetProducedGoods() const;

	UFUNCTION(BlueprintCallable, Category = "Trade Goods")
	TArray<FName> GetConsumedGoods() const;

	// Economic relationships
	UFUNCTION(BlueprintCallable, Category = "Relationships")
	FEconomicRelationship GetRelationship(int32 OtherPlanetID) const;

	UFUNCTION(BlueprintCallable, Category = "Relationships")
	void UpdateRelationship(const FEconomicRelationship& Relationship);

	UFUNCTION(BlueprintCallable, Category = "Relationships")
	float GetTariffRate(int32 OtherPlanetID) const;

	UFUNCTION(BlueprintCallable, Category = "Relationships")
	TArray<int32> GetTradingPartners() const;

	// Economic analysis
	UFUNCTION(BlueprintCallable, Category = "Analysis")
	TArray<FName> GetMostProfitableExports() const;

	UFUNCTION(BlueprintCallable, Category = "Analysis")
	TArray<FName> GetMostNeededImports() const;

	UFUNCTION(BlueprintCallable, Category = "Analysis")
	int32 CalculatePotentialProfit(FName GoodID, int32 Quantity, int32 DestinationPlanetID) const;

	// Economic metrics
	UFUNCTION(BlueprintCallable, Category = "Metrics")
	int32 GetWealthLevel() const { return WealthLevel; }

	UFUNCTION(BlueprintCallable, Category = "Metrics")
	int32 GetDevelopmentLevel() const { return DevelopmentLevel; }

	UFUNCTION(BlueprintCallable, Category = "Metrics")
	int32 GetPopulation() const { return Population; }

	UFUNCTION(BlueprintCallable, Category = "Metrics")
	int32 GetTotalGDP() const;

	// Events
	UFUNCTION(BlueprintImplementableEvent, Category = "Economy Events")
	void OnMarketPricesUpdated();

	UFUNCTION(BlueprintImplementableEvent, Category = "Economy Events")
	void OnSupplyShortage(FName GoodID);

	UFUNCTION(BlueprintImplementableEvent, Category = "Economy Events")
	void OnDemandSurge(FName GoodID);

	UFUNCTION(BlueprintImplementableEvent, Category = "Economy Events")
	void OnTradeExecuted(FName GoodID, int32 Quantity, bool bWasPurchase);

protected:
	// Determine specialization from planet data
	void DetermineSpecialization(const FGeneratedPlanetData& PlanetData, int32 Seed);

	// Setup production based on specialization
	void SetupProduction(int32 Seed);

	// Setup consumption based on specialization
	void SetupConsumption(int32 Seed);

	// Initialize market prices
	void InitializeMarketPrices();

	// Calculate price based on supply/demand
	int32 CalculateDynamicPrice(FName GoodID, bool bForBuying) const;

	// Update production stockpiles
	void UpdateProduction(float DeltaTime);

	// Update consumption needs
	void UpdateConsumption(float DeltaTime);

	// Danger-wealth interaction
	void DangerModifiesWealth(int32 DangerRating, int32 Seed);

	// Seeded random helpers
	static uint32 HashSeed(int32 Seed);
	static float SeededRandom(int32 Seed);
	static int32 SeededRandomRange(int32 Seed, int32 Min, int32 Max);
};
