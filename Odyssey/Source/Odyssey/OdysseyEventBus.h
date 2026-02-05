// OdysseyEventBus.h
// Thread-safe event bus for the Odyssey action system
// Provides lock-free event queuing and processing for mobile multi-touch scenarios

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HAL/ThreadSafeBool.h"
#include "Containers/LockFreeList.h"
#include "Misc/SpinLock.h"
#include "OdysseyActionEvent.h"
#include "OdysseyEventBus.generated.h"

// Forward declarations
class UOdysseyActionDispatcher;

/**
 * Event pool entry for pre-allocated events
 * Eliminates runtime allocations during gameplay
 */
template<typename T>
class TOdysseyEventPool
{
public:
	TOdysseyEventPool(int32 InitialSize = 64)
		: PoolSize(InitialSize)
	{
		PreAllocate(InitialSize);
	}

	~TOdysseyEventPool()
	{
		FScopeLock Lock(&PoolLock);
		for (T* Item : FreePool)
		{
			delete Item;
		}
		FreePool.Empty();
	}

	T* Acquire()
	{
		FScopeLock Lock(&PoolLock);

		if (FreePool.Num() == 0)
		{
			// Pool exhausted, allocate more (double the size)
			PreAllocate(PoolSize);
			PoolSize *= 2;
		}

		T* Item = FreePool.Pop();
		return Item;
	}

	void Release(T* Item)
	{
		if (Item)
		{
			// Reset the item before returning to pool
			*Item = T();

			FScopeLock Lock(&PoolLock);
			FreePool.Add(Item);
		}
	}

	int32 GetFreeCount() const
	{
		FScopeLock Lock(&PoolLock);
		return FreePool.Num();
	}

private:
	void PreAllocate(int32 Count)
	{
		// Note: PoolLock should already be held by caller
		FreePool.Reserve(FreePool.Num() + Count);
		for (int32 i = 0; i < Count; ++i)
		{
			FreePool.Add(new T());
		}
	}

	TArray<T*> FreePool;
	mutable FCriticalSection PoolLock;
	int32 PoolSize;
};

/**
 * Queued event wrapper with priority for sorting
 */
USTRUCT()
struct FQueuedEvent
{
	GENERATED_BODY()

	TSharedPtr<FOdysseyEventPayload> Payload;
	double QueueTime;
	int32 SequenceNumber;	// For stable sorting within same priority

	FQueuedEvent()
		: QueueTime(0.0)
		, SequenceNumber(0)
	{
	}

	// Priority comparison for heap operations
	bool operator<(const FQueuedEvent& Other) const
	{
		if (!Payload.IsValid() || !Other.Payload.IsValid())
		{
			return false;
		}

		// Higher priority first
		if (Payload->Priority != Other.Payload->Priority)
		{
			return static_cast<uint8>(Payload->Priority) > static_cast<uint8>(Other.Payload->Priority);
		}

		// Same priority: earlier sequence number first (FIFO within priority)
		return SequenceNumber < Other.SequenceNumber;
	}
};

/**
 * Subscriber entry for event routing
 */
struct FOdysseyEventSubscriber
{
	uint64 HandleId;
	TWeakObjectPtr<UObject> Subscriber;
	TFunction<void(const FOdysseyEventPayload&)> Callback;
	FOdysseyEventFilter Filter;
	int32 Priority;		// Subscriber priority (higher = called first)
	bool bOnceOnly;		// Auto-unsubscribe after first invocation

	FOdysseyEventSubscriber()
		: HandleId(0)
		, Priority(0)
		, bOnceOnly(false)
	{
	}
};

/**
 * Thread-safe event bus for Odyssey action system
 *
 * Features:
 * - Lock-free event queuing for mobile threading
 * - Pre-allocated event pools for zero runtime allocations
 * - Priority-based event processing
 * - Multicast subscription with filtering
 * - Event logging and replay for debugging
 */
UCLASS(BlueprintType)
class ODYSSEY_API UOdysseyEventBus : public UObject
{
	GENERATED_BODY()

public:
	UOdysseyEventBus();
	virtual ~UOdysseyEventBus();

