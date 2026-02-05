// OdysseyEventBus.cpp
// Thread-safe event bus implementation

#include "OdysseyEventBus.h"
#include "Engine/Engine.h"
#include "Async/Async.h"

// Static initialization
UOdysseyEventBus* UOdysseyEventBus::GlobalInstance = nullptr;
TAtomic<uint64> FOdysseyEventId::NextEventId(1);

FOdysseyEventId FOdysseyEventId::Generate()
{
	FOdysseyEventId NewId;
	NewId.Id = ++NextEventId;
	NewId.Timestamp = FPlatformTime::Seconds();
	return NewId;
}

UOdysseyEventBus::UOdysseyEventBus()
	: bIsInitialized(false)
	, bIsShuttingDown(false)
	, MaxQueueSize(256)
	, SequenceCounter(0)
	, NextHandleId(1)
	, bLoggingEnabled(false)
	, bRecordingEnabled(false)
{
}

UOdysseyEventBus::~UOdysseyEventBus()
{
	if (bIsInitialized)
	{
		Shutdown();
	}
}

UOdysseyEventBus* UOdysseyEventBus::Get()
{
	if (!GlobalInstance)
	{
		GlobalInstance = NewObject<UOdysseyEventBus>(GetTransientPackage(), NAME_None, RF_MarkAsRootSet);
		GlobalInstance->Initialize();
	}
	return GlobalInstance;
}

void UOdysseyEventBus::Initialize(int32 InMaxQueueSize, int32 PreAllocatedEvents)
{
	if (bIsInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("OdysseyEventBus::Initialize - Already initialized"));
		return;
	}

	MaxQueueSize = InMaxQueueSize;

	// Pre-allocate event queue
	EventQueue.Reserve(MaxQueueSize);

	// Create event pools
	ActionEventPool = MakeUnique<TOdysseyEventPool<FActionEventPayload>>(PreAllocatedEvents);
	EnergyEventPool = MakeUnique<TOdysseyEventPool<FEnergyEventPayload>>(PreAllocatedEvents / 2);
	CooldownEventPool = MakeUnique<TOdysseyEventPool<FCooldownEventPayload>>(PreAllocatedEvents);

	bIsInitialized = true;
	bIsShuttingDown = false;

	// Publish initialization event
	auto InitEvent = MakeShared<FOdysseyEventPayload>();
	InitEvent->Initialize(EOdysseyEventType::EventBusInitialized, nullptr, EOdysseyEventPriority::Critical);
	PublishImmediate(InitEvent);

	UE_LOG(LogTemp, Log, TEXT("OdysseyEventBus::Initialize - Initialized with queue size %d, pre-allocated %d events"),
		MaxQueueSize, PreAllocatedEvents);
}

void UOdysseyEventBus::Shutdown()
{
	if (!bIsInitialized)
	{
		return;
	}

	bIsShuttingDown = true;

	// Publish shutdown event
	auto ShutdownEvent = MakeShared<FOdysseyEventPayload>();
	ShutdownEvent->Initialize(EOdysseyEventType::EventBusShutdown, nullptr, EOdysseyEventPriority::Critical);
	PublishImmediate(ShutdownEvent);

	// Clear queue
	{
		FScopeLock Lock(&QueueLock);
		EventQueue.Empty();
	}

	// Clear subscribers
	{
		FRWScopeLock Lock(SubscribersLock, SLT_Write);
		Subscribers.Empty();
	}

	// Reset pools
	ActionEventPool.Reset();
	EnergyEventPool.Reset();
	CooldownEventPool.Reset();

	// Clear recordings
	{
		FScopeLock Lock(&RecordingLock);
		RecordedEvents.Empty();
	}

	bIsInitialized = false;

	UE_LOG(LogTemp, Log, TEXT("OdysseyEventBus::Shutdown - Event bus shut down"));
}

