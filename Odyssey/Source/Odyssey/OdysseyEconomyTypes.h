// OdysseyEconomyTypes.h
// Core type definitions for the Dynamic Economy Simulation System
// Provides foundation for supply/demand tracking, price calculations, and market events

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "OdysseyInventoryComponent.h"
#include "OdysseyEconomyTypes.generated.h"

// ============================================================================
// ECONOMY ENUMERATIONS
// ============================================================================

/**
 * Market volatility levels affecting price fluctuation intensity
 */
UENUM(BlueprintType)
enum class EMarketVolatility : uint8
{
	Stable = 0,         // ±5% price changes
	Low = 1,            // ±10% price changes
	Moderate = 2,       // ±20% price changes
	High = 3,           // ±35% price changes
	Extreme = 4         // ±50% price changes
};

/**
 * Economic event categories that can disrupt markets
 */
UENUM(BlueprintType)
enum class EEconomicEventType : uint8
{
	None = 0,
	
	// Supply-side events
	ResourceDiscovery,      // New deposit found, increases supply
	ResourceDepletion,      // Deposit exhausted, decreases supply
	ProductionBoost,        // Efficiency improvement
	ProductionDisruption,   // Equipment failure, strike, etc.
	
	// Demand-side events
	DemandSurge,            // Increased demand for resource
	DemandCollapse,         // Decreased demand
	TechnologyBreakthrough, // New crafting recipes unlock
	TechnologyObsolete,     // Resource becomes less valuable
	
	// Market-wide events
	TradeRouteOpened,       // New trading opportunity
	TradeRouteBlocked,      // Pirates, hazards block route
	MarketCrash,            // Broad price decline
	MarketBoom,             // Broad price increase
	
	// Combat/conflict events
	WarDeclared,            // Conflict affects economy
	PeaceTreaty,            // End of conflict
	PirateActivity,         // Trade disruption
	
	// Environmental events
	AsteroidStorm,          // Mining disruption
	SolarFlare,             // General disruption
	
	// Custom events
	CustomEvent = 200
};

/**
 * Economic event severity affecting impact magnitude
 */
UENUM(BlueprintType)
enum class EEconomicEventSeverity : uint8
{
	Minor = 0,      // 5-10% impact
	Moderate = 1,   // 10-25% impact
	Major = 2,      // 25-50% impact
	Critical = 3,   // 50-100% impact
	Catastrophic = 4 // 100%+ impact
};

/**
 * Market trend directions
 */
UENUM(BlueprintType)
enum class EMarketTrend : uint8
{
	StrongBear = 0,     // Rapidly falling prices
	Bear = 1,           // Falling prices
	Neutral = 2,        // Stable prices
	Bull = 3,           // Rising prices
	StrongBull = 4      // Rapidly rising prices
};

/**
 * Trade route risk levels
 */
UENUM(BlueprintType)
enum class ETradeRouteRisk : uint8
{
	Safe = 0,
	Low = 1,
	Moderate = 2,
	High = 3,
	Dangerous = 4
};

/**
 * Market location types
 */
UENUM(BlueprintType)
enum class EMarketLocationType : uint8
{
	Station = 0,
	Outpost = 1,
	Colony = 2,
	Hub = 3,
	BlackMarket = 4
};

// ============================================================================
// PRICE AND SUPPLY/DEMAND STRUCTURES
// ============================================================================

