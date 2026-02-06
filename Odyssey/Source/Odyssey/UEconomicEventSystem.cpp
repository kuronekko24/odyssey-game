// UEconomicEventSystem.cpp
// Implementation of economic event generation and management

#include "UEconomicEventSystem.h"
#include "Engine/World.h"

UEconomicEventSystem::UEconomicEventSystem()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 1.0f;

	bEventGenerationEnabled = true;
	TimeScale = 1.0f;
	MaxHistorySize = 100;
	TimeSinceLastEventCheck = 0.0f;
	EventCheckInterval = 10.0f; // Check every 10 seconds
	LastEventTime = 0.0;
	NextEventId = 1;
}

void UEconomicEventSystem::BeginPlay()
{
	Super::BeginPlay();

	InitializeDefaultTemplates();
	LastEventTime = FPlatformTime::Seconds();
}

void UEconomicEventSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	float ScaledDeltaTime = DeltaTime * TimeScale;

	// Update active events
	UpdateActiveEvents(ScaledDeltaTime);

	// Check for new event generation
	TimeSinceLastEventCheck += ScaledDeltaTime;
	if (bEventGenerationEnabled && TimeSinceLastEventCheck >= EventCheckInterval)
	{
		TryGenerateRandomEvent();
		TimeSinceLastEventCheck = 0.0f;
	}
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void UEconomicEventSystem::Initialize(const FEventGenerationParams& Params)
{
	GenerationParams = Params;
}

void UEconomicEventSystem::LoadEventTemplates(UDataTable* EventDataTable)
{
	if (!EventDataTable) return;

	// Load templates from data table
	TArray<FName> RowNames = EventDataTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		FEconomicEventTemplate* Template = EventDataTable->FindRow<FEconomicEventTemplate>(RowName, TEXT(""));
		if (Template)
		{
			EventTemplates.Add(Template->EventType, *Template);
		}
	}
}

void UEconomicEventSystem::RegisterMarket(const FMarketId& MarketId, UMarketDataComponent* MarketData, UPriceFluctuationSystem* PriceSystem)
{
	FName Key = FName(*GetMarketKey(MarketId));

	if (MarketData)
	{
		MarketDataComponents.Add(Key, MarketData);
	}
	if (PriceSystem)
	{
		PriceSystems.Add(Key, PriceSystem);
	}

	RegisteredMarkets.AddUnique(MarketId);
}

void UEconomicEventSystem::UnregisterMarket(const FMarketId& MarketId)
{
	FName Key = FName(*GetMarketKey(MarketId));

	MarketDataComponents.Remove(Key);
	PriceSystems.Remove(Key);
	RegisteredMarkets.Remove(MarketId);
}

// ============================================================================
// EVENT MANAGEMENT
// ============================================================================

int32 UEconomicEventSystem::TriggerEvent(const FEconomicEvent& Event)
{
	// Check if we're at max events
	if (ActiveEvents.Num() >= GenerationParams.MaxActiveEvents)
	{
		return -1;
	}

	FEconomicEvent NewEvent = Event;
	NewEvent.EventId = NextEventId++;
	NewEvent.Activate(FPlatformTime::Seconds());

	ActiveEvents.Add(NewEvent);
	ApplyEventToMarkets(NewEvent);

	// Add to unread notifications
	if (NewEvent.bShowNotification)
	{
		UnreadEventIds.Add(NewEvent.EventId);
	}

	// Update cooldown
	UpdateEventCooldown(NewEvent.EventType, NewEvent.Duration);

	OnEventStarted.Broadcast(NewEvent);

	return NewEvent.EventId;
}

int32 UEconomicEventSystem::TriggerEventByType(EEconomicEventType EventType, const TArray<FMarketId>& AffectedMarkets)
{
	TArray<EResourceType> DefaultResources = {
		EResourceType::Silicate,
		EResourceType::Carbon
	};

	FEconomicEvent Event = CreateEventFromTemplate(EventType, AffectedMarkets, DefaultResources);
	return TriggerEvent(Event);
}