void UOdysseyEventBus::ProcessEvents(int32 MaxEventsToProcess, float MaxProcessingTimeMs)
{
	if (!bIsInitialized || bIsShuttingDown)
	{
		return;
	}

	double StartTime = FPlatformTime::Seconds();
	double MaxTime = MaxProcessingTimeMs > 0.0f ? StartTime + (MaxProcessingTimeMs / 1000.0) : DBL_MAX;

	int32 EventsProcessed = 0;
	int32 MaxEvents = MaxEventsToProcess > 0 ? MaxEventsToProcess : INT32_MAX;

	while (EventsProcessed < MaxEvents && FPlatformTime::Seconds() < MaxTime)
	{
		TSharedPtr<FOdysseyEventPayload> Payload = DequeueEvent();
		if (!Payload.IsValid())
		{
			break;
		}

		DispatchEvent(Payload);
		EventsProcessed++;
	}

	// Update metrics
	if (EventsProcessed > 0)
	{
		double ProcessingTime = (FPlatformTime::Seconds() - StartTime) * 1000.0;

		FScopeLock Lock(&MetricsLock);
		Metrics.TotalEventsProcessed += EventsProcessed;

		// Update average processing time (exponential moving average)
		if (Metrics.AverageProcessingTimeMs == 0.0)
		{
			Metrics.AverageProcessingTimeMs = ProcessingTime / EventsProcessed;
		}
		else
		{
			Metrics.AverageProcessingTimeMs = Metrics.AverageProcessingTimeMs * 0.9 + (ProcessingTime / EventsProcessed) * 0.1;
		}

		if (ProcessingTime > Metrics.PeakProcessingTimeMs)
		{
			Metrics.PeakProcessingTimeMs = ProcessingTime;
		}
	}

	// Periodic cleanup of invalid subscribers
	static int32 CleanupCounter = 0;
	if (++CleanupCounter >= 100)	// Every 100 process calls
	{
		CleanupInvalidSubscribers();
		CleanupCounter = 0;
	}
}

bool UOdysseyEventBus::PublishEvent(TSharedPtr<FOdysseyEventPayload> Payload)
{
	if (!bIsInitialized || bIsShuttingDown || !Payload.IsValid())
	{
		return false;
	}

	// Immediate priority bypasses queue
	if (Payload->Priority == EOdysseyEventPriority::Immediate)
	{
		PublishImmediate(Payload);
		return true;
	}

	EnqueueEvent(Payload);
	return true;
}

void UOdysseyEventBus::PublishImmediate(TSharedPtr<FOdysseyEventPayload> Payload)
{
	if (!Payload.IsValid())
	{
		return;
	}

	// Process immediately on calling thread
	DispatchEvent(Payload);

	FScopeLock Lock(&MetricsLock);
	Metrics.TotalEventsPublished++;
	Metrics.TotalEventsProcessed++;
}

void UOdysseyEventBus::EnqueueEvent(TSharedPtr<FOdysseyEventPayload> Payload)
{
	FScopeLock Lock(&QueueLock);

	// Check queue capacity
	if (EventQueue.Num() >= MaxQueueSize)
	{
		// Drop lowest priority event if queue is full
		if (bLoggingEnabled)
		{
			UE_LOG(LogTemp, Warning, TEXT("OdysseyEventBus: Queue full, dropping event type %d"),
				static_cast<int32>(Payload->EventType));
		}

		FScopeLock MetricLock(&MetricsLock);
		Metrics.EventsDropped++;
		return;
	}

	FQueuedEvent QueuedEvent;
	QueuedEvent.Payload = Payload;
	QueuedEvent.QueueTime = FPlatformTime::Seconds();
	QueuedEvent.SequenceNumber = ++SequenceCounter;

	// Add to heap (priority queue)
	EventQueue.HeapPush(QueuedEvent);

	// Update metrics
	{
		FScopeLock MetricLock(&MetricsLock);
		Metrics.TotalEventsPublished++;
		Metrics.CurrentQueueDepth = EventQueue.Num();
		if (Metrics.CurrentQueueDepth > Metrics.PeakQueueDepth)
		{
			Metrics.PeakQueueDepth = Metrics.CurrentQueueDepth;
		}
	}

	if (bLoggingEnabled)
	{
		UE_LOG(LogTemp, Verbose, TEXT("OdysseyEventBus: Enqueued event %s (type %d, priority %d)"),
			*Payload->EventId.ToString(),
			static_cast<int32>(Payload->EventType),
			static_cast<int32>(Payload->Priority));
	}
}