/**
 * Historical price point for trend analysis
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FPriceHistoryEntry
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "Price History")
	double Timestamp;
	
	UPROPERTY(BlueprintReadOnly, Category = "Price History")
	int32 Price;
	
	UPROPERTY(BlueprintReadOnly, Category = "Price History")
	int32 Volume;
	
	UPROPERTY(BlueprintReadOnly, Category = "Price History")
	float SupplyDemandRatio;
	
	FPriceHistoryEntry()
		: Timestamp(0.0)
		, Price(0)
		, Volume(0)
		, SupplyDemandRatio(1.0f)
	{
	}
	
	FPriceHistoryEntry(double InTimestamp, int32 InPrice, int32 InVolume, float InRatio)
		: Timestamp(InTimestamp)
		, Price(InPrice)
		, Volume(InVolume)
		, SupplyDemandRatio(InRatio)
	{
	}
};

/**
 * Supply/demand data for a single resource at a market
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FResourceSupplyDemand
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Supply/Demand")
	EResourceType ResourceType;
	
	// Supply metrics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Supply")
	int32 CurrentSupply;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Supply")
	int32 MaxSupply;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Supply")
	float SupplyRate;  // Units per game hour produced
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Supply")
	float SupplyModifier;  // Event-based modifier
	
	// Demand metrics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demand")
	int32 BaseDemand;  // Baseline demand per hour
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demand")
	float DemandRate;  // Current consumption rate
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demand")
	float DemandModifier;  // Event-based modifier
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demand")
	float DemandElasticity;  // How sensitive demand is to price
	
	// Calculated values
	UPROPERTY(BlueprintReadOnly, Category = "Calculated")
	float SupplyDemandRatio;
	
	UPROPERTY(BlueprintReadOnly, Category = "Calculated")
	float ScarcityIndex;  // 0 = abundant, 1 = scarce
	
	FResourceSupplyDemand()
		: ResourceType(EResourceType::None)
		, CurrentSupply(100)
		, MaxSupply(1000)
		, SupplyRate(10.0f)
		, SupplyModifier(1.0f)
		, BaseDemand(10)
		, DemandRate(10.0f)
		, DemandModifier(1.0f)
		, DemandElasticity(1.0f)
		, SupplyDemandRatio(1.0f)
		, ScarcityIndex(0.0f)
	{
	}
	
	void RecalculateMetrics()
	{
		float EffectiveDemand = DemandRate * DemandModifier;
		float EffectiveSupply = FMath::Max(1.0f, static_cast<float>(CurrentSupply));
		
		SupplyDemandRatio = EffectiveSupply / FMath::Max(1.0f, EffectiveDemand * 10.0f);
		ScarcityIndex = 1.0f - FMath::Clamp(static_cast<float>(CurrentSupply) / static_cast<float>(MaxSupply), 0.0f, 1.0f);
	}
};

/**
 * Complete market price state for a resource
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FDynamicMarketPrice
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market Price")
	EResourceType ResourceType;
	
	// Base pricing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Price")
	int32 BasePrice;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Price")
	int32 MinPrice;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Price")
	int32 MaxPrice;
	
	// Current calculated prices
	UPROPERTY(BlueprintReadOnly, Category = "Current Price")
	int32 CurrentBuyPrice;
	
	UPROPERTY(BlueprintReadOnly, Category = "Current Price")
	int32 CurrentSellPrice;
	
	UPROPERTY(BlueprintReadOnly, Category = "Current Price")
	float PriceMultiplier;
	
	// Volatility
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volatility")
	EMarketVolatility Volatility;
	
	UPROPERTY(BlueprintReadOnly, Category = "Volatility")
	float CurrentVolatilityFactor;
	
	// Trend analysis
	UPROPERTY(BlueprintReadOnly, Category = "Trend")
	EMarketTrend CurrentTrend;
	
	UPROPERTY(BlueprintReadOnly, Category = "Trend")
	float TrendStrength;  // 0-1 indicating how strong the trend is
	
	UPROPERTY(BlueprintReadOnly, Category = "Trend")
	float TrendMomentum;  // Rate of change
	
	// Spread settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread")
	float BuySpreadPercent;  // % markup for buying
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread")
	float SellSpreadPercent;  // % markdown for selling
	
	// History
	UPROPERTY(BlueprintReadOnly, Category = "History")
	TArray<FPriceHistoryEntry> PriceHistory;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History")
	int32 MaxHistoryEntries;
	
	FDynamicMarketPrice()
		: ResourceType(EResourceType::None)
		, BasePrice(10)
		, MinPrice(1)
		, MaxPrice(1000)
		, CurrentBuyPrice(10)
		, CurrentSellPrice(8)
		, PriceMultiplier(1.0f)
		, Volatility(EMarketVolatility::Moderate)
		, CurrentVolatilityFactor(0.0f)
		, CurrentTrend(EMarketTrend::Neutral)
		, TrendStrength(0.0f)
		, TrendMomentum(0.0f)
		, BuySpreadPercent(0.1f)
		, SellSpreadPercent(0.1f)
		, MaxHistoryEntries(100)
	{
	}
	
	void AddHistoryEntry(int32 Price, int32 Volume, float SupplyDemandRatio)
	{
		FPriceHistoryEntry Entry(FPlatformTime::Seconds(), Price, Volume, SupplyDemandRatio);
		PriceHistory.Add(Entry);
		
		// Maintain max history size
		while (PriceHistory.Num() > MaxHistoryEntries)
		{
			PriceHistory.RemoveAt(0);
		}
	}
	
	float CalculateAveragePrice(int32 NumEntries) const
	{
		if (PriceHistory.Num() == 0) return static_cast<float>(BasePrice);
		
		int32 Count = FMath::Min(NumEntries, PriceHistory.Num());
		float Sum = 0.0f;
		
		for (int32 i = PriceHistory.Num() - Count; i < PriceHistory.Num(); ++i)
		{
			Sum += PriceHistory[i].Price;
		}
		
		return Sum / static_cast<float>(Count);
	}
};

// ============================================================================
// MARKET AND TRADE ROUTE STRUCTURES
// ============================================================================

/**
 * Unique market identifier
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FMarketId
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market ID")
	FName MarketName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market ID")
	int32 RegionId;
	
	FMarketId()
		: MarketName(NAME_None)
		, RegionId(0)
	{
	}
	
	FMarketId(FName InName, int32 InRegion)
		: MarketName(InName)
		, RegionId(InRegion)
	{
	}
	
	bool operator==(const FMarketId& Other) const
	{
		return MarketName == Other.MarketName && RegionId == Other.RegionId;
	}
	
	friend uint32 GetTypeHash(const FMarketId& Id)
	{
		return HashCombine(GetTypeHash(Id.MarketName), GetTypeHash(Id.RegionId));
	}
	
	FString ToString() const
	{
		return FString::Printf(TEXT("%s_R%d"), *MarketName.ToString(), RegionId);
	}
};

/**
 * Complete market data for a trading location
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FMarketData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	FMarketId MarketId;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	FString DisplayName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	EMarketLocationType LocationType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	FVector WorldLocation;
	
	// Supply/demand per resource
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Supply/Demand")
	TMap<EResourceType, FResourceSupplyDemand> SupplyDemandData;
	
	// Prices per resource
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prices")
	TMap<EResourceType, FDynamicMarketPrice> ResourcePrices;
	
	// Market characteristics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Characteristics")
	float TaxRate;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Characteristics")
	float TransactionFeePercent;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Characteristics")
	TArray<EResourceType> SpecializedResources;  // Resources this market focuses on
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Characteristics")
	float SpecializationBonus;  // Price bonus for specialized resources
	
	// Active economic events affecting this market
	UPROPERTY(BlueprintReadOnly, Category = "Events")
	TArray<int32> ActiveEventIds;
	
	// Statistics
	UPROPERTY(BlueprintReadOnly, Category = "Statistics")
	int64 TotalTradeVolume;
	
	UPROPERTY(BlueprintReadOnly, Category = "Statistics")
	int32 TradeCountToday;
	
	UPROPERTY(BlueprintReadOnly, Category = "Statistics")
	double LastUpdateTime;
	
	FMarketData()
		: DisplayName(TEXT("Unknown Market"))
		, LocationType(EMarketLocationType::Station)
		, WorldLocation(FVector::ZeroVector)
		, TaxRate(0.05f)
		, TransactionFeePercent(0.02f)
		, SpecializationBonus(1.15f)
		, TotalTradeVolume(0)
		, TradeCountToday(0)
		, LastUpdateTime(0.0)
	{
	}
	
	bool IsSpecializedIn(EResourceType Resource) const
	{
		return SpecializedResources.Contains(Resource);
	}
};

/**
 * Trade route between two markets
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FTradeRoute
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Route")
	FMarketId SourceMarket;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Route")
	FMarketId DestinationMarket;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Route")
	float Distance;  // In game units
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Route")
	float TravelTime;  // In game hours
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Route")
	ETradeRouteRisk RiskLevel;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Route")
	float FuelCost;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Route")
	bool bIsActive;
	
	// Opportunity data
	UPROPERTY(BlueprintReadOnly, Category = "Opportunity")
	TMap<EResourceType, float> ProfitMargins;  // Per resource profit %
	
	UPROPERTY(BlueprintReadOnly, Category = "Opportunity")
	EResourceType BestResource;
	
	UPROPERTY(BlueprintReadOnly, Category = "Opportunity")
	float BestProfitMargin;
	
	UPROPERTY(BlueprintReadOnly, Category = "Opportunity")
	int32 EstimatedProfitPerTrip;
	
	UPROPERTY(BlueprintReadOnly, Category = "Opportunity")
	double LastAnalysisTime;
	
	FTradeRoute()
		: Distance(0.0f)
		, TravelTime(0.0f)
		, RiskLevel(ETradeRouteRisk::Moderate)
		, FuelCost(0.0f)
		, bIsActive(true)
		, BestResource(EResourceType::None)
		, BestProfitMargin(0.0f)
		, EstimatedProfitPerTrip(0)
		, LastAnalysisTime(0.0)
	{
	}
	
	FString GetRouteId() const
	{
		return FString::Printf(TEXT("%s_to_%s"), 
			*SourceMarket.ToString(), 
			*DestinationMarket.ToString());
	}
};

/**
 * Trade opportunity recommendation for players
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FTradeOpportunity
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "Opportunity")
	FTradeRoute Route;
	
	UPROPERTY(BlueprintReadOnly, Category = "Opportunity")
	EResourceType Resource;
	
	UPROPERTY(BlueprintReadOnly, Category = "Opportunity")
	int32 BuyPrice;
	
	UPROPERTY(BlueprintReadOnly, Category = "Opportunity")
	int32 SellPrice;
	
	UPROPERTY(BlueprintReadOnly, Category = "Opportunity")
	int32 ProfitPerUnit;
	
	UPROPERTY(BlueprintReadOnly, Category = "Opportunity")
	float ProfitMarginPercent;
	
	UPROPERTY(BlueprintReadOnly, Category = "Opportunity")
	int32 AvailableQuantity;
	
	UPROPERTY(BlueprintReadOnly, Category = "Opportunity")
	int32 MaxProfitPotential;
	
	UPROPERTY(BlueprintReadOnly, Category = "Opportunity")
	float RiskAdjustedReturn;  // Profit adjusted for route risk
	
	UPROPERTY(BlueprintReadOnly, Category = "Opportunity")
	float TimeEfficiency;  // Profit per hour of travel
	
	UPROPERTY(BlueprintReadOnly, Category = "Opportunity")
	float OpportunityScore;  // Combined metric for ranking
	
	UPROPERTY(BlueprintReadOnly, Category = "Opportunity")
	double ExpirationTime;  // When this opportunity may no longer be valid
	
	FTradeOpportunity()
		: Resource(EResourceType::None)
		, BuyPrice(0)
		, SellPrice(0)
		, ProfitPerUnit(0)
		, ProfitMarginPercent(0.0f)
		, AvailableQuantity(0)
		, MaxProfitPotential(0)
		, RiskAdjustedReturn(0.0f)
		, TimeEfficiency(0.0f)
		, OpportunityScore(0.0f)
		, ExpirationTime(0.0)
	{
	}
	
	void CalculateMetrics()
	{
		ProfitPerUnit = SellPrice - BuyPrice;
		ProfitMarginPercent = BuyPrice > 0 ? (static_cast<float>(ProfitPerUnit) / static_cast<float>(BuyPrice)) * 100.0f : 0.0f;
		MaxProfitPotential = ProfitPerUnit * AvailableQuantity;
		
		// Risk adjustment based on route risk
		float RiskMultiplier = 1.0f;
		switch (Route.RiskLevel)
		{
			case ETradeRouteRisk::Safe: RiskMultiplier = 1.0f; break;
			case ETradeRouteRisk::Low: RiskMultiplier = 0.95f; break;
			case ETradeRouteRisk::Moderate: RiskMultiplier = 0.85f; break;
			case ETradeRouteRisk::High: RiskMultiplier = 0.70f; break;
			case ETradeRouteRisk::Dangerous: RiskMultiplier = 0.50f; break;
		}
		
		RiskAdjustedReturn = static_cast<float>(MaxProfitPotential) * RiskMultiplier;
		TimeEfficiency = Route.TravelTime > 0.0f ? RiskAdjustedReturn / Route.TravelTime : 0.0f;
		
		// Composite score weighing various factors
		OpportunityScore = (ProfitMarginPercent * 0.3f) + 
		                   (FMath::Min(TimeEfficiency / 100.0f, 1.0f) * 0.4f) + 
		                   (RiskMultiplier * 0.3f);
	}
};

// ============================================================================
// ECONOMIC EVENT STRUCTURES
// ============================================================================

/**
 * Economic event affecting markets
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FEconomicEvent
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	int32 EventId;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	EEconomicEventType EventType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	EEconomicEventSeverity Severity;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	FString EventName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	FString Description;
	
	// Timing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing")
	double StartTime;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing")
	double Duration;  // In game seconds
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing")
	double EndTime;
	
	UPROPERTY(BlueprintReadOnly, Category = "Timing")
	bool bIsActive;
	
	// Impact
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
	TArray<FMarketId> AffectedMarkets;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
	TArray<EResourceType> AffectedResources;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
	float SupplyModifier;  // Multiplier for supply
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
	float DemandModifier;  // Multiplier for demand
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
	float PriceModifier;  // Direct price multiplier
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
	float VolatilityIncrease;  // Additional volatility
	
	// Narrative
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative")
	FString NewsHeadline;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative")
	FString NewsBody;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative")
	bool bShowNotification;
	
	FEconomicEvent()
		: EventId(0)
		, EventType(EEconomicEventType::None)
		, Severity(EEconomicEventSeverity::Minor)
		, EventName(TEXT("Unknown Event"))
		, Description(TEXT(""))
		, StartTime(0.0)
		, Duration(0.0)
		, EndTime(0.0)
		, bIsActive(false)
		, SupplyModifier(1.0f)
		, DemandModifier(1.0f)
		, PriceModifier(1.0f)
		, VolatilityIncrease(0.0f)
		, bShowNotification(true)
	{
	}
	
	void Activate(double CurrentTime)
	{
		StartTime = CurrentTime;
		EndTime = CurrentTime + Duration;
		bIsActive = true;
	}
	
	bool ShouldExpire(double CurrentTime) const
	{
		return bIsActive && CurrentTime >= EndTime;
	}
	
	float GetRemainingDuration(double CurrentTime) const
	{
		return bIsActive ? FMath::Max(0.0, EndTime - CurrentTime) : 0.0f;
	}
	
	float GetProgress(double CurrentTime) const
	{
		if (!bIsActive || Duration <= 0.0) return 0.0f;
		return FMath::Clamp((CurrentTime - StartTime) / Duration, 0.0, 1.0);
	}
};

/**
 * Template for generating economic events
 */
