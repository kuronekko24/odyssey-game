// UPriceFluctuationSystem.h
// Dynamic pricing system based on supply/demand with market volatility simulation
// Implements price discovery mechanisms and trend analysis

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OdysseyEconomyTypes.h"
#include "UMarketDataComponent.h"
#include "UPriceFluctuationSystem.generated.h"

/**
 * Price calculation result for debugging and analysis
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FPriceCalculationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Price Calculation")
	EResourceType Resource;

	UPROPERTY(BlueprintReadOnly, Category = "Price Calculation")
	int32 BasePrice;

	UPROPERTY(BlueprintReadOnly, Category = "Price Calculation")
	float SupplyDemandFactor;

	UPROPERTY(BlueprintReadOnly, Category = "Price Calculation")
	float VolatilityFactor;

	UPROPERTY(BlueprintReadOnly, Category = "Price Calculation")
	float TrendFactor;

	UPROPERTY(BlueprintReadOnly, Category = "Price Calculation")
	float EventModifier;

	UPROPERTY(BlueprintReadOnly, Category = "Price Calculation")
	float SpecializationModifier;

	UPROPERTY(BlueprintReadOnly, Category = "Price Calculation")
	float FinalMultiplier;

	UPROPERTY(BlueprintReadOnly, Category = "Price Calculation")
	int32 CalculatedPrice;

	UPROPERTY(BlueprintReadOnly, Category = "Price Calculation")
	int32 ClampedPrice;

	FPriceCalculationResult()
		: Resource(EResourceType::None)
		, BasePrice(0)
		, SupplyDemandFactor(1.0f)
		, VolatilityFactor(0.0f)
		, TrendFactor(1.0f)
		, EventModifier(1.0f)
		, SpecializationModifier(1.0f)
		, FinalMultiplier(1.0f)
		, CalculatedPrice(0)
		, ClampedPrice(0)
	{
	}
};

/**
 * UPriceFluctuationSystem - Dynamic Pricing Engine
 *
 * Responsibilities:
 * - Calculate dynamic prices based on supply/demand
 * - Simulate market volatility
 * - Implement price discovery mechanisms
 * - Apply smoothing for realistic price changes
 * - Handle price floors and ceilings
 */
