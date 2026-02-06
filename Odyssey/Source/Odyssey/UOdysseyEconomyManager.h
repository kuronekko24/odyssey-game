// UOdysseyEconomyManager.h
// Master controller for the Dynamic Economy Simulation System
// Coordinates all economic subsystems and integrates with combat/crafting

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "OdysseyEconomyTypes.h"
#include "OdysseyEventBus.h"
#include "UMarketDataComponent.h"
#include "UPriceFluctuationSystem.h"
#include "UTradeRouteAnalyzer.h"
#include "UEconomicEventSystem.h"
#include "UOdysseyEconomyManager.generated.h"

// Forward declarations
class AOdysseyCharacter;
class UOdysseyCraftingComponent;
class UOdysseyTradingComponent;
class UEconomyRippleEffect;
class UEconomySaveSystem;
/**
 * Combat impact on economy
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FCombatEconomyImpact
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Impact")
	float PirateActivityIncrease;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Impact")
	float TradeRouteRiskIncrease;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Impact")
	float ResourceDropRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Impact")
	float BountyValue;

	FCombatEconomyImpact()
		: PirateActivityIncrease(0.1f)
		, TradeRouteRiskIncrease(0.05f)
		, ResourceDropRate(0.25f)
		, BountyValue(100.0f)
	{
	}
};

/**
 * Crafting impact on economy
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FCraftingEconomyImpact
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Impact")
	float ResourceConsumptionMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Impact")
	float CraftedGoodsPriceBonus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Impact")
	float ProductionEfficiencyBonus;

	FCraftingEconomyImpact()
		: ResourceConsumptionMultiplier(1.0f)
		, CraftedGoodsPriceBonus(1.2f)
		, ProductionEfficiencyBonus(1.0f)
	{
	}
};

/**
 * Economy statistics for analytics
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FEconomyStatistics
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Statistics")
	int64 TotalTradeVolume;

	UPROPERTY(BlueprintReadOnly, Category = "Statistics")
	int64 TotalTransactionValue;

	UPROPERTY(BlueprintReadOnly, Category = "Statistics")
	int32 TotalEventsGenerated;

	UPROPERTY(BlueprintReadOnly, Category = "Statistics")
	int32 ActiveMarkets;

	UPROPERTY(BlueprintReadOnly, Category = "Statistics")
	int32 ActiveTradeRoutes;

	UPROPERTY(BlueprintReadOnly, Category = "Statistics")
	float AveragePriceVolatility;

	UPROPERTY(BlueprintReadOnly, Category = "Statistics")
	EResourceType MostTradedResource;

	UPROPERTY(BlueprintReadOnly, Category = "Statistics")
	EResourceType MostProfitableResource;

	FEconomyStatistics()
		: TotalTradeVolume(0)
		, TotalTransactionValue(0)
		, TotalEventsGenerated(0)
		, ActiveMarkets(0)
		, ActiveTradeRoutes(0)
		, AveragePriceVolatility(0.0f)
		, MostTradedResource(EResourceType::None)
		, MostProfitableResource(EResourceType::None)
	{
	}
};

/**
 * UOdysseyEconomyManager - Master Economy Controller
 *
 * Responsibilities:
 * - Coordinate all economic subsystems
 * - Integrate with combat and crafting systems
 * - Process economic events from the event bus
 * - Manage market registration and routing
 * - Provide unified API for economy queries
 * - Track global economy statistics
 */