USTRUCT(BlueprintType, meta = (DataTable))
struct ODYSSEY_API FEconomicEventTemplate : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Template")
	EEconomicEventType EventType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Template")
	FString EventNameTemplate;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Template")
	FString DescriptionTemplate;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Template")
	TArray<EResourceType> PossibleResources;
	
	// Impact ranges
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
	float MinSupplyModifier;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
	float MaxSupplyModifier;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
	float MinDemandModifier;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
	float MaxDemandModifier;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
	float MinDuration;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Impact")
	float MaxDuration;
	
	// Probability
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Probability")
	float BaseSpawnChance;  // Per hour
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Probability")
	float MinCooldown;  // Minimum time between this event type
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative")
	TArray<FString> NewsHeadlineVariants;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative")
	TArray<FString> NewsBodyVariants;
	
	FEconomicEventTemplate()
		: EventType(EEconomicEventType::None)
		, MinSupplyModifier(0.9f)
		, MaxSupplyModifier(1.1f)
		, MinDemandModifier(0.9f)
		, MaxDemandModifier(1.1f)
		, MinDuration(60.0f)
		, MaxDuration(300.0f)
		, BaseSpawnChance(0.1f)
		, MinCooldown(60.0f)
	{
	}
};

// ============================================================================
// ECONOMY SYSTEM CONFIGURATION
// ============================================================================