UCLASS(ClassGroup=(Economy), meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UPriceFluctuationSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UPriceFluctuationSystem();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// INITIALIZATION
	// ============================================================================

	/**
	 * Initialize with market data component reference
	 */
	UFUNCTION(BlueprintCallable, Category = "Initialization")
	void Initialize(UMarketDataComponent* InMarketData);

	/**
	 * Set economy configuration
	 */
	UFUNCTION(BlueprintCallable, Category = "Initialization")
	void SetConfiguration(const FEconomyConfiguration& Config);

	// ============================================================================
	// PRICE CALCULATION
	// ============================================================================

	/**
	 * Calculate current buy price for a resource
	 */
	UFUNCTION(BlueprintCallable, Category = "Price")
	int32 CalculateBuyPrice(EResourceType Resource) const;

	/**
	 * Calculate current sell price for a resource
	 */
	UFUNCTION(BlueprintCallable, Category = "Price")
	int32 CalculateSellPrice(EResourceType Resource) const;

	/**
	 * Get buy price for a specific quantity (may include volume discounts)
	 */
	UFUNCTION(BlueprintCallable, Category = "Price")
	int32 CalculateBuyPriceForQuantity(EResourceType Resource, int32 Quantity) const;

	/**
	 * Get sell price for a specific quantity (may include volume penalties)
	 */
	UFUNCTION(BlueprintCallable, Category = "Price")
	int32 CalculateSellPriceForQuantity(EResourceType Resource, int32 Quantity) const;

	/**
	 * Get detailed price calculation breakdown
	 */
	UFUNCTION(BlueprintCallable, Category = "Price")
	FPriceCalculationResult GetPriceCalculationDetails(EResourceType Resource) const;

	/**
	 * Get current price multiplier for resource
	 */
	UFUNCTION(BlueprintCallable, Category = "Price")
	float GetCurrentPriceMultiplier(EResourceType Resource) const;

	// ============================================================================
	// MARKET DYNAMICS
	// ============================================================================

	/**
	 * Update all prices (called internally by tick)
	 */
	UFUNCTION(BlueprintCallable, Category = "Market")
	void UpdateAllPrices();

	/**
	 * Update price for specific resource
	 */
	UFUNCTION(BlueprintCallable, Category = "Market")
	void UpdateResourcePrice(EResourceType Resource);

	/**
	 * Apply price shock (immediate price change)
	 */
	UFUNCTION(BlueprintCallable, Category = "Market")
	void ApplyPriceShock(EResourceType Resource, float ShockMultiplier, float DecayRate);

	/**
	 * Set volatility for resource
	 */
	UFUNCTION(BlueprintCallable, Category = "Market")
	void SetResourceVolatility(EResourceType Resource, EMarketVolatility Volatility);

	/**
	 * Apply event price modifier
	 */
	UFUNCTION(BlueprintCallable, Category = "Market")
	void ApplyEventModifier(EResourceType Resource, float Modifier, float Duration);

	/**
	 * Clear all event modifiers for resource
	 */
	UFUNCTION(BlueprintCallable, Category = "Market")
	void ClearEventModifiers(EResourceType Resource);

	// ============================================================================
	// SUPPLY/DEMAND FACTORS
	// ============================================================================

	/**
	 * Calculate supply/demand price factor
	 * Returns multiplier (>1 if demand > supply, <1 if supply > demand)
	 */
	UFUNCTION(BlueprintCallable, Category = "Factors")
	float CalculateSupplyDemandFactor(EResourceType Resource) const;

	/**
	 * Calculate scarcity premium
	 */
	UFUNCTION(BlueprintCallable, Category = "Factors")
	float CalculateScarcityPremium(EResourceType Resource) const;

	/**
	 * Calculate abundance discount
	 */
	UFUNCTION(BlueprintCallable, Category = "Factors")
	float CalculateAbundanceDiscount(EResourceType Resource) const;

	// ============================================================================
	// VOLATILITY SIMULATION
	// ============================================================================

	/**
	 * Generate volatility factor for price randomization
	 */
	UFUNCTION(BlueprintCallable, Category = "Volatility")
	float GenerateVolatilityFactor(EResourceType Resource) const;

	/**
	 * Get volatility percentage range for resource
	 */
	UFUNCTION(BlueprintCallable, Category = "Volatility")
	float GetVolatilityRange(EResourceType Resource) const;

	/**
	 * Simulate market noise
	 */
	UFUNCTION(BlueprintCallable, Category = "Volatility")
	float SimulateMarketNoise(float BaseVolatility) const;

	// ============================================================================
	// TREND ANALYSIS
	// ============================================================================

	/**
	 * Calculate trend-based price adjustment
	 */
	UFUNCTION(BlueprintCallable, Category = "Trends")
	float CalculateTrendFactor(EResourceType Resource) const;

	/**
	 * Get trend momentum
	 */
	UFUNCTION(BlueprintCallable, Category = "Trends")
	float GetTrendMomentum(EResourceType Resource) const;

	/**
	 * Predict future price (simple extrapolation)
	 */
	UFUNCTION(BlueprintCallable, Category = "Trends")
	int32 PredictFuturePrice(EResourceType Resource, float HoursAhead) const;

	// ============================================================================
	// PRICE HISTORY
	// ============================================================================

	/**
	 * Get current prices for all resources
	 */
	UFUNCTION(BlueprintCallable, Category = "Prices")
	TMap<EResourceType, FDynamicMarketPrice> GetAllCurrentPrices() const;

	/**
	 * Get price data for specific resource
	 */
	UFUNCTION(BlueprintCallable, Category = "Prices")
	FDynamicMarketPrice GetPriceData(EResourceType Resource) const;

	/**
	 * Record trade for price history
	 */
	UFUNCTION(BlueprintCallable, Category = "Prices")
	void RecordTrade(EResourceType Resource, int32 Price, int32 Volume, bool bWasBuy);

	// ============================================================================
	// EVENTS
	// ============================================================================

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMarketPriceChanged OnPriceChanged;

protected:
	// Market data reference
	UPROPERTY()
	UMarketDataComponent* MarketDataComponent;

	// Current prices per resource
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TMap<EResourceType, FDynamicMarketPrice> CurrentPrices;

	// Event modifiers (Resource -> {Modifier, ExpirationTime})
	TMap<EResourceType, TArray<TPair<float, double>>> ActiveEventModifiers;

	// Price shocks (Resource -> {CurrentMultiplier, DecayRate})
	TMap<EResourceType, TPair<float, float>> ActivePriceShocks;

	// Configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FEconomyConfiguration EconomyConfig;

	// Timing
	float TimeSinceLastUpdate;
	float TimeSinceLastVolatilityUpdate;

	// Random stream for deterministic randomization
	FRandomStream PriceRandomStream;

private:
	float CalculateBaseMultiplier(EResourceType Resource) const;
	float ApplyPriceSmoothing(float TargetMultiplier, float CurrentMultiplier) const;
	float GetVolatilityMultiplierRange(EMarketVolatility Volatility) const;
	void UpdateEventModifiers(float DeltaTime);
	void DecayPriceShocks(float DeltaTime);
	void InitializeDefaultPrices();
	FDynamicMarketPrice& GetOrCreatePrice(EResourceType Resource);
};
