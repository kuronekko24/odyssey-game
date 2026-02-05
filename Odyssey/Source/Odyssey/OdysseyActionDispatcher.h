// OdysseyActionDispatcher.h
// Central event dispatcher for Odyssey action system
// Provides O(1) action lookup via hash map and extensible action registration

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "OdysseyActionEvent.h"
#include "OdysseyActionDispatcher.generated.h"

// Forward declarations
class UOdysseyEventBus;
class UOdysseyActionCommand;
class UOdysseyCommandQueue;
class UOdysseyCommandHistory;
class UOdysseyActionButtonManager;

/**
 * Action handler function signature
 * Returns a command to execute, or nullptr to reject the action
 */
DECLARE_DELEGATE_RetVal_TwoParams(UOdysseyActionCommand*, FActionHandlerDelegate, const FActionEventPayload&, AActor*);

/**
 * Action handler registration info
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FActionHandlerInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Handler Info")
	uint8 ActionType;

	UPROPERTY(BlueprintReadOnly, Category = "Handler Info")
	FName HandlerName;

	UPROPERTY(BlueprintReadOnly, Category = "Handler Info")
	int32 Priority;	// Higher priority handlers are tried first

	UPROPERTY(BlueprintReadOnly, Category = "Handler Info")
	bool bEnabled;

	UPROPERTY(BlueprintReadOnly, Category = "Handler Info")
	int64 ExecutionCount;

	UPROPERTY(BlueprintReadOnly, Category = "Handler Info")
	double TotalExecutionTime;

	FActionHandlerDelegate Handler;

	FActionHandlerInfo()
		: ActionType(0)
		, HandlerName(NAME_None)
		, Priority(0)
		, bEnabled(true)
		, ExecutionCount(0)
		, TotalExecutionTime(0.0)
	{
	}

	double GetAverageExecutionTime() const
	{
		return ExecutionCount > 0 ? TotalExecutionTime / ExecutionCount : 0.0;
	}
};

/**
 * Dispatcher configuration
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FDispatcherConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int32 CommandQueueSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int32 CommandHistorySize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bEnableCommandQueue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bEnableCommandHistory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bEnablePerformanceMetrics;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bEnableLogging;

	FDispatcherConfig()
		: CommandQueueSize(32)
		, CommandHistorySize(50)
		, bEnableCommandQueue(true)
		, bEnableCommandHistory(true)
		, bEnablePerformanceMetrics(true)
		, bEnableLogging(false)
	{
	}
};

/**
 * Dispatcher performance metrics
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FDispatcherMetrics
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	int64 TotalActionsDispatched;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	int64 ActionsExecuted;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	int64 ActionsFailed;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	int64 ActionsRejected;	// No handler found

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	double AverageDispatchTimeMs;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	double PeakDispatchTimeMs;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	int32 RegisteredHandlers;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	int32 ActiveCooldowns;

	FDispatcherMetrics()
		: TotalActionsDispatched(0)
		, ActionsExecuted(0)
		, ActionsFailed(0)
		, ActionsRejected(0)
		, AverageDispatchTimeMs(0.0)
		, PeakDispatchTimeMs(0.0)
		, RegisteredHandlers(0)
		, ActiveCooldowns(0)
	{
	}

	void Reset()
	{
		TotalActionsDispatched = 0;
		ActionsExecuted = 0;
		ActionsFailed = 0;
		ActionsRejected = 0;
		AverageDispatchTimeMs = 0.0;
		PeakDispatchTimeMs = 0.0;
		// Don't reset RegisteredHandlers or ActiveCooldowns
	}
};

/**
 * Cooldown timer entry for event-driven cooldowns
 */
USTRUCT()
struct FCooldownTimer
{
	GENERATED_BODY()

	uint8 ActionType;
	FName ActionName;
	float TotalDuration;
	float RemainingTime;
	bool bNotifyOnTick;		// Whether to publish tick events
	float TickInterval;		// Interval between tick events
	float TimeSinceLastTick;

	FCooldownTimer()
		: ActionType(0)
		, ActionName(NAME_None)
		, TotalDuration(0.0f)
		, RemainingTime(0.0f)
		, bNotifyOnTick(false)
		, TickInterval(0.1f)
		, TimeSinceLastTick(0.0f)
	{
	}
};

/**
 * Central action dispatcher for the Odyssey event-driven action system
 *
 * Key features:
 * - O(1) action lookup via hash map
 * - Dynamic action handler registration
 * - Event-driven cooldown timers
 * - Command queue for thread-safe action processing
 * - Command history for undo/redo
 * - Performance metrics and debugging
 */