/**
 * Configuration for the economy simulation
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FEconomyConfiguration
{
	GENERATED_BODY()
	
	// Simulation settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	float TickIntervalSeconds;  // How often to update economy
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	float PriceUpdateIntervalSeconds;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	float TradeRouteAnalysisIntervalSeconds;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	float EventCheckIntervalSeconds;
	
	// Price calculation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing")
	float BaseVolatilityPercent;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing")
	float SupplyDemandPriceInfluence;  // 0-1, how much S/D affects price
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing")
	float PriceSmoothingFactor;  // How fast prices change (0=instant, 1=slow)
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing")
	float MinPriceChangePercent;  // Minimum change to register
	
	// Events
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Events")
	int32 MaxActiveEvents;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Events")
	float EventSpawnRateMultiplier;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Events")
	bool bAllowCatastrophicEvents;
	
	// Trade routes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade Routes")
	int32 MaxTradeOpportunities;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade Routes")
	float MinProfitMarginForOpportunity;

	// Ripple effect settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ripple Effects")
	int32 MaxActiveRipples;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ripple Effects")
	float RippleMinMagnitudeThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ripple Effects")
	int32 RippleMaxPropagationDepth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ripple Effects")
	float RippleDefaultDampening;
	
	// Mobile optimization
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	int32 MaxMarketsToUpdatePerTick;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	int32 MaxPriceHistoryEntries;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	bool bEnableDetailedLogging;
	
	FEconomyConfiguration()
		: TickIntervalSeconds(1.0f)
		, PriceUpdateIntervalSeconds(5.0f)
		, TradeRouteAnalysisIntervalSeconds(10.0f)
		, EventCheckIntervalSeconds(30.0f)
		, BaseVolatilityPercent(0.05f)
		, SupplyDemandPriceInfluence(0.7f)
		, PriceSmoothingFactor(0.8f)
		, MinPriceChangePercent(0.01f)
		, MaxActiveEvents(5)
		, EventSpawnRateMultiplier(1.0f)
		, bAllowCatastrophicEvents(true)
		, MaxTradeOpportunities(20)
		, MinProfitMarginForOpportunity(0.1f)
		, MaxActiveRipples(10)
		, RippleMinMagnitudeThreshold(0.02f)
		, RippleMaxPropagationDepth(4)
		, RippleDefaultDampening(0.3f)
		, MaxMarketsToUpdatePerTick(5)
		, MaxPriceHistoryEntries(100)
		, bEnableDetailedLogging(false)
	{
	}
};

// ============================================================================
// ECONOMY EVENT DELEGATES
// ============================================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMarketPriceChanged, EResourceType, Resource, const FDynamicMarketPrice&, NewPrice);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEconomicEventStarted, const FEconomicEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEconomicEventEnded, const FEconomicEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTradeOpportunityFound, const FTradeOpportunity&, Opportunity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMarketTradeCompleted, const FMarketId&, Market, int32, TotalValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSupplyDemandChanged, EResourceType, Resource, const FResourceSupplyDemand&, NewData);

// ============================================================================
// RIPPLE EFFECT TYPES (Task #5 - Economy System Extension)
// ============================================================================

/**
 * Ripple propagation type for economic chain reactions
 */
