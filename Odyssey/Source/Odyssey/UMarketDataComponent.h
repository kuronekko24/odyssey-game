// UMarketDataComponent.h
// Component for tracking supply/demand across all goods in real-time
// Provides historical price tracking and scarcity analysis

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OdysseyEconomyTypes.h"
#include "UMarketDataComponent.generated.h"

/**
 * UMarketDataComponent - Supply/Demand Tracking System
 * 
 * Responsibilities:
 * - Track goods inventory across markets
 * - Calculate supply/demand ratios
 * - Maintain historical price data
 * - Provide scarcity analysis
 * - Simulate resource consumption and production
 */
UCLASS(ClassGroup=(Economy), meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UMarketDataComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMarketDataComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// INITIALIZATION
	// ============================================================================

	/**
	 * Initialize market data with default values
	 */
	UFUNCTION(BlueprintCallable, Category = "Market Data")
	void InitializeMarketData(const FMarketId& MarketId, const FString& DisplayName);

	/**
	 * Initialize from data table
	 */
	UFUNCTION(BlueprintCallable, Category = "Market Data")
	void InitializeFromDataTable(UDataTable* MarketDataTable);

	// ============================================================================
	// SUPPLY MANAGEMENT
	// ============================================================================

	/**
	 * Get current supply for a resource
	 */
	UFUNCTION(BlueprintCallable, Category = "Supply")
	int32 GetCurrentSupply(EResourceType Resource) const;

	/**
	 * Get maximum supply capacity
	 */
	UFUNCTION(BlueprintCallable, Category = "Supply")
	int32 GetMaxSupply(EResourceType Resource) const;

	/**
	 * Get supply as percentage of max
	 */
	UFUNCTION(BlueprintCallable, Category = "Supply")
	float GetSupplyPercent(EResourceType Resource) const;

	/**
	 * Add supply (from production, player selling, etc.)
	 */
	UFUNCTION(BlueprintCallable, Category = "Supply")
	void AddSupply(EResourceType Resource, int32 Amount);

	/**
	 * Remove supply (from consumption, player buying, etc.)
	 */
	UFUNCTION(BlueprintCallable, Category = "Supply")
	bool RemoveSupply(EResourceType Resource, int32 Amount);

	/**
	 * Set supply rate (production per game hour)
	 */
	UFUNCTION(BlueprintCallable, Category = "Supply")
	void SetSupplyRate(EResourceType Resource, float Rate);

	/**
	 * Apply supply modifier from events
	 */
	UFUNCTION(BlueprintCallable, Category = "Supply")
	void SetSupplyModifier(EResourceType Resource, float Modifier);

	// ============================================================================
	// DEMAND MANAGEMENT
	// ============================================================================

	/**
	 * Get base demand for a resource
	 */
	UFUNCTION(BlueprintCallable, Category = "Demand")
	int32 GetBaseDemand(EResourceType Resource) const;

	/**
	 * Get current demand rate
	 */
	UFUNCTION(BlueprintCallable, Category = "Demand")
	float GetDemandRate(EResourceType Resource) const;

	/**
	 * Get demand elasticity
	 */
	UFUNCTION(BlueprintCallable, Category = "Demand")
	float GetDemandElasticity(EResourceType Resource) const;

	/**
	 * Set demand rate
	 */
	UFUNCTION(BlueprintCallable, Category = "Demand")
	void SetDemandRate(EResourceType Resource, float Rate);

	/**
	 * Apply demand modifier from events
	 */
	UFUNCTION(BlueprintCallable, Category = "Demand")
	void SetDemandModifier(EResourceType Resource, float Modifier);

	/**
	 * Register player demand (affects market)
	 */
	UFUNCTION(BlueprintCallable, Category = "Demand")
	void RegisterPlayerDemand(EResourceType Resource, int32 Quantity);

	// ============================================================================
	// SUPPLY/DEMAND ANALYSIS
	// ============================================================================

	/**
	 * Get supply/demand ratio for resource
	 * >1.0 = oversupply, <1.0 = undersupply
	 */
	UFUNCTION(BlueprintCallable, Category = "Analysis")
	float GetSupplyDemandRatio(EResourceType Resource) const;

	/**
	 * Get scarcity index (0 = abundant, 1 = scarce)
	 */
	UFUNCTION(BlueprintCallable, Category = "Analysis")
	float GetScarcityIndex(EResourceType Resource) const;

	/**
	 * Get complete supply/demand data for resource
	 */
	UFUNCTION(BlueprintCallable, Category = "Analysis")
	FResourceSupplyDemand GetSupplyDemandData(EResourceType Resource) const;

	/**
	 * Get all supply/demand data
	 */
	UFUNCTION(BlueprintCallable, Category = "Analysis")
	TMap<EResourceType, FResourceSupplyDemand> GetAllSupplyDemandData() const;

	/**
	 * Check if resource is scarce (below threshold)
	 */
	UFUNCTION(BlueprintCallable, Category = "Analysis")
	bool IsResourceScarce(EResourceType Resource, float ScarcityThreshold = 0.7f) const;

	/**
	 * Check if resource is abundant (above threshold)
	 */
	UFUNCTION(BlueprintCallable, Category = "Analysis")
	bool IsResourceAbundant(EResourceType Resource, float AbundanceThreshold = 0.3f) const;

	/**
	 * Get resources sorted by scarcity
	 */
	UFUNCTION(BlueprintCallable, Category = "Analysis")
	TArray<EResourceType> GetResourcesByScarcity(bool bMostScarceFirst = true) const;

	// ============================================================================
	// PRICE HISTORY
	// ============================================================================

	/**
	 * Record a price point in history
	 */
	UFUNCTION(BlueprintCallable, Category = "Price History")
	void RecordPricePoint(EResourceType Resource, int32 Price, int32 Volume);

	/**
	 * Get price history for resource
	 */
	UFUNCTION(BlueprintCallable, Category = "Price History")
	TArray<FPriceHistoryEntry> GetPriceHistory(EResourceType Resource) const;

	/**
	 * Get average price over N entries
	 */
	UFUNCTION(BlueprintCallable, Category = "Price History")
	float GetAveragePrice(EResourceType Resource, int32 NumEntries = 10) const;

	/**
	 * Get price trend direction
	 */
	UFUNCTION(BlueprintCallable, Category = "Price History")
	EMarketTrend GetPriceTrend(EResourceType Resource) const;

	/**
	 * Get price volatility measure
	 */
	UFUNCTION(BlueprintCallable, Category = "Price History")
	float GetPriceVolatility(EResourceType Resource) const;

	/**
	 * Get highest price in recent history
	 */
	UFUNCTION(BlueprintCallable, Category = "Price History")
	int32 GetHighestRecentPrice(EResourceType Resource, int32 NumEntries = 20) const;

	/**
	 * Get lowest price in recent history
	 */
	UFUNCTION(BlueprintCallable, Category = "Price History")
	int32 GetLowestRecentPrice(EResourceType Resource, int32 NumEntries = 20) const;

	// ============================================================================
	// MARKET INFO
	// ============================================================================

	/**
	 * Get market ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Market Info")
	FMarketId GetMarketId() const { return MarketId; }

	/**
	 * Get complete market data
	 */
	UFUNCTION(BlueprintCallable, Category = "Market Info")
	FMarketData GetMarketData() const { return CachedMarketData; }

	/**
	 * Get market location type
	 */
	UFUNCTION(BlueprintCallable, Category = "Market Info")
	EMarketLocationType GetLocationType() const { return LocationType; }

	/**
	 * Check if market specializes in resource
	 */
	UFUNCTION(BlueprintCallable, Category = "Market Info")
	bool IsSpecializedIn(EResourceType Resource) const;

	/**
	 * Get specialization bonus
	 */
	UFUNCTION(BlueprintCallable, Category = "Market Info")
	float GetSpecializationBonus() const { return SpecializationBonus; }

	// ============================================================================
	// SIMULATION
	// ============================================================================

	/**
	 * Simulate supply/demand changes (called internally)
	 */
	UFUNCTION(BlueprintCallable, Category = "Simulation")
	void SimulateSupplyDemand(float DeltaGameHours);

	/**
	 * Force immediate recalculation of all metrics
	 */
	UFUNCTION(BlueprintCallable, Category = "Simulation")
	void RecalculateAllMetrics();

	/**
	 * Reset to default values
	 */
	UFUNCTION(BlueprintCallable, Category = "Simulation")
	void ResetToDefaults();

	// ============================================================================
	// EVENTS
	// ============================================================================

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSupplyDemandChanged OnSupplyDemandChanged;

protected:
	// Market identification
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	FMarketId MarketId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market")
	EMarketLocationType LocationType;

	// Supply/demand data per resource
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TMap<EResourceType, FResourceSupplyDemand> SupplyDemandMap;

	// Price history per resource
	UPROPERTY(BlueprintReadOnly, Category = "History")
	TMap<EResourceType, FDynamicMarketPrice> PriceDataMap;

	// Market specialization
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Specialization")
	TArray<EResourceType> SpecializedResources;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Specialization")
	float SpecializationBonus;

	// Configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int32 MaxPriceHistoryEntries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float SimulationTimeScale;

	// Cached data for efficient access
	UPROPERTY(BlueprintReadOnly, Category = "Cache")
	FMarketData CachedMarketData;

	// Timing
	float AccumulatedSimTime;
	float LastUpdateTime;

private:
	void InitializeDefaultSupplyDemand();
	void UpdateCachedMarketData();
	FResourceSupplyDemand& GetOrCreateSupplyDemand(EResourceType Resource);
	FDynamicMarketPrice& GetOrCreatePriceData(EResourceType Resource);
	EMarketTrend CalculateTrend(const TArray<FPriceHistoryEntry>& History) const;
	float CalculateVolatilityFromHistory(const TArray<FPriceHistoryEntry>& History) const;
};
