// EconomyRippleEffect.h
// Economic ripple propagation system for Odyssey Dynamic Economy (Task #5)
//
// Implements chain-reaction economics: when a market event occurs (war, shortage,
// discovery), its effects propagate outward through connected trade routes with
// distance-based dampening. This creates realistic "butterfly effect" dynamics
// where a pirate attack on one station can cause price spikes several hops away.
//
// Key design choices:
// - Wave-front propagation (BFS) rather than instant global application
// - Configurable dampening per hop for tunable realism
// - Cycle detection to prevent infinite propagation loops
// - Mobile-friendly: processes one wave per tick, not the whole graph

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OdysseyEconomyTypes.h"
#include "EconomyRippleEffect.generated.h"

// Forward declarations
class UMarketDataComponent;
class UPriceFluctuationSystem;
class UTradeRouteAnalyzer;

/**
 * UEconomyRippleEffect - Economic Chain Reaction Propagation Engine
 *
 * Creates and propagates economic ripple effects through the market network.
 * When an event occurs at a market, the ripple system:
 *   1. Creates an FEconomicRipple at the origin
 *   2. Each tick, advances the wave-front by one hop
 *   3. At each new market, applies a dampened version of the original effect
 *   4. Continues until magnitude drops below threshold or max depth reached
 *
 * This component is owned by UOdysseyEconomyManager and should not be
 * manually attached to actors.
 */
