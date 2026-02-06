// UTradeRouteAnalyzer.h
// Trade opportunity detection and profitable route analysis
// Identifies arbitrage opportunities and calculates potential profits

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OdysseyEconomyTypes.h"
#include "UMarketDataComponent.h"
#include "UPriceFluctuationSystem.h"
#include "UTradeRouteAnalyzer.generated.h"

/**
 * Route analysis result with detailed profit breakdown
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FRouteAnalysisResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	bool bIsViable;

	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FString RouteName;

	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FMarketId SourceMarket;

	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FMarketId DestinationMarket;

	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	TMap<EResourceType, FTradeOpportunity> Opportunities;

	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	EResourceType BestResource;

	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 MaxPotentialProfit;

	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	float OverallRouteScore;

	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	double AnalysisTime;

	FRouteAnalysisResult()
		: bIsViable(false)
		, BestResource(EResourceType::None)
		, MaxPotentialProfit(0)
		, OverallRouteScore(0.0f)
		, AnalysisTime(0.0)
	{
	}
};

/**
 * Market pair for route analysis
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FMarketPair
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market Pair")
	FMarketId MarketA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market Pair")
	FMarketId MarketB;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market Pair")
	float Distance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market Pair")
	float TravelTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market Pair")
	ETradeRouteRisk Risk;

	FMarketPair()
		: Distance(0.0f)
		, TravelTime(0.0f)
		, Risk(ETradeRouteRisk::Moderate)
	{
	}

	FMarketPair(const FMarketId& A, const FMarketId& B, float Dist, float Time, ETradeRouteRisk RiskLevel)
		: MarketA(A)
		, MarketB(B)
		, Distance(Dist)
		, TravelTime(Time)
		, Risk(RiskLevel)
	{
	}
};

/**
 * UTradeRouteAnalyzer - Trade Opportunity Detection System
 *
 * Responsibilities:
 * - Identify profitable trade routes between markets
 * - Calculate potential profits considering all costs
 * - Track and rank opportunities as markets change
 * - Provide recommendations to players
 * - Detect arbitrage opportunities
 */