bool UEconomicEventSystem::CancelEvent(int32 EventId)
{
	for (int32 i = 0; i < ActiveEvents.Num(); ++i)
	{
		if (ActiveEvents[i].EventId == EventId)
		{
			FEconomicEvent& Event = ActiveEvents[i];
			Event.bIsActive = false;
			RemoveEventFromMarkets(Event);
			OnEventEnded.Broadcast(Event);

			// Move to history
			EventHistory.Insert(Event, 0);
			while (EventHistory.Num() > MaxHistorySize)
			{
				EventHistory.RemoveAt(EventHistory.Num() - 1);
			}

			ActiveEvents.RemoveAt(i);
			return true;
		}
	}
	return false;
}

void UEconomicEventSystem::ForceExpireEvent(int32 EventId)
{
	CancelEvent(EventId);
}

void UEconomicEventSystem::ExtendEventDuration(int32 EventId, float AdditionalSeconds)
{
	for (FEconomicEvent& Event : ActiveEvents)
	{
		if (Event.EventId == EventId)
		{
			Event.Duration += AdditionalSeconds;
			Event.EndTime += AdditionalSeconds;
			return;
		}
	}
}

void UEconomicEventSystem::ModifyEventSeverity(int32 EventId, EEconomicEventSeverity NewSeverity)
{
	for (FEconomicEvent& Event : ActiveEvents)
	{
		if (Event.EventId == EventId)
		{
			float OldMultiplier = CalculateSeverityMultiplier(Event.Severity);
			float NewMultiplier = CalculateSeverityMultiplier(NewSeverity);
			float Ratio = NewMultiplier / OldMultiplier;

			Event.Severity = NewSeverity;
			Event.SupplyModifier = 1.0f + (Event.SupplyModifier - 1.0f) * Ratio;
			Event.DemandModifier = 1.0f + (Event.DemandModifier - 1.0f) * Ratio;
			Event.PriceModifier = 1.0f + (Event.PriceModifier - 1.0f) * Ratio;

			// Re-apply to markets
			RemoveEventFromMarkets(Event);
			ApplyEventToMarkets(Event);
			return;
		}
	}
}

// ============================================================================
// EVENT QUERIES
// ============================================================================

TArray<FEconomicEvent> UEconomicEventSystem::GetActiveEvents() const
{
	return ActiveEvents;
}

TArray<FEconomicEvent> UEconomicEventSystem::GetEventsAffectingMarket(const FMarketId& MarketId) const
{
	TArray<FEconomicEvent> Result;

	for (const FEconomicEvent& Event : ActiveEvents)
	{
		if (Event.AffectedMarkets.Contains(MarketId))
		{
			Result.Add(Event);
		}
	}

	return Result;
}

TArray<FEconomicEvent> UEconomicEventSystem::GetEventsAffectingResource(EResourceType Resource) const
{
	TArray<FEconomicEvent> Result;

	for (const FEconomicEvent& Event : ActiveEvents)
	{
		if (Event.AffectedResources.Contains(Resource))
		{
			Result.Add(Event);
		}
	}

	return Result;
}

FEconomicEvent UEconomicEventSystem::GetEvent(int32 EventId) const
{
	for (const FEconomicEvent& Event : ActiveEvents)
	{
		if (Event.EventId == EventId)
		{
			return Event;
		}
	}

	for (const FEconomicEvent& Event : EventHistory)
	{
		if (Event.EventId == EventId)
		{
			return Event;
		}
	}

	return FEconomicEvent();
}

bool UEconomicEventSystem::IsEventActive(int32 EventId) const
{
	for (const FEconomicEvent& Event : ActiveEvents)
	{
		if (Event.EventId == EventId)
		{
			return true;
		}
	}
	return false;
}

TArray<FEconomicEvent> UEconomicEventSystem::GetEventHistory(int32 MaxCount) const
{
	TArray<FEconomicEvent> Result;
	int32 Count = FMath::Min(MaxCount, EventHistory.Num());

	for (int32 i = 0; i < Count; ++i)
	{
		Result.Add(EventHistory[i]);
	}

	return Result;
}