UCLASS(ClassGroup=(Economy), meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UEconomyRippleEffect : public UActorComponent
{
	GENERATED_BODY()

public:
	UEconomyRippleEffect();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// INITIALIZATION
	// ============================================================================

	/**
	 * Initialize the ripple system with references to market infrastructure
	 * @param InMarketDataMap  Map of market key -> UMarketDataComponent
	 * @param InPriceSystemMap Map of market key -> UPriceFluctuationSystem
	 * @param InTradeRouteAnalyzer The route analyzer for finding connected markets
	 * @param InConfig Economy configuration
	 */
	UFUNCTION(BlueprintCallable, Category = "Ripple|Init")
	void InitializeRippleSystem(
		const FEconomyConfiguration& InConfig);

	/** Provide references to market components (call after markets are registered) */
	void SetMarketReferences(
		TMap<FName, UMarketDataComponent*>* InMarketDataMap,
		TMap<FName, UPriceFluctuationSystem*>* InPriceSystemMap,
		UTradeRouteAnalyzer* InTradeRouteAnalyzer);

	// ============================================================================
	// RIPPLE CREATION
	// ============================================================================

	/**
	 * Create a supply shock ripple from a market
	 * @param OriginMarket   Market where the supply change occurred
	 * @param Resources      Affected resources
	 * @param Magnitude      Strength of the shock (0.0-1.0, negative = supply decrease)
	 * @param SourceEventId  Optional originating event ID
	 * @return Ripple ID, or -1 on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "Ripple|Create")
	int32 CreateSupplyShockRipple(
		const FMarketId& OriginMarket,
		const TArray<EResourceType>& Resources,
		float Magnitude,
		int32 SourceEventId = -1);

	/**
	 * Create a demand shock ripple
	 */
	UFUNCTION(BlueprintCallable, Category = "Ripple|Create")
	int32 CreateDemandShockRipple(
		const FMarketId& OriginMarket,
		const TArray<EResourceType>& Resources,
		float Magnitude,
		int32 SourceEventId = -1);

	/**
	 * Create a price shock ripple
	 */
	UFUNCTION(BlueprintCallable, Category = "Ripple|Create")
	int32 CreatePriceShockRipple(
		const FMarketId& OriginMarket,
		const TArray<EResourceType>& Resources,
		float Magnitude,
		int32 SourceEventId = -1);

	/**
	 * Create a trade disruption ripple (e.g., from route blockage or pirate activity)
	 */
	UFUNCTION(BlueprintCallable, Category = "Ripple|Create")
	int32 CreateTradeDisruptionRipple(
		const FMarketId& OriginMarket,
		float Magnitude,
		int32 SourceEventId = -1);

	/**
	 * Create a combat zone ripple (from nearby combat activity)
	 */
	UFUNCTION(BlueprintCallable, Category = "Ripple|Create")
	int32 CreateCombatZoneRipple(
		const FMarketId& NearestMarket,
		float CombatIntensity,
		int32 SourceEventId = -1);

	/**
	 * Create a crafting demand ripple (surge in crafting creates ingredient demand wave)
	 */
	UFUNCTION(BlueprintCallable, Category = "Ripple|Create")
	int32 CreateCraftingDemandRipple(
		const FMarketId& CraftingMarket,
		const TArray<EResourceType>& IngredientResources,
		float DemandIntensity,
		int32 SourceEventId = -1);

	/**
	 * Create a generic ripple with full parameter control
	 */
	UFUNCTION(BlueprintCallable, Category = "Ripple|Create")
	int32 CreateRipple(const FEconomicRipple& RippleTemplate);

	// ============================================================================
	// RIPPLE QUERIES
	// ============================================================================

	/** Get all currently active ripples */
	UFUNCTION(BlueprintCallable, Category = "Ripple|Query")
	TArray<FEconomicRipple> GetActiveRipples() const;

	/** Get ripple by ID */
	UFUNCTION(BlueprintCallable, Category = "Ripple|Query")
	FEconomicRipple GetRipple(int32 RippleId) const;

	/** Get active ripple count */
	UFUNCTION(BlueprintCallable, Category = "Ripple|Query")
	int32 GetActiveRippleCount() const { return ActiveRipples.Num(); }

	/** Cancel a specific ripple */
	UFUNCTION(BlueprintCallable, Category = "Ripple|Control")
	bool CancelRipple(int32 RippleId);

	/** Cancel all active ripples */
	UFUNCTION(BlueprintCallable, Category = "Ripple|Control")
	void CancelAllRipples();

	// ============================================================================
	// CONFIGURATION
	// ============================================================================

	UFUNCTION(BlueprintCallable, Category = "Ripple|Config")
	void SetConfiguration(const FEconomyConfiguration& InConfig);

	// ============================================================================
	// EVENTS
	// ============================================================================

	UPROPERTY(BlueprintAssignable, Category = "Ripple|Events")
	FOnEconomicRipplePropagated OnRipplePropagated;

protected:
	/** Active ripples being propagated */
	UPROPERTY(BlueprintReadOnly, Category = "Ripple|State")
	TArray<FEconomicRipple> ActiveRipples;

	/** Configuration reference */
	FEconomyConfiguration EconomyConfig;

	/** External references (not owned) */
	TMap<FName, UMarketDataComponent*>* MarketDataMap;
	TMap<FName, UPriceFluctuationSystem*>* PriceSystemMap;

	UPROPERTY()
	UTradeRouteAnalyzer* TradeRouteAnalyzer;

	/** Next ripple ID counter */
	int32 NextRippleId;

private:
	/**
	 * Advance all active ripples by one propagation step
	 * Called from TickComponent.
	 */
	void PropagateRipples(float DeltaTime);

	/**
	 * Apply a ripple's effect to a specific market
	 */
	void ApplyRippleToMarket(const FEconomicRipple& Ripple, const FMarketId& MarketId, float EffectiveMagnitude);

	/**
	 * Find markets connected to the given market via trade routes
	 * Excludes already-visited markets.
	 */
	TArray<FMarketId> GetConnectedMarkets(const FMarketId& MarketId, const TArray<FMarketId>& ExcludeList) const;

	/** Convert a FMarketId to the FName key used in component maps */
	FName GetMarketKey(const FMarketId& MarketId) const;

	/** Create a base ripple with common settings */
	FEconomicRipple CreateBaseRipple(
		ERippleType Type,
		const FMarketId& Origin,
		const TArray<EResourceType>& Resources,
		float Magnitude,
		int32 SourceEventId);
};
