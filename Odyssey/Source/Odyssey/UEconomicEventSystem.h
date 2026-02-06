// UEconomicEventSystem.h
// Economic event generation and management system
// Creates market disruptions, opportunities, and narrative-driven economic events

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "OdysseyEconomyTypes.h"
#include "UMarketDataComponent.h"
#include "UPriceFluctuationSystem.h"
#include "UEconomicEventSystem.generated.h"

/**
 * Event generation parameters
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FEventGenerationParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	float BaseEventChancePerHour;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	float MinTimeBetweenEvents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	float MaxActiveEvents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool bAllowCatastrophicEvents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	float CatastrophicEventChance;

	FEventGenerationParams()
		: BaseEventChancePerHour(0.2f)
		, MinTimeBetweenEvents(30.0f)
		, MaxActiveEvents(5)
		, bAllowCatastrophicEvents(true)
		, CatastrophicEventChance(0.05f)
	{
	}
};

/**
 * Event cooldown tracking
 */
USTRUCT()
struct FEventCooldown
{
	GENERATED_BODY()

	EEconomicEventType EventType;
	double LastOccurrenceTime;
	float CooldownDuration;

	FEventCooldown()
		: EventType(EEconomicEventType::None)
		, LastOccurrenceTime(0.0)
		, CooldownDuration(0.0f)
	{
	}
};

/**
 * UEconomicEventSystem - Market Event Generator
 *
 * Responsibilities:
 * - Generate random economic events (wars, discoveries, shortages)
 * - Apply economic impacts to markets
 * - Create narrative-driven opportunities
 * - Manage event lifecycles
 * - Provide news/notification content
 */