// ============================================================================
// EVENT GENERATION
// ============================================================================

bool UEconomicEventSystem::TryGenerateRandomEvent()
{
	// Check if we're at max
	if (ActiveEvents.Num() >= GenerationParams.MaxActiveEvents)
	{
		return false;
	}

	// Check minimum time between events
	double CurrentTime = FPlatformTime::Seconds();
	if (CurrentTime - LastEventTime < GenerationParams.MinTimeBetweenEvents)
	{
		return false;
	}

	// Calculate chance based on time since last check
	float ChancePerCheck = GenerationParams.BaseEventChancePerHour * (EventCheckInterval / 3600.0f);
	float Roll = FMath::FRand();

	if (Roll > ChancePerCheck)
	{
		return false;
	}

	// Generate event
	FEconomicEvent Event = GenerateRandomEventInternal();
	int32 EventId = TriggerEvent(Event);

	if (EventId > 0)
	{
		LastEventTime = CurrentTime;
		return true;
	}

	return false;
}

int32 UEconomicEventSystem::GenerateMarketEvent(const FMarketId& MarketId)
{
	TArray<FMarketId> Markets;
	Markets.Add(MarketId);

	TArray<EResourceType> Resources = SelectRandomResources(FMath::RandRange(1, 3));

	// Pick random event type
	TArray<EEconomicEventType> Types = GetAvailableEventTypes();
	if (Types.Num() == 0) return -1;

	EEconomicEventType EventType = Types[FMath::RandRange(0, Types.Num() - 1)];

	FEconomicEvent Event = CreateEventFromTemplate(EventType, Markets, Resources);
	return TriggerEvent(Event);
}

int32 UEconomicEventSystem::GenerateResourceEvent(EResourceType Resource)
{
	TArray<FMarketId> Markets = SelectRandomMarkets(FMath::RandRange(1, 3));
	TArray<EResourceType> Resources;
	Resources.Add(Resource);

	// Pick supply/demand related event
	TArray<EEconomicEventType> Types = {
		EEconomicEventType::ResourceDiscovery,
		EEconomicEventType::ResourceDepletion,
		EEconomicEventType::DemandSurge,
		EEconomicEventType::DemandCollapse
	};

	EEconomicEventType EventType = Types[FMath::RandRange(0, Types.Num() - 1)];

	FEconomicEvent Event = CreateEventFromTemplate(EventType, Markets, Resources);
	return TriggerEvent(Event);
}

int32 UEconomicEventSystem::GenerateChainEvent(int32 TriggeringEventId)
{
	FEconomicEvent TriggeringEvent = GetEvent(TriggeringEventId);
	if (TriggeringEvent.EventId == 0) return -1;

	// Chain events based on type
	EEconomicEventType ChainType = EEconomicEventType::None;

	switch (TriggeringEvent.EventType)
	{
		case EEconomicEventType::WarDeclared:
			ChainType = EEconomicEventType::TradeRouteBlocked;
			break;
		case EEconomicEventType::ResourceDepletion:
			ChainType = EEconomicEventType::DemandSurge;
			break;
		case EEconomicEventType::TechnologyBreakthrough:
			ChainType = EEconomicEventType::ProductionBoost;
			break;
		default:
			return -1;
	}

	FEconomicEvent ChainEvent = CreateEventFromTemplate(
		ChainType,
		TriggeringEvent.AffectedMarkets,
		TriggeringEvent.AffectedResources
	);

	ChainEvent.Description = FString::Printf(
		TEXT("Following %s: %s"),
		*TriggeringEvent.EventName,
		*ChainEvent.Description
	);

	return TriggerEvent(ChainEvent);
}

// ============================================================================
// EVENT TEMPLATES
// ============================================================================

void UEconomicEventSystem::AddEventTemplate(const FEconomicEventTemplate& Template)
{
	EventTemplates.Add(Template.EventType, Template);
}