	/**
	 * Initialize the event bus
	 * @param MaxQueueSize Maximum events in queue before dropping
	 * @param PreAllocatedEvents Number of events to pre-allocate
	 */
	UFUNCTION(BlueprintCallable, Category = "Event Bus")
	void Initialize(int32 MaxQueueSize = 256, int32 PreAllocatedEvents = 64);

	/**
	 * Shutdown the event bus and cleanup
	 */
	UFUNCTION(BlueprintCallable, Category = "Event Bus")
	void Shutdown();

	/**
	 * Process queued events (call from game thread tick)
	 * @param MaxEventsToProcess Limit events processed per frame (0 = all)
	 * @param MaxProcessingTimeMs Time budget for processing (0 = no limit)
	 */
	UFUNCTION(BlueprintCallable, Category = "Event Bus")
	void ProcessEvents(int32 MaxEventsToProcess = 0, float MaxProcessingTimeMs = 0.0f);

	// ============================================================================
	// Event Publishing
	// ============================================================================

	/**
	 * Publish an event to the bus (thread-safe)
	 * @param Payload Event data
	 * @return True if event was queued successfully
	 */
	bool PublishEvent(TSharedPtr<FOdysseyEventPayload> Payload);

	/**
	 * Publish an immediate event (bypasses queue, processes synchronously)
	 * Warning: Only use for Critical/Immediate priority events
	 */
	void PublishImmediate(TSharedPtr<FOdysseyEventPayload> Payload);

	/**
	 * Blueprint-friendly event publishing
	 */
	UFUNCTION(BlueprintCallable, Category = "Event Bus")
	bool PublishActionEvent(uint8 ActionType, FName ActionName, AActor* Source, int32 EnergyCost);

	UFUNCTION(BlueprintCallable, Category = "Event Bus")
	bool PublishEnergyEvent(float PreviousEnergy, float CurrentEnergy, float MaxEnergy, FName Reason);

	UFUNCTION(BlueprintCallable, Category = "Event Bus")
	bool PublishCooldownEvent(EOdysseyEventType EventType, uint8 ActionType, float TotalDuration, float RemainingTime);

	UFUNCTION(BlueprintCallable, Category = "Event Bus")
	bool PublishAbilityEvent(EOdysseyEventType EventType, uint8 AbilityType, FName AbilityName, bool bIsActive, float Duration);

	// ============================================================================
	// Event Subscription
	// ============================================================================

	/**
	 * Subscribe to events of a specific type
	 * @param EventType Type of events to receive
	 * @param Callback Function to call when event is received
	 * @param Filter Optional filter for selective subscription
	 * @param Priority Subscriber priority (higher = called first)
	 * @return Handle for unsubscription
	 */
	FOdysseyEventHandle Subscribe(
		EOdysseyEventType EventType,
		TFunction<void(const FOdysseyEventPayload&)> Callback,
		const FOdysseyEventFilter& Filter = FOdysseyEventFilter(),
		int32 Priority = 0
	);

	/**
	 * Subscribe to multiple event types with same callback
	 */
	TArray<FOdysseyEventHandle> SubscribeMultiple(
		const TArray<EOdysseyEventType>& EventTypes,
		TFunction<void(const FOdysseyEventPayload&)> Callback,
		const FOdysseyEventFilter& Filter = FOdysseyEventFilter(),
		int32 Priority = 0
	);

	/**
	 * Subscribe for a single event only (auto-unsubscribe after)
	 */
	FOdysseyEventHandle SubscribeOnce(
		EOdysseyEventType EventType,
		TFunction<void(const FOdysseyEventPayload&)> Callback,
		const FOdysseyEventFilter& Filter = FOdysseyEventFilter()
	);

	/**
	 * Unsubscribe using handle
	 */
	UFUNCTION(BlueprintCallable, Category = "Event Bus")
	bool Unsubscribe(UPARAM(ref) FOdysseyEventHandle& Handle);

	/**
	 * Unsubscribe all subscriptions for an object
	 */
	void UnsubscribeAll(UObject* Subscriber);

	// ============================================================================
	// Blueprint Event Delegates
	// ============================================================================

	UPROPERTY(BlueprintAssignable, Category = "Event Bus|Delegates")
	FActionEventDelegate OnActionRequested;