UENUM(BlueprintType)
enum class ERippleType : uint8
{
	SupplyShock = 0,     // Supply change propagates to connected markets
	DemandShock,         // Demand change propagates
	PriceShock,          // Price shock propagates with dampening
	TradeDisruption,     // Route blockage creates shortages in dependent markets
	CombatZone,          // Combat activity raises risk and reduces trade
	CraftingDemand       // Crafting surge increases demand for ingredients
};

/**
 * Describes a single economic ripple propagating through the market network.
 * Ripples travel from a source market outward through trade routes, applying
 * dampened versions of the original economic effect at each hop.
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FEconomicRipple
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Ripple")
	int32 RippleId;

	UPROPERTY(BlueprintReadOnly, Category = "Ripple")
	ERippleType RippleType;

	UPROPERTY(BlueprintReadOnly, Category = "Ripple")
	FMarketId OriginMarket;

	UPROPERTY(BlueprintReadOnly, Category = "Ripple")
	TArray<EResourceType> AffectedResources;

	/** Base magnitude of the effect (negative = decrease, positive = increase) */
	UPROPERTY(BlueprintReadOnly, Category = "Ripple")
	float BaseMagnitude;

	/** How much magnitude is lost per hop (0.0-1.0). 0.3 = 30% lost per hop. */
	UPROPERTY(BlueprintReadOnly, Category = "Ripple")
	float DampeningFactor;

	UPROPERTY(BlueprintReadOnly, Category = "Ripple")
	int32 CurrentDepth;

	UPROPERTY(BlueprintReadOnly, Category = "Ripple")
	int32 MaxDepth;

	UPROPERTY(BlueprintReadOnly, Category = "Ripple")
	float PropagationSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Ripple")
	float AccumulatedTime;

	UPROPERTY(BlueprintReadOnly, Category = "Ripple")
	TArray<FMarketId> VisitedMarkets;

	UPROPERTY(BlueprintReadOnly, Category = "Ripple")
	TArray<FMarketId> NextWaveMarkets;

	UPROPERTY(BlueprintReadOnly, Category = "Ripple")
	double CreationTime;

	UPROPERTY(BlueprintReadOnly, Category = "Ripple")
	int32 SourceEventId;

	UPROPERTY(BlueprintReadOnly, Category = "Ripple")
	bool bIsActive;

	FEconomicRipple()
		: RippleId(0), RippleType(ERippleType::SupplyShock)
		, BaseMagnitude(0.5f), DampeningFactor(0.3f)
		, CurrentDepth(0), MaxDepth(4), PropagationSpeed(1.0f), AccumulatedTime(0.0f)
		, CreationTime(0.0), SourceEventId(-1), bIsActive(true)
	{
	}

	float GetCurrentMagnitude() const
	{
		return BaseMagnitude * FMath::Pow(1.0f - DampeningFactor, static_cast<float>(CurrentDepth));
	}

	bool HasDissipated(float MinMagnitude = 0.01f) const
	{
		return FMath::Abs(GetCurrentMagnitude()) < MinMagnitude || CurrentDepth >= MaxDepth;
	}
};