TArray<EEconomicEventType> UEconomicEventSystem::GetAvailableEventTypes() const
{
	TArray<EEconomicEventType> Types;
	EventTemplates.GetKeys(Types);
	return Types;
}

FEconomicEvent UEconomicEventSystem::CreateEventFromTemplate(
	EEconomicEventType EventType,
	const TArray<FMarketId>& Markets,
	const TArray<EResourceType>& Resources) const
{
	FEconomicEvent Event;
	Event.EventType = EventType;
	Event.AffectedMarkets = Markets;
	Event.AffectedResources = Resources;

	const FEconomicEventTemplate* Template = EventTemplates.Find(EventType);
	if (Template)
	{
		Event.EventName = Template->EventNameTemplate;
		Event.Description = Template->DescriptionTemplate;
		Event.Duration = FMath::RandRange(Template->MinDuration, Template->MaxDuration);
		Event.SupplyModifier = FMath::RandRange(Template->MinSupplyModifier, Template->MaxSupplyModifier);
		Event.DemandModifier = FMath::RandRange(Template->MinDemandModifier, Template->MaxDemandModifier);

		if (Template->NewsHeadlineVariants.Num() > 0)
		{
			Event.NewsHeadline = Template->NewsHeadlineVariants[FMath::RandRange(0, Template->NewsHeadlineVariants.Num() - 1)];
		}
		if (Template->NewsBodyVariants.Num() > 0)
		{
			Event.NewsBody = Template->NewsBodyVariants[FMath::RandRange(0, Template->NewsBodyVariants.Num() - 1)];
		}
	}
	else
	{
		// Default values
		Event.EventName = TEXT("Economic Event");
		Event.Description = TEXT("Market conditions have changed.");
		Event.Duration = 120.0f;
		Event.SupplyModifier = 1.0f;
		Event.DemandModifier = 1.0f;
	}

	Event.Severity = DetermineSeverity();
	float SeverityMult = CalculateSeverityMultiplier(Event.Severity);
	Event.SupplyModifier = 1.0f + (Event.SupplyModifier - 1.0f) * SeverityMult;
	Event.DemandModifier = 1.0f + (Event.DemandModifier - 1.0f) * SeverityMult;
	Event.PriceModifier = 1.0f + (Event.SupplyModifier - 1.0f) * -0.5f; // Inverse relationship
	Event.bShowNotification = true;

	return Event;
}

// ============================================================================
// NEWS AND NOTIFICATIONS
// ============================================================================

TArray<FString> UEconomicEventSystem::GetLatestHeadlines(int32 MaxCount) const
{
	TArray<FString> Headlines;

	for (int32 i = 0; i < FMath::Min(MaxCount, ActiveEvents.Num()); ++i)
	{
		Headlines.Add(GenerateHeadline(ActiveEvents[i]));
	}

	return Headlines;
}

FString UEconomicEventSystem::GetEventHeadline(int32 EventId) const
{
	FEconomicEvent Event = GetEvent(EventId);
	return GenerateHeadline(Event);
}

FString UEconomicEventSystem::GetEventNewsBody(int32 EventId) const
{
	FEconomicEvent Event = GetEvent(EventId);
	return GenerateNewsBody(Event);
}

bool UEconomicEventSystem::HasUnreadNotifications() const
{
	return UnreadEventIds.Num() > 0;
}

void UEconomicEventSystem::MarkNotificationsRead()
{
	UnreadEventIds.Empty();
}

// ============================================================================
// IMPACT CALCULATION
// ============================================================================

float UEconomicEventSystem::GetTotalSupplyModifier(const FMarketId& MarketId, EResourceType Resource) const
{
	float TotalModifier = 1.0f;

	for (const FEconomicEvent& Event : ActiveEvents)
	{
		if (Event.AffectedMarkets.Contains(MarketId) && Event.AffectedResources.Contains(Resource))
		{
			TotalModifier *= Event.SupplyModifier;
		}
	}

	return TotalModifier;
}