TSharedPtr<FOdysseyEventPayload> UOdysseyEventBus::DequeueEvent()
{
	FScopeLock Lock(&QueueLock);

	if (EventQueue.Num() == 0)
	{
		return nullptr;
	}

	FQueuedEvent QueuedEvent;
	EventQueue.HeapPop(QueuedEvent);

	// Update metrics
	{
		FScopeLock MetricLock(&MetricsLock);
		Metrics.CurrentQueueDepth = EventQueue.Num();
	}

	return QueuedEvent.Payload;
}

void UOdysseyEventBus::DispatchEvent(const TSharedPtr<FOdysseyEventPayload>& Payload)
{
	if (!Payload.IsValid() || Payload->IsConsumed())
	{
		return;
	}

	// Record if enabled
	if (bRecordingEnabled)
	{
		FScopeLock Lock(&RecordingLock);
		RecordedEvents.Add(*Payload);
	}

	// Log if enabled
	if (bLoggingEnabled)
	{
		UE_LOG(LogTemp, Log, TEXT("OdysseyEventBus: Dispatching event %s (type %d)"),
			*Payload->EventId.ToString(),
			static_cast<int32>(Payload->EventType));
	}

	// Notify C++ subscribers
	NotifySubscribers(Payload);

	// Broadcast to Blueprint delegates
	BroadcastToBlueprintDelegates(Payload);
}

void UOdysseyEventBus::NotifySubscribers(const TSharedPtr<FOdysseyEventPayload>& Payload)
{
	TArray<FOdysseyEventSubscriber> SubscribersToNotify;
	TArray<uint64> HandlesToRemove;

	// Gather subscribers under read lock
	{
		FRWScopeLock Lock(SubscribersLock, SLT_ReadOnly);

		if (TArray<FOdysseyEventSubscriber>* TypeSubscribers = Subscribers.Find(Payload->EventType))
		{
			for (const FOdysseyEventSubscriber& Sub : *TypeSubscribers)
			{
				// Check if subscriber is still valid
				if (!Sub.Subscriber.IsValid())
				{
					HandlesToRemove.Add(Sub.HandleId);
					continue;
				}

				// Check filter
				if (!Sub.Filter.Matches(*Payload))
				{
					continue;
				}

				SubscribersToNotify.Add(Sub);

				// Mark for removal if once-only
				if (Sub.bOnceOnly)
				{
					HandlesToRemove.Add(Sub.HandleId);
				}
			}
		}
	}

	// Sort by priority (higher first)
	SubscribersToNotify.Sort([](const FOdysseyEventSubscriber& A, const FOdysseyEventSubscriber& B)
	{
		return A.Priority > B.Priority;
	});

	// Notify subscribers (outside lock to prevent deadlocks)
	for (const FOdysseyEventSubscriber& Sub : SubscribersToNotify)
	{
		if (Payload->IsConsumed() && Payload->bCancellable)
		{
			break;	// Event was consumed, stop propagation
		}

		if (Sub.Callback)
		{
			Sub.Callback(*Payload);
		}
	}

	// Remove invalid/once-only subscribers
	if (HandlesToRemove.Num() > 0)
	{
		FRWScopeLock Lock(SubscribersLock, SLT_Write);

		if (TArray<FOdysseyEventSubscriber>* TypeSubscribers = Subscribers.Find(Payload->EventType))
		{
			TypeSubscribers->RemoveAll([&HandlesToRemove](const FOdysseyEventSubscriber& Sub)
			{
				return HandlesToRemove.Contains(Sub.HandleId);
			});
		}
	}
}