UCLASS(BlueprintType)
class ODYSSEY_API UOdysseyActionDispatcher : public UObject
{
	GENERATED_BODY()

public:
	UOdysseyActionDispatcher();
	virtual ~UOdysseyActionDispatcher();

	// ============================================================================
	// Initialization
	// ============================================================================

	/**
	 * Initialize the dispatcher
	 * @param InEventBus Event bus for publishing events
	 * @param Config Dispatcher configuration
	 */
	UFUNCTION(BlueprintCallable, Category = "Dispatcher")
	void Initialize(UOdysseyEventBus* InEventBus, const FDispatcherConfig& Config = FDispatcherConfig());

	/**
	 * Shutdown and cleanup
	 */
	UFUNCTION(BlueprintCallable, Category = "Dispatcher")
	void Shutdown();

	/**
	 * Tick the dispatcher (update cooldowns, process queue)
	 * @param DeltaTime Time since last tick
	 */
	UFUNCTION(BlueprintCallable, Category = "Dispatcher")
	void Tick(float DeltaTime);

	// ============================================================================
	// Handler Registration
	// ============================================================================

	/**
	 * Register an action handler
	 * @param ActionType Type of action to handle
	 * @param HandlerName Unique name for this handler
	 * @param Handler Delegate that creates/returns a command for the action
	 * @param Priority Handler priority (higher = tried first)
	 * @return True if registration was successful
	 */
	bool RegisterHandler(
		uint8 ActionType,
		FName HandlerName,
		FActionHandlerDelegate Handler,
		int32 Priority = 0
	);

	/**
	 * Register a simple handler using lambda
	 */
	template<typename TCommand>
	bool RegisterSimpleHandler(uint8 ActionType, FName HandlerName, int32 Priority = 0)
	{
		return RegisterHandler(ActionType, HandlerName,
			FActionHandlerDelegate::CreateLambda([this](const FActionEventPayload& Payload, AActor* Source) -> UOdysseyActionCommand*
			{
				TCommand* Command = NewObject<TCommand>();
				Command->SetSource(Source);
				Command->SetEventBus(EventBus);
				return Command;
			}),
			Priority);
	}

	/**
	 * Unregister a handler by name
	 */
	UFUNCTION(BlueprintCallable, Category = "Dispatcher")
	bool UnregisterHandler(FName HandlerName);

	/**
	 * Unregister all handlers for an action type
	 */
	UFUNCTION(BlueprintCallable, Category = "Dispatcher")
	void UnregisterAllHandlersForAction(uint8 ActionType);

	/**
	 * Enable/disable a handler
	 */
	UFUNCTION(BlueprintCallable, Category = "Dispatcher")
	void SetHandlerEnabled(FName HandlerName, bool bEnabled);

	/**
	 * Check if a handler is registered
	 */
	UFUNCTION(BlueprintCallable, Category = "Dispatcher")
	bool IsHandlerRegistered(FName HandlerName) const;

	/**
	 * Get all registered handler info
	 */
	UFUNCTION(BlueprintCallable, Category = "Dispatcher")
	TArray<FActionHandlerInfo> GetRegisteredHandlers() const;

	// ============================================================================
	// Action Dispatch
	// ============================================================================

	/**
	 * Dispatch an action request
	 * @param ActionType Type of action to dispatch
	 * @param Source Actor requesting the action
	 * @param Target Optional target actor
	 * @param TargetLocation Optional target location
	 * @return True if action was dispatched successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "Dispatcher")
	bool DispatchAction(uint8 ActionType, AActor* Source, AActor* Target = nullptr, FVector TargetLocation = FVector::ZeroVector);

	/**
	 * Dispatch an action from an event payload
	 */
	bool DispatchFromPayload(const FActionEventPayload& Payload);

	/**
	 * Queue an action for later processing (thread-safe)
	 */
	UFUNCTION(BlueprintCallable, Category = "Dispatcher")
	bool QueueAction(uint8 ActionType, AActor* Source, AActor* Target = nullptr);

	/**
	 * Process queued actions
	 * @param MaxActions Maximum actions to process (0 = all)
	 * @return Number of actions processed
	 */
	UFUNCTION(BlueprintCallable, Category = "Dispatcher")
	int32 ProcessQueuedActions(int32 MaxActions = 0);

	// ============================================================================
	// Cooldown Management
	// ============================================================================