	UPROPERTY(BlueprintAssignable, Category = "Event Bus|Delegates")
	FActionEventDelegate OnActionExecuted;

	UPROPERTY(BlueprintAssignable, Category = "Event Bus|Delegates")
	FActionFailedEventDelegate OnActionFailed;

	UPROPERTY(BlueprintAssignable, Category = "Event Bus|Delegates")
	FCooldownEventDelegate OnCooldownStarted;

	UPROPERTY(BlueprintAssignable, Category = "Event Bus|Delegates")
	FCooldownEventDelegate OnCooldownCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Event Bus|Delegates")
	FEnergyEventDelegate OnEnergyChanged;

	UPROPERTY(BlueprintAssignable, Category = "Event Bus|Delegates")
	FAbilityEventDelegate OnAbilityStateChanged;

	// ============================================================================
	// Query and Metrics
	// ============================================================================

	UFUNCTION(BlueprintCallable, Category = "Event Bus")
	int32 GetQueueDepth() const;

	UFUNCTION(BlueprintCallable, Category = "Event Bus")
	bool IsInitialized() const { return bIsInitialized; }

	UFUNCTION(BlueprintCallable, Category = "Event Bus")
	FOdysseyEventMetrics GetMetrics() const;

	UFUNCTION(BlueprintCallable, Category = "Event Bus")
	void ResetMetrics();

	// ============================================================================
	// Debug and Logging
	// ============================================================================

	UFUNCTION(BlueprintCallable, Category = "Event Bus|Debug")
	void SetLoggingEnabled(bool bEnabled) { bLoggingEnabled = bEnabled; }

	UFUNCTION(BlueprintCallable, Category = "Event Bus|Debug")
	void SetEventRecordingEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Event Bus|Debug")
	TArray<FOdysseyEventPayload> GetRecordedEvents() const;

	UFUNCTION(BlueprintCallable, Category = "Event Bus|Debug")
	void ClearRecordedEvents();

	/**
	 * Replay recorded events (useful for testing/debugging)
	 */
	UFUNCTION(BlueprintCallable, Category = "Event Bus|Debug")
	void ReplayRecordedEvents(float TimeScale = 1.0f);

	// ============================================================================
	// Singleton Access
	// ============================================================================

	/**
	 * Get the global event bus instance
	 */
	static UOdysseyEventBus* Get();

private:
	// Queue management
	void EnqueueEvent(TSharedPtr<FOdysseyEventPayload> Payload);
	TSharedPtr<FOdysseyEventPayload> DequeueEvent();
	void DispatchEvent(const TSharedPtr<FOdysseyEventPayload>& Payload);
	void NotifySubscribers(const TSharedPtr<FOdysseyEventPayload>& Payload);
	void BroadcastToBlueprintDelegates(const TSharedPtr<FOdysseyEventPayload>& Payload);

	// Subscription management
	uint64 GenerateHandleId();
	void CleanupInvalidSubscribers();

	// State
	bool bIsInitialized;
	FThreadSafeBool bIsShuttingDown;
	int32 MaxQueueSize;

	// Event queue (priority queue implemented as heap)
	TArray<FQueuedEvent> EventQueue;
	mutable FCriticalSection QueueLock;
	TAtomic<int32> SequenceCounter;

	// Subscriber registry: EventType -> Array of subscribers
	TMap<EOdysseyEventType, TArray<FOdysseyEventSubscriber>> Subscribers;
	mutable FRWLock SubscribersLock;
	TAtomic<uint64> NextHandleId;

	// Event pools for common event types
	TUniquePtr<TOdysseyEventPool<FActionEventPayload>> ActionEventPool;
	TUniquePtr<TOdysseyEventPool<FEnergyEventPayload>> EnergyEventPool;
	TUniquePtr<TOdysseyEventPool<FCooldownEventPayload>> CooldownEventPool;

	// Metrics
	FOdysseyEventMetrics Metrics;
	mutable FCriticalSection MetricsLock;

	// Debug
	bool bLoggingEnabled;
	bool bRecordingEnabled;
	TArray<FOdysseyEventPayload> RecordedEvents;
	mutable FCriticalSection RecordingLock;

	// Singleton
	static UOdysseyEventBus* GlobalInstance;
};