void UOdysseyEventBus::BroadcastToBlueprintDelegates(const TSharedPtr<FOdysseyEventPayload>& Payload)
{
	switch (Payload->EventType)
	{
		case EOdysseyEventType::ActionRequested:
		case EOdysseyEventType::ActionExecuted:
		{
			if (const FActionEventPayload* ActionPayload = static_cast<const FActionEventPayload*>(Payload.Get()))
			{
				if (Payload->EventType == EOdysseyEventType::ActionRequested)
				{
					OnActionRequested.Broadcast(*ActionPayload);
				}
				else
				{
					OnActionExecuted.Broadcast(*ActionPayload);
				}
			}
			break;
		}

		case EOdysseyEventType::ActionFailed:
		{
			if (const FActionFailedEventPayload* FailedPayload = static_cast<const FActionFailedEventPayload*>(Payload.Get()))
			{
				OnActionFailed.Broadcast(*FailedPayload);
			}
			break;
		}

		case EOdysseyEventType::CooldownStarted:
		case EOdysseyEventType::CooldownCompleted:
		{
			if (const FCooldownEventPayload* CooldownPayload = static_cast<const FCooldownEventPayload*>(Payload.Get()))
			{
				if (Payload->EventType == EOdysseyEventType::CooldownStarted)
				{
					OnCooldownStarted.Broadcast(*CooldownPayload);
				}
				else
				{
					OnCooldownCompleted.Broadcast(*CooldownPayload);
				}
			}
			break;
		}

		case EOdysseyEventType::EnergyChanged:
		case EOdysseyEventType::EnergyDepleted:
		case EOdysseyEventType::EnergyRestored:
		{
			if (const FEnergyEventPayload* EnergyPayload = static_cast<const FEnergyEventPayload*>(Payload.Get()))
			{
				OnEnergyChanged.Broadcast(*EnergyPayload);
			}
			break;
		}

		case EOdysseyEventType::AbilityActivated:
		case EOdysseyEventType::AbilityDeactivated:
		case EOdysseyEventType::AbilityToggled:
		{
			if (const FAbilityEventPayload* AbilityPayload = static_cast<const FAbilityEventPayload*>(Payload.Get()))
			{
				OnAbilityStateChanged.Broadcast(*AbilityPayload);
			}
			break;
		}

		default:
			break;
	}
}

// ============================================================================
// Blueprint-friendly publishing methods
// ============================================================================

bool UOdysseyEventBus::PublishActionEvent(uint8 ActionType, FName ActionName, AActor* Source, int32 EnergyCost)
{
	auto Payload = MakeShared<FActionEventPayload>();
	Payload->Initialize(EOdysseyEventType::ActionRequested, Source);
	Payload->ActionType = ActionType;
	Payload->ActionName = ActionName;
	Payload->EnergyCost = EnergyCost;

	return PublishEvent(Payload);
}

bool UOdysseyEventBus::PublishEnergyEvent(float PreviousEnergy, float CurrentEnergy, float MaxEnergy, FName Reason)
{
	auto Payload = MakeShared<FEnergyEventPayload>();

	EOdysseyEventType EventType = EOdysseyEventType::EnergyChanged;
	if (CurrentEnergy <= 0.0f && PreviousEnergy > 0.0f)
	{
		EventType = EOdysseyEventType::EnergyDepleted;
	}
	else if (CurrentEnergy >= MaxEnergy && PreviousEnergy < MaxEnergy)
	{
		EventType = EOdysseyEventType::EnergyRestored;
	}

	Payload->Initialize(EventType);
	Payload->PreviousEnergy = PreviousEnergy;
	Payload->CurrentEnergy = CurrentEnergy;
	Payload->MaxEnergy = MaxEnergy;
	Payload->DeltaEnergy = CurrentEnergy - PreviousEnergy;
	Payload->ChangeReason = Reason;

	return PublishEvent(Payload);
}

bool UOdysseyEventBus::PublishCooldownEvent(EOdysseyEventType EventType, uint8 ActionType, float TotalDuration, float RemainingTime)
{
	auto Payload = MakeShared<FCooldownEventPayload>();
	Payload->Initialize(EventType);
	Payload->ActionType = ActionType;
	Payload->TotalDuration = TotalDuration;
	Payload->RemainingTime = RemainingTime;
	Payload->Progress = TotalDuration > 0.0f ? 1.0f - (RemainingTime / TotalDuration) : 1.0f;

	return PublishEvent(Payload);
}