UCLASS(ClassGroup=(Economy), meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UTradeRouteAnalyzer : public UActorComponent
{
	GENERATED_BODY()

public:
	UTradeRouteAnalyzer();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// INITIALIZATION
	// ============================================================================

	/**
	 * Register a market for trade route analysis
	 */
	UFUNCTION(BlueprintCallable, Category = "Initialization")
	void RegisterMarket(const FMarketId& MarketId, UMarketDataComponent* MarketData, UPriceFluctuationSystem* PriceSystem);

	/**
	 * Unregister a market
	 */
	UFUNCTION(BlueprintCallable, Category = "Initialization")
	void UnregisterMarket(const FMarketId& MarketId);

	/**
	 * Define a trade route between markets
	 */
	UFUNCTION(BlueprintCallable, Category = "Initialization")
	void DefineTradeRoute(const FMarketId& Source, const FMarketId& Destination, float Distance, float TravelTime, ETradeRouteRisk Risk);

	/**
	 * Auto-generate routes between all registered markets
	 */
	UFUNCTION(BlueprintCallable, Category = "Initialization")
	void GenerateAllRoutes(float BaseSpeedUnitsPerHour);

	// ============================================================================
	// ROUTE ANALYSIS
	// ============================================================================

	/**
	 * Analyze a specific trade route
	 */
	UFUNCTION(BlueprintCallable, Category = "Analysis")
	FRouteAnalysisResult AnalyzeRoute(const FMarketId& Source, const FMarketId& Destination) const;

	/**
	 * Analyze all routes from a specific market
	 */
	UFUNCTION(BlueprintCallable, Category = "Analysis")
	TArray<FRouteAnalysisResult> AnalyzeRoutesFrom(const FMarketId& Source) const;

	/**
	 * Analyze all routes to a specific market
	 */
	UFUNCTION(BlueprintCallable, Category = "Analysis")
	TArray<FRouteAnalysisResult> AnalyzeRoutesTo(const FMarketId& Destination) const;

	/**
	 * Full analysis of all routes
	 */
	UFUNCTION(BlueprintCallable, Category = "Analysis")
	void AnalyzeAllRoutes();

	/**
	 * Get trade opportunity for specific resource on route
	 */
	UFUNCTION(BlueprintCallable, Category = "Analysis")
	FTradeOpportunity GetOpportunity(const FMarketId& Source, const FMarketId& Destination, EResourceType Resource) const;

	// ============================================================================
	// OPPORTUNITY DISCOVERY
	// ============================================================================

	/**
	 * Get top trade opportunities sorted by score
	 */
	UFUNCTION(BlueprintCallable, Category = "Opportunities")
	TArray<FTradeOpportunity> GetTopOpportunities(int32 MaxCount = 10) const;

	/**
	 * Get opportunities from a specific source market
	 */
	UFUNCTION(BlueprintCallable, Category = "Opportunities")
	TArray<FTradeOpportunity> GetOpportunitiesFrom(const FMarketId& Source, int32 MaxCount = 10) const;

	/**
	 * Get opportunities for a specific resource
	 */
	UFUNCTION(BlueprintCallable, Category = "Opportunities")
	TArray<FTradeOpportunity> GetOpportunitiesForResource(EResourceType Resource, int32 MaxCount = 10) const;

	/**
	 * Find arbitrage opportunities (same resource, price discrepancy)
	 */
	UFUNCTION(BlueprintCallable, Category = "Opportunities")
	TArray<FTradeOpportunity> FindArbitrageOpportunities(float MinProfitMargin = 0.15f) const;

	/**
	 * Get opportunities meeting criteria
	 */
	UFUNCTION(BlueprintCallable, Category = "Opportunities")
	TArray<FTradeOpportunity> GetFilteredOpportunities(
		float MinProfitMargin,
		ETradeRouteRisk MaxRisk,
		float MaxTravelTime,
		int32 MaxCount = 10) const;

	// ============================================================================
	// PROFIT CALCULATION
	// ============================================================================

	/**
	 * Calculate net profit for a trade
	 */
	UFUNCTION(BlueprintCallable, Category = "Profit")
	int32 CalculateNetProfit(
		const FMarketId& Source,
		const FMarketId& Destination,
		EResourceType Resource,
		int32 Quantity) const;

	/**
	 * Calculate profit after all costs (taxes, fees, fuel)
	 */
	UFUNCTION(BlueprintCallable, Category = "Profit")
	int32 CalculateNetProfitAfterCosts(
		const FMarketId& Source,
		const FMarketId& Destination,
		EResourceType Resource,
		int32 Quantity,
		float FuelCostPerUnit) const;

	/**
	 * Calculate optimal trade quantity based on available capital
	 */
	UFUNCTION(BlueprintCallable, Category = "Profit")
	int32 CalculateOptimalQuantity(
		const FMarketId& Source,
		const FMarketId& Destination,
		EResourceType Resource,
		int32 AvailableCapital,
		int32 CargoCapacity) const;

	/**
	 * Calculate round-trip profit potential
	 */
	UFUNCTION(BlueprintCallable, Category = "Profit")
	int32 CalculateRoundTripProfit(
		const FMarketId& MarketA,
		const FMarketId& MarketB,
		int32 Capital,
		int32 CargoCapacity) const;

	// ============================================================================
	// ROUTE INFORMATION
	// ============================================================================

	/**
	 * Get all defined routes
	 */
	UFUNCTION(BlueprintCallable, Category = "Routes")
	TArray<FTradeRoute> GetAllRoutes() const;

	/**
	 * Get routes from a market
	 */
	UFUNCTION(BlueprintCallable, Category = "Routes")
	TArray<FTradeRoute> GetRoutesFrom(const FMarketId& Source) const;

	/**
	 * Get route between two markets
	 */
	UFUNCTION(BlueprintCallable, Category = "Routes")
	FTradeRoute GetRoute(const FMarketId& Source, const FMarketId& Destination) const;

	/**
	 * Check if route exists
	 */
	UFUNCTION(BlueprintCallable, Category = "Routes")
	bool HasRoute(const FMarketId& Source, const FMarketId& Destination) const;

	/**
	 * Get safest route to destination
	 */
	UFUNCTION(BlueprintCallable, Category = "Routes")
	FTradeRoute GetSafestRoute(const FMarketId& Source, const FMarketId& Destination) const;

	/**
	 * Get fastest route to destination
	 */
	UFUNCTION(BlueprintCallable, Category = "Routes")
	FTradeRoute GetFastestRoute(const FMarketId& Source, const FMarketId& Destination) const;

	// ============================================================================
	// MARKET COMPARISON
	// ============================================================================

	/**
	 * Compare prices between two markets
	 */
	UFUNCTION(BlueprintCallable, Category = "Comparison")
	TMap<EResourceType, float> ComparePrices(const FMarketId& MarketA, const FMarketId& MarketB) const;

	/**
	 * Find best market to buy a resource
	 */
	UFUNCTION(BlueprintCallable, Category = "Comparison")
	FMarketId FindBestBuyMarket(EResourceType Resource) const;

	/**
	 * Find best market to sell a resource
	 */
	UFUNCTION(BlueprintCallable, Category = "Comparison")
	FMarketId FindBestSellMarket(EResourceType Resource) const;

	/**
	 * Get price differential between markets for resource
	 */
	UFUNCTION(BlueprintCallable, Category = "Comparison")
	float GetPriceDifferential(const FMarketId& Source, const FMarketId& Destination, EResourceType Resource) const;

	// ============================================================================
	// CONFIGURATION
	// ============================================================================

	/**
	 * Set minimum profit margin for opportunity detection
	 */
	UFUNCTION(BlueprintCallable, Category = "Config")
	void SetMinProfitMargin(float Margin) { MinProfitMarginThreshold = Margin; }

	/**
	 * Set analysis update interval
	 */
	UFUNCTION(BlueprintCallable, Category = "Config")
	void SetAnalysisInterval(float Seconds) { AnalysisIntervalSeconds = Seconds; }

	/**
	 * Set max opportunities to track
	 */
	UFUNCTION(BlueprintCallable, Category = "Config")
	void SetMaxOpportunities(int32 Max) { MaxTrackedOpportunities = Max; }

	// ============================================================================
	// EVENTS
	// ============================================================================

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnTradeOpportunityFound OnOpportunityFound;

protected:
	// Registered markets
	UPROPERTY()
	TMap<FName, UMarketDataComponent*> MarketDataComponents;

	UPROPERTY()
	TMap<FName, UPriceFluctuationSystem*> PriceSystems;

	// Market locations for distance calculation
	TMap<FName, FVector> MarketLocations;

	// Defined trade routes
	UPROPERTY(BlueprintReadOnly, Category = "Routes")
	TArray<FTradeRoute> TradeRoutes;

	// Current opportunities (cached)
	UPROPERTY(BlueprintReadOnly, Category = "Opportunities")
	TArray<FTradeOpportunity> CurrentOpportunities;

	// Configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float MinProfitMarginThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float AnalysisIntervalSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int32 MaxTrackedOpportunities;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float OpportunityExpirationSeconds;

	// Timing
	float TimeSinceLastAnalysis;

private:
	void UpdateOpportunities();
	FTradeOpportunity CalculateOpportunity(
		const FTradeRoute& Route,
		EResourceType Resource,
		int32 SourceBuyPrice,
		int32 DestSellPrice,
		int32 AvailableSupply) const;
	void SortOpportunities();
	void PruneExpiredOpportunities();
	FString GetMarketKey(const FMarketId& MarketId) const;
	int32 GetBuyPriceAt(const FMarketId& Market, EResourceType Resource) const;
	int32 GetSellPriceAt(const FMarketId& Market, EResourceType Resource) const;
	int32 GetSupplyAt(const FMarketId& Market, EResourceType Resource) const;
};