UCLASS(ClassGroup=(Economy), meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UOdysseyEconomyManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdysseyEconomyManager();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// INITIALIZATION
	// ============================================================================

	/**
	 * Initialize the economy system with configuration
	 */
	UFUNCTION(BlueprintCallable, Category = "Initialization")
	void InitializeEconomy(const FEconomyConfiguration& Config);

	/**
	 * Load economy data from data tables
	 */
	UFUNCTION(BlueprintCallable, Category = "Initialization")
	void LoadEconomyData(
		UDataTable* MarketDataTable,
		UDataTable* ResourceDataTable,
		UDataTable* EventTemplateTable);

	/**
	 * Register with event bus
	 */
	UFUNCTION(BlueprintCallable, Category = "Initialization")
	void ConnectToEventBus(UOdysseyEventBus* EventBus);

	// ============================================================================
	// MARKET MANAGEMENT
	// ============================================================================

	/**
	 * Create and register a new market
	 */
	UFUNCTION(BlueprintCallable, Category = "Markets")
	bool CreateMarket(const FMarketId& MarketId, const FString& DisplayName, FVector WorldLocation, EMarketLocationType Type);

	/**
	 * Remove a market
	 */
	UFUNCTION(BlueprintCallable, Category = "Markets")
	void RemoveMarket(const FMarketId& MarketId);

	/**
	 * Get market data component
	 */
	UFUNCTION(BlueprintCallable, Category = "Markets")
	UMarketDataComponent* GetMarketData(const FMarketId& MarketId) const;

	/**
	 * Get price system for market
	 */
	UFUNCTION(BlueprintCallable, Category = "Markets")
	UPriceFluctuationSystem* GetPriceSystem(const FMarketId& MarketId) const;

	/**
	 * Get all registered markets
	 */
	UFUNCTION(BlueprintCallable, Category = "Markets")
	TArray<FMarketId> GetAllMarkets() const;

	/**
	 * Get nearest market to location
	 */
	UFUNCTION(BlueprintCallable, Category = "Markets")
	FMarketId GetNearestMarket(FVector Location) const;

	// ============================================================================
	// TRADING API
	// ============================================================================

	/**
	 * Execute a buy transaction
	 */
	UFUNCTION(BlueprintCallable, Category = "Trading")
	bool ExecuteBuy(
		const FMarketId& MarketId,
		EResourceType Resource,
		int32 Quantity,
		AOdysseyCharacter* Buyer);

	/**
	 * Execute a sell transaction
	 */
	UFUNCTION(BlueprintCallable, Category = "Trading")
	bool ExecuteSell(
		const FMarketId& MarketId,
		EResourceType Resource,
		int32 Quantity,
		AOdysseyCharacter* Seller);

	/**
	 * Get buy price at market
	 */
	UFUNCTION(BlueprintCallable, Category = "Trading")
	int32 GetBuyPrice(const FMarketId& MarketId, EResourceType Resource, int32 Quantity = 1) const;

	/**
	 * Get sell price at market
	 */
	UFUNCTION(BlueprintCallable, Category = "Trading")
	int32 GetSellPrice(const FMarketId& MarketId, EResourceType Resource, int32 Quantity = 1) const;

	/**
	 * Check if resource can be bought at market
	 */
	UFUNCTION(BlueprintCallable, Category = "Trading")
	bool CanBuy(const FMarketId& MarketId, EResourceType Resource, int32 Quantity) const;

	/**
	 * Check if resource can be sold at market
	 */
	UFUNCTION(BlueprintCallable, Category = "Trading")
	bool CanSell(const FMarketId& MarketId, EResourceType Resource, int32 Quantity) const;

	// ============================================================================
	// TRADE ROUTES & OPPORTUNITIES
	// ============================================================================

	/**
	 * Get trade route analyzer
	 */
	UFUNCTION(BlueprintCallable, Category = "Trade Routes")
	UTradeRouteAnalyzer* GetTradeRouteAnalyzer() const { return TradeRouteAnalyzer; }

	/**
	 * Get top trade opportunities
	 */
	UFUNCTION(BlueprintCallable, Category = "Trade Routes")
	TArray<FTradeOpportunity> GetTopTradeOpportunities(int32 MaxCount = 10) const;

	/**
	 * Get opportunities from player's current location
	 */
	UFUNCTION(BlueprintCallable, Category = "Trade Routes")
	TArray<FTradeOpportunity> GetOpportunitiesFromLocation(FVector PlayerLocation, int32 MaxCount = 10) const;

	/**
	 * Find best trade route for resource
	 */
	UFUNCTION(BlueprintCallable, Category = "Trade Routes")
	FTradeRoute FindBestRouteForResource(EResourceType Resource) const;

	// ============================================================================
	// ECONOMIC EVENTS
	// ============================================================================

	/**
	 * Get economic event system
	 */
	UFUNCTION(BlueprintCallable, Category = "Events")
	UEconomicEventSystem* GetEventSystem() const { return EconomicEventSystem; }

	/**
	 * Trigger economic event manually
	 */
	UFUNCTION(BlueprintCallable, Category = "Events")
	int32 TriggerEconomicEvent(EEconomicEventType EventType, const TArray<FMarketId>& Markets);

	/**
	 * Get active economic events
	 */
	UFUNCTION(BlueprintCallable, Category = "Events")
	TArray<FEconomicEvent> GetActiveEvents() const;

	/**
	 * Get news headlines
	 */
	UFUNCTION(BlueprintCallable, Category = "Events")
	TArray<FString> GetEconomyNews(int32 MaxCount = 5) const;

	// ============================================================================

	// ============================================================================
	// RIPPLE EFFECTS
	// ============================================================================

	UFUNCTION(BlueprintCallable, Category = "Ripple Effects")
	UEconomyRippleEffect* GetRippleEffectSystem() const { return RippleEffectSystem; }

	// ============================================================================
	// SAVE/LOAD
	// ============================================================================

	UFUNCTION(BlueprintCallable, Category = "Save System")
	UEconomySaveSystem* GetSaveSystem() const { return SaveSystem; }

	UFUNCTION(BlueprintCallable, Category = "Save System")
	bool QuickSave();

	UFUNCTION(BlueprintCallable, Category = "Save System")
	bool QuickLoad();
	// COMBAT INTEGRATION
	// ============================================================================

	/**
	 * Report combat event for economic impact
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat Integration")
	void ReportCombatEvent(AActor* Attacker, AActor* Victim, float DamageDealt, bool bWasKill);

	/**
	 * Get combat loot value
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat Integration")
	int32 CalculateCombatLootValue(AActor* DefeatedEnemy) const;

	/**
	 * Apply combat zone economic effects
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat Integration")
	void ApplyCombatZoneEffects(const FMarketId& NearestMarket, float CombatIntensity);

	// ============================================================================
	// CRAFTING INTEGRATION
	// ============================================================================

	/**
	 * Report crafting activity for demand tracking
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting Integration")
	void ReportCraftingActivity(EResourceType ConsumedResource, int32 Quantity, EResourceType ProducedResource, int32 ProducedQuantity);

	/**
	 * Get crafted item value bonus
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting Integration")
	float GetCraftedItemValueBonus(EResourceType CraftedResource) const;

	/**
	 * Calculate resource demand from crafting
	 */
	UFUNCTION(BlueprintCallable, Category = "Crafting Integration")
	float GetCraftingDemandMultiplier(EResourceType Resource) const;

	// ============================================================================
	// STATISTICS & ANALYTICS
	// ============================================================================

	/**
	 * Get economy statistics
	 */
	UFUNCTION(BlueprintCallable, Category = "Statistics")
	FEconomyStatistics GetStatistics() const;

	/**
	 * Get global price trend for resource
	 */
	UFUNCTION(BlueprintCallable, Category = "Statistics")
	EMarketTrend GetGlobalPriceTrend(EResourceType Resource) const;

	/**
	 * Get average price across all markets
	 */
	UFUNCTION(BlueprintCallable, Category = "Statistics")
	float GetAverageMarketPrice(EResourceType Resource) const;

	/**
	 * Get price range (min/max) across markets
	 */
	UFUNCTION(BlueprintCallable, Category = "Statistics")
	void GetPriceRange(EResourceType Resource, int32& OutMinPrice, int32& OutMaxPrice) const;

	// ============================================================================
	// PLAYER ECONOMY
	// ============================================================================

	/**
	 * Calculate player's net worth
	 */
	UFUNCTION(BlueprintCallable, Category = "Player Economy")
	int32 CalculatePlayerNetWorth(AOdysseyCharacter* Player) const;

	/**
	 * Get player's trading history summary
	 */
	UFUNCTION(BlueprintCallable, Category = "Player Economy")
	FString GetPlayerTradingSummary(AOdysseyCharacter* Player) const;

	/**
	 * Recommend trades for player based on inventory
	 */
	UFUNCTION(BlueprintCallable, Category = "Player Economy")
	TArray<FTradeOpportunity> GetRecommendedTrades(AOdysseyCharacter* Player, int32 MaxCount = 5) const;

	// ============================================================================
	// CONFIGURATION
	// ============================================================================

	/**
	 * Get current configuration
	 */
	UFUNCTION(BlueprintCallable, Category = "Config")
	FEconomyConfiguration GetConfiguration() const { return EconomyConfig; }

	/**
	 * Update configuration
	 */
	UFUNCTION(BlueprintCallable, Category = "Config")
	void UpdateConfiguration(const FEconomyConfiguration& NewConfig);

	/**
	 * Set simulation time scale
	 */
	UFUNCTION(BlueprintCallable, Category = "Config")
	void SetTimeScale(float Scale);

	/**
	 * Pause/resume economy simulation
	 */
	UFUNCTION(BlueprintCallable, Category = "Config")
	void SetSimulationPaused(bool bPaused);

	// ============================================================================
	// EVENTS
	// ============================================================================

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMarketPriceChanged OnPriceChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnEconomicEventStarted OnEventStarted;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnEconomicEventEnded OnEventEnded;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnTradeOpportunityFound OnOpportunityFound;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMarketTradeCompleted OnTradeCompleted;

	// ============================================================================
	// SINGLETON ACCESS
	// ============================================================================

	/**
	 * Get global economy manager instance
	 */
	UFUNCTION(BlueprintCallable, Category = "Singleton")
	static UOdysseyEconomyManager* Get();

protected:
	// Subsystem components
	UPROPERTY()
	UTradeRouteAnalyzer* TradeRouteAnalyzer;

	UPROPERTY()
	UEconomicEventSystem* EconomicEventSystem;

	UPROPERTY()
	UEconomyRippleEffect* RippleEffectSystem;

	UPROPERTY()
	UEconomySaveSystem* SaveSystem;
	// Market components (MarketId -> Components)
	UPROPERTY()
	TMap<FName, UMarketDataComponent*> MarketDataComponents;

	UPROPERTY()
	TMap<FName, UPriceFluctuationSystem*> PriceSystems;

	// Market locations
	TMap<FName, FVector> MarketLocations;
	TArray<FMarketId> RegisteredMarkets;

	// Event bus reference
	UPROPERTY()
	UOdysseyEventBus* EventBus;

	// Event subscriptions
	TArray<FOdysseyEventHandle> EventSubscriptions;

	// Configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FEconomyConfiguration EconomyConfig;

	// Combat/Crafting impact settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Integration")
	FCombatEconomyImpact CombatImpact;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Integration")
	FCraftingEconomyImpact CraftingImpact;

	// Statistics
	UPROPERTY(BlueprintReadOnly, Category = "Statistics")
	FEconomyStatistics Statistics;

	// State
	bool bIsInitialized;
	bool bSimulationPaused;
	float TimeScale;

	// Crafting demand tracking
	TMap<EResourceType, float> CraftingDemandMultipliers;

	// Singleton
	static UOdysseyEconomyManager* GlobalInstance;

private:
	void CreateSubsystems();
	void SetupEventListeners();
	void HandleCombatEvent(const FCombatEventPayload& Payload);
	void HandleInteractionEvent(const FInteractionEventPayload& Payload);
	void UpdateStatistics();
	FName GetMarketKey(const FMarketId& MarketId) const;
	void BroadcastPriceChange(EResourceType Resource, const FDynamicMarketPrice& Price);

	// UFUNCTION handlers for subsystem delegate binding
	UFUNCTION()
	void HandlePriceChanged(EResourceType Resource, const FDynamicMarketPrice& NewPrice);

	UFUNCTION()
	void HandleEconomicEventStarted(const FEconomicEvent& Event);

	UFUNCTION()
	void HandleEconomicEventEnded(const FEconomicEvent& Event);

	UFUNCTION()
	void HandleOpportunityFound(const FTradeOpportunity& Opportunity);};