bool UOdysseyEventBus::PublishAbilityEvent(EOdysseyEventType EventType, uint8 AbilityType, FName AbilityName, bool bIsActive, float Duration)
{
	auto Payload = MakeShared<FAbilityEventPayload>();
	Payload->Initialize(EventType);
	Payload->AbilityType = AbilityType;
	Payload->AbilityName = AbilityName;
	Payload->bIsActive = bIsActive;
	Payload->Duration = Duration;
	Payload->RemainingTime = Duration;

	return PublishEvent(Payload);
}

// ============================================================================
// Subscription management
// ============================================================================

FOdysseyEventHandle UOdysseyEventBus::Subscribe(
	EOdysseyEventType EventType,
	TFunction<void(const FOdysseyEventPayload&)> Callback,
	const FOdysseyEventFilter& Filter,
	int32 Priority)
{
	if (!Callback)
	{
		return FOdysseyEventHandle();
	}

	FOdysseyEventSubscriber Subscriber;
	Subscriber.HandleId = GenerateHandleId();
	Subscriber.Callback = Callback;
	Subscriber.Filter = Filter;
	Subscriber.Priority = Priority;
	Subscriber.bOnceOnly = false;

	{
		FRWScopeLock Lock(SubscribersLock, SLT_Write);
		Subscribers.FindOrAdd(EventType).Add(Subscriber);
	}

	{
		FScopeLock Lock(&MetricsLock);
		Metrics.ActiveSubscribers++;
	}

	if (bLoggingEnabled)
	{
		UE_LOG(LogTemp, Verbose, TEXT("OdysseyEventBus: New subscriber %llu for event type %d"),
			Subscriber.HandleId, static_cast<int32>(EventType));
	}

	return FOdysseyEventHandle(Subscriber.HandleId, EventType);
}

TArray<FOdysseyEventHandle> UOdysseyEventBus::SubscribeMultiple(
	const TArray<EOdysseyEventType>& EventTypes,
	TFunction<void(const FOdysseyEventPayload&)> Callback,
	const FOdysseyEventFilter& Filter,
	int32 Priority)
{
	TArray<FOdysseyEventHandle> Handles;
	Handles.Reserve(EventTypes.Num());

	for (EOdysseyEventType EventType : EventTypes)
	{
		Handles.Add(Subscribe(EventType, Callback, Filter, Priority));
	}

	return Handles;
}

FOdysseyEventHandle UOdysseyEventBus::SubscribeOnce(
	EOdysseyEventType EventType,
	TFunction<void(const FOdysseyEventPayload&)> Callback,
	const FOdysseyEventFilter& Filter)
{
	if (!Callback)
	{
		return FOdysseyEventHandle();
	}

	FOdysseyEventSubscriber Subscriber;
	Subscriber.HandleId = GenerateHandleId();
	Subscriber.Callback = Callback;
	Subscriber.Filter = Filter;
	Subscriber.Priority = 0;
	Subscriber.bOnceOnly = true;

	{
		FRWScopeLock Lock(SubscribersLock, SLT_Write);
		Subscribers.FindOrAdd(EventType).Add(Subscriber);
	}

	{
		FScopeLock Lock(&MetricsLock);
		Metrics.ActiveSubscribers++;
	}

	return FOdysseyEventHandle(Subscriber.HandleId, EventType);
}

bool UOdysseyEventBus::Unsubscribe(FOdysseyEventHandle& Handle)
{
	if (!Handle.IsValid())
	{
		return false;
	}

	bool bRemoved = false;

	{
		FRWScopeLock Lock(SubscribersLock, SLT_Write);

		if (TArray<FOdysseyEventSubscriber>* TypeSubscribers = Subscribers.Find(Handle.EventType))
		{
			int32 RemovedCount = TypeSubscribers->RemoveAll([&Handle](const FOdysseyEventSubscriber& Sub)
			{
				return Sub.HandleId == Handle.HandleId;
			});

			bRemoved = RemovedCount > 0;
		}
	}

	if (bRemoved)
	{
		FScopeLock Lock(&MetricsLock);
		Metrics.ActiveSubscribers = FMath::Max(0, Metrics.ActiveSubscribers - 1);
	}

	Handle.Reset();
	return bRemoved;
}