// ============================================================================
// ECONOMY SAVE DATA TYPES (Task #5 - Serialization)
// ============================================================================

/**
 * Serializable snapshot of a single market's state for save/load
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FMarketSaveData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	FMarketId MarketId;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	FString DisplayName;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	EMarketLocationType LocationType;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	FVector WorldLocation;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	TMap<EResourceType, FResourceSupplyDemand> SupplyDemandData;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	TMap<EResourceType, FDynamicMarketPrice> ResourcePrices;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	TArray<EResourceType> SpecializedResources;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	float TaxRate;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	int64 TotalTradeVolume;

	FMarketSaveData()
		: LocationType(EMarketLocationType::Station)
		, WorldLocation(FVector::ZeroVector)
		, TaxRate(0.05f)
		, TotalTradeVolume(0)
	{
	}
};

/**
 * Full economy save snapshot
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FEconomySaveData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	int32 SaveVersion;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	double SaveTimestamp;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	TArray<FMarketSaveData> Markets;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	TArray<FTradeRoute> TradeRoutes;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	TArray<FEconomicEvent> ActiveEvents;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	TArray<FEconomicEvent> EventHistory;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	int64 TotalGlobalTradeVolume;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	int32 TotalEventsGenerated;

	FEconomySaveData()
		: SaveVersion(1), SaveTimestamp(0.0), TotalGlobalTradeVolume(0), TotalEventsGenerated(0)
	{
	}
};

// ============================================================================
// ADDITIONAL DELEGATES (Task #5 Extensions)
// ============================================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEconomicRipplePropagated, const FEconomicRipple&, Ripple);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEconomySaved, const FEconomySaveData&, SaveData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEconomyLoaded, const FEconomySaveData&, SaveData);