UCLASS(ClassGroup=(Economy), meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UEconomicEventSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UEconomicEventSystem();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// INITIALIZATION
	// ============================================================================

	/**
	 * Initialize event system
	 */
	UFUNCTION(BlueprintCallable, Category = "Initialization")
	void Initialize(const FEventGenerationParams& Params);

	/**
	 * Load event templates from data table
	 */
	UFUNCTION(BlueprintCallable, Category = "Initialization")
	void LoadEventTemplates(UDataTable* EventDataTable);

	/**
	 * Register a market to receive events
	 */
	UFUNCTION(BlueprintCallable, Category = "Initialization")
	void RegisterMarket(const FMarketId& MarketId, UMarketDataComponent* MarketData, UPriceFluctuationSystem* PriceSystem);

	/**
	 * Unregister a market
	 */
	UFUNCTION(BlueprintCallable, Category = "Initialization")
	void UnregisterMarket(const FMarketId& MarketId);

	// ============================================================================
	// EVENT MANAGEMENT
	// ============================================================================

	/**
	 * Manually trigger an event
	 */
	UFUNCTION(BlueprintCallable, Category = "Events")
	int32 TriggerEvent(const FEconomicEvent& Event);

	/**
	 * Trigger event by type with automatic parameters
	 */
	UFUNCTION(BlueprintCallable, Category = "Events")
	int32 TriggerEventByType(EEconomicEventType EventType, const TArray<FMarketId>& AffectedMarkets);

	/**
	 * Cancel an active event
	 */
	UFUNCTION(BlueprintCallable, Category = "Events")
	bool CancelEvent(int32 EventId);

	/**
	 * Force expire an event immediately
	 */
	UFUNCTION(BlueprintCallable, Category = "Events")
	void ForceExpireEvent(int32 EventId);

	/**
	 * Extend event duration
	 */
	UFUNCTION(BlueprintCallable, Category = "Events")
	void ExtendEventDuration(int32 EventId, float AdditionalSeconds);

	/**
	 * Modify event severity
	 */
	UFUNCTION(BlueprintCallable, Category = "Events")
	void ModifyEventSeverity(int32 EventId, EEconomicEventSeverity NewSeverity);

	// ============================================================================
	// EVENT QUERIES
	// ============================================================================

	/**
	 * Get all active events
	 */
	UFUNCTION(BlueprintCallable, Category = "Events")
	TArray<FEconomicEvent> GetActiveEvents() const;

	/**
	 * Get events affecting a specific market
	 */
	UFUNCTION(BlueprintCallable, Category = "Events")
	TArray<FEconomicEvent> GetEventsAffectingMarket(const FMarketId& MarketId) const;

	/**
	 * Get events affecting a specific resource
	 */
	UFUNCTION(BlueprintCallable, Category = "Events")
	TArray<FEconomicEvent> GetEventsAffectingResource(EResourceType Resource) const;

	/**
	 * Get event by ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Events")
	FEconomicEvent GetEvent(int32 EventId) const;

	/**
	 * Check if event is active
	 */
	UFUNCTION(BlueprintCallable, Category = "Events")
	bool IsEventActive(int32 EventId) const;

	/**
	 * Get number of active events
	 */
	UFUNCTION(BlueprintCallable, Category = "Events")
	int32 GetActiveEventCount() const { return ActiveEvents.Num(); }

	/**
	 * Get event history
	 */
	UFUNCTION(BlueprintCallable, Category = "Events")
	TArray<FEconomicEvent> GetEventHistory(int32 MaxCount = 20) const;

	// ============================================================================
	// EVENT GENERATION
	// ============================================================================

	/**
	 * Generate random event (called internally)
	 */
	UFUNCTION(BlueprintCallable, Category = "Generation")
	bool TryGenerateRandomEvent();

	/**
	 * Generate event for specific market
	 */
	UFUNCTION(BlueprintCallable, Category = "Generation")
	int32 GenerateMarketEvent(const FMarketId& MarketId);

	/**
	 * Generate resource-specific event
	 */
	UFUNCTION(BlueprintCallable, Category = "Generation")
	int32 GenerateResourceEvent(EResourceType Resource);

	/**
	 * Generate chain reaction event (triggered by another event)
	 */
	UFUNCTION(BlueprintCallable, Category = "Generation")
	int32 GenerateChainEvent(int32 TriggeringEventId);

	// ============================================================================
	// EVENT TEMPLATES
	// ============================================================================

	/**
	 * Add event template
	 */
	UFUNCTION(BlueprintCallable, Category = "Templates")
	void AddEventTemplate(const FEconomicEventTemplate& Template);

	/**
	 * Get available event types
	 */
	UFUNCTION(BlueprintCallable, Category = "Templates")
	TArray<EEconomicEventType> GetAvailableEventTypes() const;

	/**
	 * Create event from template
	 */
	UFUNCTION(BlueprintCallable, Category = "Templates")
	FEconomicEvent CreateEventFromTemplate(EEconomicEventType EventType, const TArray<FMarketId>& Markets, const TArray<EResourceType>& Resources) const;

	// ============================================================================
	// NEWS AND NOTIFICATIONS
	// ============================================================================

	/**
	 * Get latest news headlines
	 */
	UFUNCTION(BlueprintCallable, Category = "News")
	TArray<FString> GetLatestHeadlines(int32 MaxCount = 5) const;

	/**
	 * Get news for specific event
	 */
	UFUNCTION(BlueprintCallable, Category = "News")
	FString GetEventHeadline(int32 EventId) const;

	/**
	 * Get detailed news body for event
	 */
	UFUNCTION(BlueprintCallable, Category = "News")
	FString GetEventNewsBody(int32 EventId) const;

	/**
	 * Check for unread notifications
	 */
	UFUNCTION(BlueprintCallable, Category = "News")
	bool HasUnreadNotifications() const;

	/**
	 * Mark notifications as read
	 */
	UFUNCTION(BlueprintCallable, Category = "News")
	void MarkNotificationsRead();

	// ============================================================================
	// IMPACT CALCULATION
	// ============================================================================

	/**
	 * Calculate total event impact on resource at market
	 */
	UFUNCTION(BlueprintCallable, Category = "Impact")
	float GetTotalSupplyModifier(const FMarketId& MarketId, EResourceType Resource) const;

	/**
	 * Calculate total demand modifier from events
	 */
	UFUNCTION(BlueprintCallable, Category = "Impact")
	float GetTotalDemandModifier(const FMarketId& MarketId, EResourceType Resource) const;

	/**
	 * Calculate total price modifier from events
	 */
	UFUNCTION(BlueprintCallable, Category = "Impact")
	float GetTotalPriceModifier(const FMarketId& MarketId, EResourceType Resource) const;

	/**
	 * Get volatility increase from events
	 */
	UFUNCTION(BlueprintCallable, Category = "Impact")
	float GetEventVolatilityIncrease(const FMarketId& MarketId) const;

	// ============================================================================
	// CONFIGURATION
	// ============================================================================

	/**
	 * Set generation parameters
	 */
	UFUNCTION(BlueprintCallable, Category = "Config")
	void SetGenerationParams(const FEventGenerationParams& Params);

	/**
	 * Enable/disable event generation
	 */
	UFUNCTION(BlueprintCallable, Category = "Config")
	void SetEventGenerationEnabled(bool bEnabled) { bEventGenerationEnabled = bEnabled; }

	/**
	 * Set time scale for event simulation
	 */
	UFUNCTION(BlueprintCallable, Category = "Config")
	void SetTimeScale(float Scale) { TimeScale = Scale; }

	// ============================================================================
	// EVENTS
	// ============================================================================

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnEconomicEventStarted OnEventStarted;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnEconomicEventEnded OnEventEnded;

protected:
	// Registered markets
	UPROPERTY()
	TMap<FName, UMarketDataComponent*> MarketDataComponents;

	UPROPERTY()
	TMap<FName, UPriceFluctuationSystem*> PriceSystems;

	TArray<FMarketId> RegisteredMarkets;

	// Active events
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TArray<FEconomicEvent> ActiveEvents;

	// Event history
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TArray<FEconomicEvent> EventHistory;

	// Event templates
	TMap<EEconomicEventType, FEconomicEventTemplate> EventTemplates;

	// Cooldowns per event type
	TMap<EEconomicEventType, FEventCooldown> EventCooldowns;

	// Generation parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FEventGenerationParams GenerationParams;

	// State
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bEventGenerationEnabled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float TimeScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int32 MaxHistorySize;

	// Timing
	float TimeSinceLastEventCheck;
	float EventCheckInterval;
	double LastEventTime;
	int32 NextEventId;

	// Notification state
	TArray<int32> UnreadEventIds;

private:
	void UpdateActiveEvents(float DeltaTime);
	void ExpireEvent(int32 EventIndex);
	void ApplyEventToMarkets(const FEconomicEvent& Event);
	void RemoveEventFromMarkets(const FEconomicEvent& Event);
	bool CanGenerateEventType(EEconomicEventType EventType) const;
	void UpdateEventCooldown(EEconomicEventType EventType, float Duration);
	FEconomicEvent GenerateRandomEventInternal() const;
	EEconomicEventSeverity DetermineSeverity() const;
	float CalculateSeverityMultiplier(EEconomicEventSeverity Severity) const;
	void InitializeDefaultTemplates();
	FString GetMarketKey(const FMarketId& MarketId) const;
	TArray<FMarketId> SelectRandomMarkets(int32 Count) const;
	TArray<EResourceType> SelectRandomResources(int32 Count) const;
	FString GenerateHeadline(const FEconomicEvent& Event) const;
	FString GenerateNewsBody(const FEconomicEvent& Event) const;
};