void UOdysseyEventBus::UnsubscribeAll(UObject* Subscriber)
{
	if (!Subscriber)
	{
		return;
	}

	int32 TotalRemoved = 0;

	{
		FRWScopeLock Lock(SubscribersLock, SLT_Write);

		for (auto& Pair : Subscribers)
		{
			int32 RemovedCount = Pair.Value.RemoveAll([Subscriber](const FOdysseyEventSubscriber& Sub)
			{
				return Sub.Subscriber.Get() == Subscriber;
			});

			TotalRemoved += RemovedCount;
		}
	}

	if (TotalRemoved > 0)
	{
		FScopeLock Lock(&MetricsLock);
		Metrics.ActiveSubscribers = FMath::Max(0, Metrics.ActiveSubscribers - TotalRemoved);
	}
}

uint64 UOdysseyEventBus::GenerateHandleId()
{
	return ++NextHandleId;
}

void UOdysseyEventBus::CleanupInvalidSubscribers()
{
	int32 TotalRemoved = 0;

	{
		FRWScopeLock Lock(SubscribersLock, SLT_Write);

		for (auto& Pair : Subscribers)
		{
			int32 RemovedCount = Pair.Value.RemoveAll([](const FOdysseyEventSubscriber& Sub)
			{
				return !Sub.Subscriber.IsValid() && Sub.Subscriber.IsExplicitlyNull();
			});

			TotalRemoved += RemovedCount;
		}
	}

	if (TotalRemoved > 0)
	{
		FScopeLock Lock(&MetricsLock);
		Metrics.ActiveSubscribers = FMath::Max(0, Metrics.ActiveSubscribers - TotalRemoved);

		if (bLoggingEnabled)
		{
			UE_LOG(LogTemp, Verbose, TEXT("OdysseyEventBus: Cleaned up %d invalid subscribers"), TotalRemoved);
		}
	}
}

// ============================================================================
// Query and metrics
// ============================================================================

int32 UOdysseyEventBus::GetQueueDepth() const
{
	FScopeLock Lock(&QueueLock);
	return EventQueue.Num();
}

FOdysseyEventMetrics UOdysseyEventBus::GetMetrics() const
{
	FScopeLock Lock(&MetricsLock);
	return Metrics;
}

void UOdysseyEventBus::ResetMetrics()
{
	FScopeLock Lock(&MetricsLock);
	int32 CurrentSubscribers = Metrics.ActiveSubscribers;
	Metrics.Reset();
	Metrics.ActiveSubscribers = CurrentSubscribers;
}

// ============================================================================
// Debug and logging
// ============================================================================

void UOdysseyEventBus::SetEventRecordingEnabled(bool bEnabled)
{
	bRecordingEnabled = bEnabled;

	if (!bEnabled)
	{
		ClearRecordedEvents();
	}
}

TArray<FOdysseyEventPayload> UOdysseyEventBus::GetRecordedEvents() const
{
	FScopeLock Lock(&RecordingLock);
	return RecordedEvents;
}

void UOdysseyEventBus::ClearRecordedEvents()
{
	FScopeLock Lock(&RecordingLock);
	RecordedEvents.Empty();
}

void UOdysseyEventBus::ReplayRecordedEvents(float TimeScale)
{
	TArray<FOdysseyEventPayload> EventsToReplay;

	{
		FScopeLock Lock(&RecordingLock);
		EventsToReplay = RecordedEvents;
	}

	if (EventsToReplay.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("OdysseyEventBus::ReplayRecordedEvents - No events to replay"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("OdysseyEventBus::ReplayRecordedEvents - Replaying %d events at %.2fx speed"),
		EventsToReplay.Num(), TimeScale);

	// Sort by creation time
	EventsToReplay.Sort([](const FOdysseyEventPayload& A, const FOdysseyEventPayload& B)
	{
		return A.CreationTime < B.CreationTime;
	});

	// Replay with timing (simplified - immediate replay)
	for (const FOdysseyEventPayload& Event : EventsToReplay)
	{
		auto ReplayPayload = MakeShared<FOdysseyEventPayload>(Event);
		ReplayPayload->EventId = FOdysseyEventId::Generate();	// New ID for replay
		ReplayPayload->bConsumed = false;

		PublishEvent(ReplayPayload);
	}
}