float UEconomicEventSystem::GetTotalDemandModifier(const FMarketId& MarketId, EResourceType Resource) const
{
	float TotalModifier = 1.0f;

	for (const FEconomicEvent& Event : ActiveEvents)
	{
		if (Event.AffectedMarkets.Contains(MarketId) && Event.AffectedResources.Contains(Resource))
		{
			TotalModifier *= Event.DemandModifier;
		}
	}

	return TotalModifier;
}

float UEconomicEventSystem::GetTotalPriceModifier(const FMarketId& MarketId, EResourceType Resource) const
{
	float TotalModifier = 1.0f;

	for (const FEconomicEvent& Event : ActiveEvents)
	{
		if (Event.AffectedMarkets.Contains(MarketId) && Event.AffectedResources.Contains(Resource))
		{
			TotalModifier *= Event.PriceModifier;
		}
	}

	return TotalModifier;
}

float UEconomicEventSystem::GetEventVolatilityIncrease(const FMarketId& MarketId) const
{
	float TotalIncrease = 0.0f;

	for (const FEconomicEvent& Event : ActiveEvents)
	{
		if (Event.AffectedMarkets.Contains(MarketId))
		{
			TotalIncrease += Event.VolatilityIncrease;
		}
	}

	return TotalIncrease;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void UEconomicEventSystem::SetGenerationParams(const FEventGenerationParams& Params)
{
	GenerationParams = Params;
}

// ============================================================================
// PRIVATE HELPERS
// ============================================================================

void UEconomicEventSystem::UpdateActiveEvents(float DeltaTime)
{
	double CurrentTime = FPlatformTime::Seconds();
	TArray<int32> ExpiredIndices;

	for (int32 i = 0; i < ActiveEvents.Num(); ++i)
	{
		if (ActiveEvents[i].ShouldExpire(CurrentTime))
		{
			ExpiredIndices.Add(i);
		}
	}

	// Remove expired events (reverse order to maintain indices)
	for (int32 i = ExpiredIndices.Num() - 1; i >= 0; --i)
	{
		ExpireEvent(ExpiredIndices[i]);
	}
}

void UEconomicEventSystem::ExpireEvent(int32 EventIndex)
{
	if (EventIndex < 0 || EventIndex >= ActiveEvents.Num()) return;

	FEconomicEvent& Event = ActiveEvents[EventIndex];
	Event.bIsActive = false;

	RemoveEventFromMarkets(Event);
	OnEventEnded.Broadcast(Event);

	// Move to history
	EventHistory.Insert(Event, 0);
	while (EventHistory.Num() > MaxHistorySize)
	{
		EventHistory.RemoveAt(EventHistory.Num() - 1);
	}

	ActiveEvents.RemoveAt(EventIndex);
}

void UEconomicEventSystem::ApplyEventToMarkets(const FEconomicEvent& Event)
{
	for (const FMarketId& MarketId : Event.AffectedMarkets)
	{
		FName Key = FName(*GetMarketKey(MarketId));

		// Apply to market data
		UMarketDataComponent** MarketData = MarketDataComponents.Find(Key);
		if (MarketData && *MarketData)
		{
			for (EResourceType Resource : Event.AffectedResources)
			{
				(*MarketData)->SetSupplyModifier(Resource, Event.SupplyModifier);
				(*MarketData)->SetDemandModifier(Resource, Event.DemandModifier);
			}
		}

		// Apply to price system
		UPriceFluctuationSystem** PriceSystem = PriceSystems.Find(Key);
		if (PriceSystem && *PriceSystem)
		{
			for (EResourceType Resource : Event.AffectedResources)
			{
				(*PriceSystem)->ApplyEventModifier(Resource, Event.PriceModifier, Event.Duration);
			}
		}
	}
}

void UEconomicEventSystem::RemoveEventFromMarkets(const FEconomicEvent& Event)
{
	for (const FMarketId& MarketId : Event.AffectedMarkets)
	{
		FName Key = FName(*GetMarketKey(MarketId));

		// Reset market data modifiers
		UMarketDataComponent** MarketData = MarketDataComponents.Find(Key);
		if (MarketData && *MarketData)
		{
			for (EResourceType Resource : Event.AffectedResources)
			{
				// Recalculate modifier from remaining events
				float NewSupplyMod = GetTotalSupplyModifier(MarketId, Resource) / Event.SupplyModifier;
				float NewDemandMod = GetTotalDemandModifier(MarketId, Resource) / Event.DemandModifier;
				(*MarketData)->SetSupplyModifier(Resource, NewSupplyMod);
				(*MarketData)->SetDemandModifier(Resource, NewDemandMod);
			}
		}
	}
}

bool UEconomicEventSystem::CanGenerateEventType(EEconomicEventType EventType) const
{
	const FEventCooldown* Cooldown = EventCooldowns.Find(EventType);
	if (Cooldown)
	{
		double CurrentTime = FPlatformTime::Seconds();
		return (CurrentTime - Cooldown->LastOccurrenceTime) >= Cooldown->CooldownDuration;
	}
	return true;
}

void UEconomicEventSystem::UpdateEventCooldown(EEconomicEventType EventType, float Duration)
{
	FEventCooldown& Cooldown = EventCooldowns.FindOrAdd(EventType);
	Cooldown.EventType = EventType;
	Cooldown.LastOccurrenceTime = FPlatformTime::Seconds();
	
	const FEconomicEventTemplate* Template = EventTemplates.Find(EventType);
	Cooldown.CooldownDuration = Template ? Template->MinCooldown : Duration * 0.5f;
}

FEconomicEvent UEconomicEventSystem::GenerateRandomEventInternal() const
{
	// Get available types that aren't on cooldown
	TArray<EEconomicEventType> AvailableTypes;
	for (const auto& Pair : EventTemplates)
	{
		if (CanGenerateEventType(Pair.Key))
		{
			// Skip catastrophic if disabled
			if (!GenerationParams.bAllowCatastrophicEvents)
			{
				if (Pair.Key == EEconomicEventType::MarketCrash)
				{
					continue;
				}
			}
			AvailableTypes.Add(Pair.Key);
		}
	}

	if (AvailableTypes.Num() == 0)
	{
		return FEconomicEvent();
	}

	EEconomicEventType SelectedType = AvailableTypes[FMath::RandRange(0, AvailableTypes.Num() - 1)];
	TArray<FMarketId> Markets = SelectRandomMarkets(FMath::RandRange(1, 3));
	TArray<EResourceType> Resources = SelectRandomResources(FMath::RandRange(1, 2));

	return CreateEventFromTemplate(SelectedType, Markets, Resources);
}

EEconomicEventSeverity UEconomicEventSystem::DetermineSeverity() const
{
	float Roll = FMath::FRand();

	if (Roll < 0.05f && GenerationParams.bAllowCatastrophicEvents)
	{
		return EEconomicEventSeverity::Catastrophic;
	}
	else if (Roll < 0.15f)
	{
		return EEconomicEventSeverity::Critical;
	}
	else if (Roll < 0.35f)
	{
		return EEconomicEventSeverity::Major;
	}
	else if (Roll < 0.65f)
	{
		return EEconomicEventSeverity::Moderate;
	}
	else
	{
		return EEconomicEventSeverity::Minor;
	}
}

float UEconomicEventSystem::CalculateSeverityMultiplier(EEconomicEventSeverity Severity) const
{
	switch (Severity)
	{
		case EEconomicEventSeverity::Minor: return 0.5f;
		case EEconomicEventSeverity::Moderate: return 1.0f;
		case EEconomicEventSeverity::Major: return 1.5f;
		case EEconomicEventSeverity::Critical: return 2.0f;
		case EEconomicEventSeverity::Catastrophic: return 3.0f;
		default: return 1.0f;
	}
}

void UEconomicEventSystem::InitializeDefaultTemplates()
{
	// Resource Discovery
	{
		FEconomicEventTemplate Template;
		Template.EventType = EEconomicEventType::ResourceDiscovery;
		Template.EventNameTemplate = TEXT("New Resource Deposit Found");
		Template.DescriptionTemplate = TEXT("Explorers have discovered a new resource deposit.");
		Template.MinSupplyModifier = 1.2f;
		Template.MaxSupplyModifier = 1.5f;
		Template.MinDemandModifier = 0.95f;
		Template.MaxDemandModifier = 1.05f;
		Template.MinDuration = 120.0f;
		Template.MaxDuration = 300.0f;
		Template.BaseSpawnChance = 0.15f;
		Template.MinCooldown = 60.0f;
		Template.NewsHeadlineVariants.Add(TEXT("Major Resource Discovery Boosts Supply"));
		Template.NewsHeadlineVariants.Add(TEXT("New Mining Operations Increase Production"));
		EventTemplates.Add(Template.EventType, Template);
	}

	// Resource Depletion
	{
		FEconomicEventTemplate Template;
		Template.EventType = EEconomicEventType::ResourceDepletion;
		Template.EventNameTemplate = TEXT("Resource Deposit Depleted");
		Template.DescriptionTemplate = TEXT("A major resource deposit has been exhausted.");
		Template.MinSupplyModifier = 0.5f;
		Template.MaxSupplyModifier = 0.8f;
		Template.MinDemandModifier = 1.0f;
		Template.MaxDemandModifier = 1.2f;
		Template.MinDuration = 180.0f;
		Template.MaxDuration = 600.0f;
		Template.BaseSpawnChance = 0.1f;
		Template.MinCooldown = 120.0f;
		Template.NewsHeadlineVariants.Add(TEXT("Resource Shortages Expected as Deposit Runs Dry"));
		EventTemplates.Add(Template.EventType, Template);
	}

	// Demand Surge
	{
		FEconomicEventTemplate Template;
		Template.EventType = EEconomicEventType::DemandSurge;
		Template.EventNameTemplate = TEXT("Demand Surge");
		Template.DescriptionTemplate = TEXT("Increased demand has driven up prices.");
		Template.MinSupplyModifier = 0.9f;
		Template.MaxSupplyModifier = 1.0f;
		Template.MinDemandModifier = 1.3f;
		Template.MaxDemandModifier = 1.8f;
		Template.MinDuration = 60.0f;
		Template.MaxDuration = 180.0f;
		Template.BaseSpawnChance = 0.2f;
		Template.MinCooldown = 45.0f;
		Template.NewsHeadlineVariants.Add(TEXT("Prices Surge Amid High Demand"));
		EventTemplates.Add(Template.EventType, Template);
	}

	// Trade Route Blocked
	{
		FEconomicEventTemplate Template;
		Template.EventType = EEconomicEventType::TradeRouteBlocked;
		Template.EventNameTemplate = TEXT("Trade Route Disrupted");
		Template.DescriptionTemplate = TEXT("A major trade route has been blocked.");
		Template.MinSupplyModifier = 0.6f;
		Template.MaxSupplyModifier = 0.85f;
		Template.MinDemandModifier = 1.1f;
		Template.MaxDemandModifier = 1.3f;
		Template.MinDuration = 120.0f;
		Template.MaxDuration = 360.0f;
		Template.BaseSpawnChance = 0.1f;
		Template.MinCooldown = 180.0f;
		Template.NewsHeadlineVariants.Add(TEXT("Trade Routes Blocked - Shortages Expected"));
		EventTemplates.Add(Template.EventType, Template);
	}

	// Pirate Activity
	{
		FEconomicEventTemplate Template;
		Template.EventType = EEconomicEventType::PirateActivity;
		Template.EventNameTemplate = TEXT("Pirate Activity Reported");
		Template.DescriptionTemplate = TEXT("Pirates are disrupting trade in the region.");
		Template.MinSupplyModifier = 0.7f;
		Template.MaxSupplyModifier = 0.9f;
		Template.MinDemandModifier = 1.0f;
		Template.MaxDemandModifier = 1.1f;
		Template.MinDuration = 90.0f;
		Template.MaxDuration = 240.0f;
		Template.BaseSpawnChance = 0.15f;
		Template.MinCooldown = 90.0f;
		Template.NewsHeadlineVariants.Add(TEXT("Pirate Attacks Threaten Trade"));
		EventTemplates.Add(Template.EventType, Template);
	}

	// Market Boom
	{
		FEconomicEventTemplate Template;
		Template.EventType = EEconomicEventType::MarketBoom;
		Template.EventNameTemplate = TEXT("Market Boom");
		Template.DescriptionTemplate = TEXT("Economic prosperity has increased trade activity.");
		Template.MinSupplyModifier = 1.1f;
		Template.MaxSupplyModifier = 1.3f;
		Template.MinDemandModifier = 1.2f;
		Template.MaxDemandModifier = 1.5f;
		Template.MinDuration = 180.0f;
		Template.MaxDuration = 480.0f;
		Template.BaseSpawnChance = 0.08f;
		Template.MinCooldown = 300.0f;
		Template.NewsHeadlineVariants.Add(TEXT("Economic Boom Drives Market Activity"));
		EventTemplates.Add(Template.EventType, Template);
	}
}

FString UEconomicEventSystem::GetMarketKey(const FMarketId& MarketId) const
{
	return MarketId.ToString();
}

TArray<FMarketId> UEconomicEventSystem::SelectRandomMarkets(int32 Count) const
{
	TArray<FMarketId> Result;
	TArray<FMarketId> Available = RegisteredMarkets;

	int32 NumToSelect = FMath::Min(Count, Available.Num());

	for (int32 i = 0; i < NumToSelect; ++i)
	{
		int32 Index = FMath::RandRange(0, Available.Num() - 1);
		Result.Add(Available[Index]);
		Available.RemoveAt(Index);
	}

	return Result;
}

TArray<EResourceType> UEconomicEventSystem::SelectRandomResources(int32 Count) const
{
	TArray<EResourceType> AllResources = {
		EResourceType::Silicate,
		EResourceType::Carbon,
		EResourceType::RefinedSilicate,
		EResourceType::RefinedCarbon,
		EResourceType::CompositeMaterial
	};

	TArray<EResourceType> Result;
	int32 NumToSelect = FMath::Min(Count, AllResources.Num());

	for (int32 i = 0; i < NumToSelect; ++i)
	{
		int32 Index = FMath::RandRange(0, AllResources.Num() - 1);
		Result.Add(AllResources[Index]);
		AllResources.RemoveAt(Index);
	}

	return Result;
}

FString UEconomicEventSystem::GenerateHeadline(const FEconomicEvent& Event) const
{
	if (!Event.NewsHeadline.IsEmpty())
	{
		return Event.NewsHeadline;
	}

	return FString::Printf(TEXT("[%s] %s"),
		*UEnum::GetValueAsString(Event.Severity),
		*Event.EventName);
}

FString UEconomicEventSystem::GenerateNewsBody(const FEconomicEvent& Event) const
{
	if (!Event.NewsBody.IsEmpty())
	{
		return Event.NewsBody;
	}

	FString Body = Event.Description + TEXT("\n\n");

	if (Event.SupplyModifier != 1.0f)
	{
		float SupplyChange = (Event.SupplyModifier - 1.0f) * 100.0f;
		Body += FString::Printf(TEXT("Supply Impact: %+.1f%%\n"), SupplyChange);
	}

	if (Event.DemandModifier != 1.0f)
	{
		float DemandChange = (Event.DemandModifier - 1.0f) * 100.0f;
		Body += FString::Printf(TEXT("Demand Impact: %+.1f%%\n"), DemandChange);
	}

	Body += FString::Printf(TEXT("\nDuration: %.0f seconds remaining"), 
		Event.GetRemainingDuration(FPlatformTime::Seconds()));

	return Body;
}