	/**
	 * Start a cooldown for an action
	 */
	UFUNCTION(BlueprintCallable, Category = "Cooldown")
	void StartCooldown(uint8 ActionType, FName ActionName, float Duration, bool bNotifyOnTick = false);

	/**
	 * Check if an action is on cooldown
	 */
	UFUNCTION(BlueprintCallable, Category = "Cooldown")
	bool IsOnCooldown(uint8 ActionType) const;

	/**
	 * Get remaining cooldown time for an action
	 */
	UFUNCTION(BlueprintCallable, Category = "Cooldown")
	float GetRemainingCooldown(uint8 ActionType) const;

	/**
	 * Get cooldown progress (0.0 = just started, 1.0 = complete)
	 */
	UFUNCTION(BlueprintCallable, Category = "Cooldown")
	float GetCooldownProgress(uint8 ActionType) const;

	/**
	 * Clear a cooldown immediately
	 */
	UFUNCTION(BlueprintCallable, Category = "Cooldown")
	void ClearCooldown(uint8 ActionType);

	/**
	 * Clear all cooldowns
	 */
	UFUNCTION(BlueprintCallable, Category = "Cooldown")
	void ClearAllCooldowns();

	// ============================================================================
	// Undo/Redo
	// ============================================================================

	/**
	 * Undo the last action
	 */
	UFUNCTION(BlueprintCallable, Category = "Undo")
	bool Undo();

	/**
	 * Redo the last undone action
	 */
	UFUNCTION(BlueprintCallable, Category = "Undo")
	bool Redo();

	/**
	 * Check if undo is available
	 */
	UFUNCTION(BlueprintCallable, Category = "Undo")
	bool CanUndo() const;

	/**
	 * Check if redo is available
	 */
	UFUNCTION(BlueprintCallable, Category = "Undo")
	bool CanRedo() const;

	// ============================================================================
	// Button Manager Integration
	// ============================================================================

	/**
	 * Set the button manager for cooldown and energy integration
	 */
	UFUNCTION(BlueprintCallable, Category = "Integration")
	void SetButtonManager(UOdysseyActionButtonManager* Manager);

	/**
	 * Register default handlers for all standard action types
	 */
	UFUNCTION(BlueprintCallable, Category = "Integration")
	void RegisterDefaultHandlers();

	// ============================================================================
	// Metrics and Debug
	// ============================================================================

	UFUNCTION(BlueprintCallable, Category = "Metrics")
	FDispatcherMetrics GetMetrics() const;

	UFUNCTION(BlueprintCallable, Category = "Metrics")
	void ResetMetrics();

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void SetLoggingEnabled(bool bEnabled);

	// ============================================================================
	// Singleton Access
	// ============================================================================

	static UOdysseyActionDispatcher* Get();

private:
	// Internal dispatch
	UOdysseyActionCommand* CreateCommandForAction(const FActionEventPayload& Payload, AActor* Source);
	void ExecuteCommand(UOdysseyActionCommand* Command);
	void UpdateMetrics(double DispatchTime, bool bSuccess);

	// Cooldown updates
	void UpdateCooldowns(float DeltaTime);

	// Event handlers
	void OnActionRequested(const FOdysseyEventPayload& Payload);

	// State
	bool bIsInitialized;
	FDispatcherConfig Configuration;

	// Event bus
	UPROPERTY()
	UOdysseyEventBus* EventBus;

	// Command processing
	UPROPERTY()
	UOdysseyCommandQueue* CommandQueue;

	UPROPERTY()
	UOdysseyCommandHistory* CommandHistory;

	// Button manager for integration
	UPROPERTY()
	UOdysseyActionButtonManager* ButtonManager;

	// Handler registry: ActionType -> Array of handlers (sorted by priority)
	TMap<uint8, TArray<FActionHandlerInfo>> HandlerRegistry;
	TMap<FName, uint8> HandlerNameToType;	// Reverse lookup for unregistration
	mutable FRWLock RegistryLock;

	// Active cooldowns: ActionType -> CooldownTimer
	TMap<uint8, FCooldownTimer> ActiveCooldowns;
	mutable FCriticalSection CooldownLock;

	// Metrics
	FDispatcherMetrics Metrics;
	mutable FCriticalSection MetricsLock;

	// Debug
	bool bLoggingEnabled;

	// Event subscription handles
	FOdysseyEventHandle ActionRequestedHandle;

	// Singleton
	static UOdysseyActionDispatcher* GlobalInstance;
};
