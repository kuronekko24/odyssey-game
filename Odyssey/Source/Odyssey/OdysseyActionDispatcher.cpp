// OdysseyActionDispatcher.cpp
// Central event dispatcher implementation

#include "OdysseyActionDispatcher.h"
#include "OdysseyEventBus.h"
#include "OdysseyActionCommand.h"
#include "OdysseyActionButton.h"
#include "Engine/Engine.h"

// Static initialization
UOdysseyActionDispatcher* UOdysseyActionDispatcher::GlobalInstance = nullptr;

UOdysseyActionDispatcher::UOdysseyActionDispatcher()
	: bIsInitialized(false)
	, EventBus(nullptr)
	, CommandQueue(nullptr)
	, CommandHistory(nullptr)
	, ButtonManager(nullptr)
	, bLoggingEnabled(false)
{
}

UOdysseyActionDispatcher::~UOdysseyActionDispatcher()
{
	if (bIsInitialized)
	{
		Shutdown();
	}
}

UOdysseyActionDispatcher* UOdysseyActionDispatcher::Get()
{
	if (!GlobalInstance)
	{
		GlobalInstance = NewObject<UOdysseyActionDispatcher>(GetTransientPackage(), NAME_None, RF_MarkAsRootSet);
		GlobalInstance->Initialize(UOdysseyEventBus::Get());
	}
	return GlobalInstance;
}

void UOdysseyActionDispatcher::Initialize(UOdysseyEventBus* InEventBus, const FDispatcherConfig& Config)
{
	if (bIsInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("OdysseyActionDispatcher::Initialize - Already initialized"));
		return;
	}

	EventBus = InEventBus;
	Configuration = Config;

	// Create command queue
	if (Config.bEnableCommandQueue)
	{
		CommandQueue = NewObject<UOdysseyCommandQueue>(this);
		CommandQueue->Initialize(EventBus, Config.CommandQueueSize);
	}

	// Create command history
	if (Config.bEnableCommandHistory)
	{
		CommandHistory = NewObject<UOdysseyCommandHistory>(this);
		CommandHistory->Initialize(Config.CommandHistorySize);
	}

	// Subscribe to action requested events
	if (EventBus)
	{
		ActionRequestedHandle = EventBus->Subscribe(
			EOdysseyEventType::ActionRequested,
			[this](const FOdysseyEventPayload& Payload) { OnActionRequested(Payload); }
		);
	}

	bIsInitialized = true;
	bLoggingEnabled = Config.bEnableLogging;

	UE_LOG(LogTemp, Log, TEXT("OdysseyActionDispatcher::Initialize - Dispatcher initialized"));
}

void UOdysseyActionDispatcher::Shutdown()
{
	if (!bIsInitialized)
	{
		return;
	}

	// Unsubscribe from events
	if (EventBus && ActionRequestedHandle.IsValid())
	{
		EventBus->Unsubscribe(ActionRequestedHandle);
	}

	// Clear handlers
	{
		FRWScopeLock Lock(RegistryLock, SLT_Write);
		HandlerRegistry.Empty();
		HandlerNameToType.Empty();
	}

	// Clear cooldowns
	{
		FScopeLock Lock(&CooldownLock);
		ActiveCooldowns.Empty();
	}

	bIsInitialized = false;

	UE_LOG(LogTemp, Log, TEXT("OdysseyActionDispatcher::Shutdown - Dispatcher shut down"));
}

void UOdysseyActionDispatcher::Tick(float DeltaTime)
{
	if (!bIsInitialized)
	{
		return;
	}

	// Update cooldowns
	UpdateCooldowns(DeltaTime);

	// Process queued commands
	if (CommandQueue)
	{
		CommandQueue->ProcessCommands(0);	// Process all
	}
}

// ============================================================================
// Handler Registration
// ============================================================================

bool UOdysseyActionDispatcher::RegisterHandler(
	uint8 ActionType,
	FName HandlerName,
	FActionHandlerDelegate Handler,
	int32 Priority)
{
	if (!Handler.IsBound())
	{
		UE_LOG(LogTemp, Warning, TEXT("OdysseyActionDispatcher::RegisterHandler - Handler delegate not bound for %s"), *HandlerName.ToString());
		return false;
	}

	FRWScopeLock Lock(RegistryLock, SLT_Write);

	// Check if name already registered
	if (HandlerNameToType.Contains(HandlerName))
	{
		UE_LOG(LogTemp, Warning, TEXT("OdysseyActionDispatcher::RegisterHandler - Handler '%s' already registered"), *HandlerName.ToString());
		return false;
	}

	// Create handler info
	FActionHandlerInfo Info;
	Info.ActionType = ActionType;
	Info.HandlerName = HandlerName;
	Info.Handler = Handler;
	Info.Priority = Priority;
	Info.bEnabled = true;
	Info.ExecutionCount = 0;
	Info.TotalExecutionTime = 0.0;

	// Add to registry
	TArray<FActionHandlerInfo>& Handlers = HandlerRegistry.FindOrAdd(ActionType);
	Handlers.Add(Info);

	// Sort by priority (highest first)
	Handlers.Sort([](const FActionHandlerInfo& A, const FActionHandlerInfo& B)
	{
		return A.Priority > B.Priority;
	});

	// Add reverse lookup
	HandlerNameToType.Add(HandlerName, ActionType);

	// Update metrics
	{
		FScopeLock MetricLock(&MetricsLock);
		Metrics.RegisteredHandlers++;
	}

	if (bLoggingEnabled)
	{
		UE_LOG(LogTemp, Log, TEXT("OdysseyActionDispatcher: Registered handler '%s' for action type %d with priority %d"),
			*HandlerName.ToString(), ActionType, Priority);
	}

	// Publish registration event
	if (EventBus)
	{
		auto Payload = MakeShared<FOdysseyEventPayload>();
		Payload->Initialize(EOdysseyEventType::ActionRegistered);
		EventBus->PublishEvent(Payload);
	}

	return true;
}

bool UOdysseyActionDispatcher::UnregisterHandler(FName HandlerName)
{
	FRWScopeLock Lock(RegistryLock, SLT_Write);

	// Find the action type
	uint8* ActionTypePtr = HandlerNameToType.Find(HandlerName);
	if (!ActionTypePtr)
	{
		return false;
	}

	uint8 ActionType = *ActionTypePtr;

	// Remove from handlers array
	if (TArray<FActionHandlerInfo>* Handlers = HandlerRegistry.Find(ActionType))
	{
		Handlers->RemoveAll([&HandlerName](const FActionHandlerInfo& Info)
		{
			return Info.HandlerName == HandlerName;
		});
	}

	// Remove from reverse lookup
	HandlerNameToType.Remove(HandlerName);

	// Update metrics
	{
		FScopeLock MetricLock(&MetricsLock);
		Metrics.RegisteredHandlers = FMath::Max(0, Metrics.RegisteredHandlers - 1);
	}

	if (bLoggingEnabled)
	{
		UE_LOG(LogTemp, Log, TEXT("OdysseyActionDispatcher: Unregistered handler '%s'"), *HandlerName.ToString());
	}

	// Publish unregistration event
	if (EventBus)
	{
		auto Payload = MakeShared<FOdysseyEventPayload>();
		Payload->Initialize(EOdysseyEventType::ActionUnregistered);
		EventBus->PublishEvent(Payload);
	}

	return true;
}

void UOdysseyActionDispatcher::UnregisterAllHandlersForAction(uint8 ActionType)
{
	FRWScopeLock Lock(RegistryLock, SLT_Write);

	if (TArray<FActionHandlerInfo>* Handlers = HandlerRegistry.Find(ActionType))
	{
		// Remove from reverse lookup
		for (const FActionHandlerInfo& Info : *Handlers)
		{
			HandlerNameToType.Remove(Info.HandlerName);
		}

		int32 Count = Handlers->Num();
		Handlers->Empty();

		// Update metrics
		{
			FScopeLock MetricLock(&MetricsLock);
			Metrics.RegisteredHandlers = FMath::Max(0, Metrics.RegisteredHandlers - Count);
		}
	}
}

void UOdysseyActionDispatcher::SetHandlerEnabled(FName HandlerName, bool bEnabled)
{
	FRWScopeLock Lock(RegistryLock, SLT_Write);

	uint8* ActionTypePtr = HandlerNameToType.Find(HandlerName);
	if (!ActionTypePtr)
	{
		return;
	}

	if (TArray<FActionHandlerInfo>* Handlers = HandlerRegistry.Find(*ActionTypePtr))
	{
		for (FActionHandlerInfo& Info : *Handlers)
		{
			if (Info.HandlerName == HandlerName)
			{
				Info.bEnabled = bEnabled;
				break;
			}
		}
	}
}

bool UOdysseyActionDispatcher::IsHandlerRegistered(FName HandlerName) const
{
	FRWScopeLock Lock(RegistryLock, SLT_ReadOnly);
	return HandlerNameToType.Contains(HandlerName);
}

TArray<FActionHandlerInfo> UOdysseyActionDispatcher::GetRegisteredHandlers() const
{
	TArray<FActionHandlerInfo> Result;

	FRWScopeLock Lock(RegistryLock, SLT_ReadOnly);

	for (const auto& Pair : HandlerRegistry)
	{
		Result.Append(Pair.Value);
	}

	return Result;
}

// ============================================================================
// Action Dispatch
// ============================================================================

bool UOdysseyActionDispatcher::DispatchAction(uint8 ActionType, AActor* Source, AActor* Target, FVector TargetLocation)
{
	if (!bIsInitialized)
	{
		return false;
	}

	double StartTime = FPlatformTime::Seconds();

	// Create payload
	FActionEventPayload Payload;
	Payload.Initialize(EOdysseyEventType::ActionRequested, Source);
	Payload.ActionType = ActionType;
	Payload.Target = Target;
	Payload.TargetLocation = TargetLocation;
	Payload.bHasTargetLocation = !TargetLocation.IsZero();

	// Check cooldown first
	if (IsOnCooldown(ActionType))
	{
		if (bLoggingEnabled)
		{
			UE_LOG(LogTemp, Log, TEXT("OdysseyActionDispatcher: Action %d rejected - on cooldown"), ActionType);
		}

		// Publish failure event
		if (EventBus)
		{
			auto FailedPayload = MakeShared<FActionFailedEventPayload>();
			FailedPayload->Initialize(EOdysseyEventType::ActionFailed, Source);
			FailedPayload->ActionType = ActionType;
			FailedPayload->FailureReason = EActionFailureReason::OnCooldown;
			FailedPayload->RemainingCooldown = GetRemainingCooldown(ActionType);
			EventBus->PublishEvent(FailedPayload);
		}

		UpdateMetrics(FPlatformTime::Seconds() - StartTime, false);
		return false;
	}

	// Create command
	UOdysseyActionCommand* Command = CreateCommandForAction(Payload, Source);
	if (!Command)
	{
		if (bLoggingEnabled)
		{
			UE_LOG(LogTemp, Log, TEXT("OdysseyActionDispatcher: No handler found for action %d"), ActionType);
		}

		FScopeLock MetricLock(&MetricsLock);
		Metrics.ActionsRejected++;

		UpdateMetrics(FPlatformTime::Seconds() - StartTime, false);
		return false;
	}

	// Execute command
	ExecuteCommand(Command);

	UpdateMetrics(FPlatformTime::Seconds() - StartTime, Command->GetState() == ECommandState::Executed);

	return Command->GetState() == ECommandState::Executed;
}

bool UOdysseyActionDispatcher::DispatchFromPayload(const FActionEventPayload& Payload)
{
	return DispatchAction(
		Payload.ActionType,
		const_cast<AActor*>(Payload.Source.Get()),
		const_cast<AActor*>(Payload.Target.Get()),
		Payload.TargetLocation
	);
}

bool UOdysseyActionDispatcher::QueueAction(uint8 ActionType, AActor* Source, AActor* Target)
{
	if (!CommandQueue)
	{
		// No queue, dispatch immediately
		return DispatchAction(ActionType, Source, Target);
	}

	// Create payload
	FActionEventPayload Payload;
	Payload.Initialize(EOdysseyEventType::ActionRequested, Source);
	Payload.ActionType = ActionType;
	Payload.Target = Target;

	// Create command
	UOdysseyActionCommand* Command = CreateCommandForAction(Payload, Source);
	if (!Command)
	{
		return false;
	}

	return CommandQueue->Enqueue(Command);
}

int32 UOdysseyActionDispatcher::ProcessQueuedActions(int32 MaxActions)
{
	if (!CommandQueue)
	{
		return 0;
	}

	return CommandQueue->ProcessCommands(MaxActions);
}

UOdysseyActionCommand* UOdysseyActionDispatcher::CreateCommandForAction(const FActionEventPayload& Payload, AActor* Source)
{
	FRWScopeLock Lock(RegistryLock, SLT_ReadOnly);

	TArray<FActionHandlerInfo>* Handlers = HandlerRegistry.Find(Payload.ActionType);
	if (!Handlers || Handlers->Num() == 0)
	{
		return nullptr;
	}

	// Try handlers in priority order
	for (FActionHandlerInfo& Info : *Handlers)
	{
		if (!Info.bEnabled || !Info.Handler.IsBound())
		{
			continue;
		}

		double HandlerStartTime = FPlatformTime::Seconds();

		UOdysseyActionCommand* Command = Info.Handler.Execute(Payload, Source);

		double HandlerTime = FPlatformTime::Seconds() - HandlerStartTime;
		Info.ExecutionCount++;
		Info.TotalExecutionTime += HandlerTime;

		if (Command)
		{
			return Command;
		}
	}

	return nullptr;
}

void UOdysseyActionDispatcher::ExecuteCommand(UOdysseyActionCommand* Command)
{
	if (!Command)
	{
		return;
	}

	Command->SetEventBus(EventBus);
	ECommandResult Result = Command->Execute();

	if (Result == ECommandResult::Success)
	{
		// Record in history if undoable
		if (CommandHistory && Command->CanUndo())
		{
			CommandHistory->RecordCommand(Command);
		}

		FScopeLock Lock(&MetricsLock);
		Metrics.ActionsExecuted++;
	}
	else
	{
		FScopeLock Lock(&MetricsLock);
		Metrics.ActionsFailed++;
	}
}

void UOdysseyActionDispatcher::UpdateMetrics(double DispatchTime, bool bSuccess)
{
	FScopeLock Lock(&MetricsLock);

	Metrics.TotalActionsDispatched++;

	double TimeMs = DispatchTime * 1000.0;

	// Update average (exponential moving average)
	if (Metrics.AverageDispatchTimeMs == 0.0)
	{
		Metrics.AverageDispatchTimeMs = TimeMs;
	}
	else
	{
		Metrics.AverageDispatchTimeMs = Metrics.AverageDispatchTimeMs * 0.9 + TimeMs * 0.1;
	}

	if (TimeMs > Metrics.PeakDispatchTimeMs)
	{
		Metrics.PeakDispatchTimeMs = TimeMs;
	}
}

// ============================================================================
// Cooldown Management
// ============================================================================

void UOdysseyActionDispatcher::StartCooldown(uint8 ActionType, FName ActionName, float Duration, bool bNotifyOnTick)
{
	FScopeLock Lock(&CooldownLock);

	FCooldownTimer Timer;
	Timer.ActionType = ActionType;
	Timer.ActionName = ActionName;
	Timer.TotalDuration = Duration;
	Timer.RemainingTime = Duration;
	Timer.bNotifyOnTick = bNotifyOnTick;
	Timer.TimeSinceLastTick = 0.0f;

	ActiveCooldowns.Add(ActionType, Timer);

	// Update metrics
	{
		FScopeLock MetricLock(&MetricsLock);
		Metrics.ActiveCooldowns = ActiveCooldowns.Num();
	}

	// Publish cooldown started event
	if (EventBus)
	{
		EventBus->PublishCooldownEvent(EOdysseyEventType::CooldownStarted, ActionType, Duration, Duration);
	}

	if (bLoggingEnabled)
	{
		UE_LOG(LogTemp, Log, TEXT("OdysseyActionDispatcher: Started cooldown for action %d (%.2fs)"), ActionType, Duration);
	}
}

bool UOdysseyActionDispatcher::IsOnCooldown(uint8 ActionType) const
{
	FScopeLock Lock(&CooldownLock);
	return ActiveCooldowns.Contains(ActionType);
}

float UOdysseyActionDispatcher::GetRemainingCooldown(uint8 ActionType) const
{
	FScopeLock Lock(&CooldownLock);

	const FCooldownTimer* Timer = ActiveCooldowns.Find(ActionType);
	return Timer ? Timer->RemainingTime : 0.0f;
}

float UOdysseyActionDispatcher::GetCooldownProgress(uint8 ActionType) const
{
	FScopeLock Lock(&CooldownLock);

	const FCooldownTimer* Timer = ActiveCooldowns.Find(ActionType);
	if (!Timer || Timer->TotalDuration <= 0.0f)
	{
		return 1.0f;
	}

	return 1.0f - (Timer->RemainingTime / Timer->TotalDuration);
}

void UOdysseyActionDispatcher::ClearCooldown(uint8 ActionType)
{
	FScopeLock Lock(&CooldownLock);

	if (ActiveCooldowns.Remove(ActionType) > 0)
	{
		// Update metrics
		{
			FScopeLock MetricLock(&MetricsLock);
			Metrics.ActiveCooldowns = ActiveCooldowns.Num();
		}

		// Publish cooldown completed event
		if (EventBus)
		{
			EventBus->PublishCooldownEvent(EOdysseyEventType::CooldownCompleted, ActionType, 0.0f, 0.0f);
		}
	}
}

void UOdysseyActionDispatcher::ClearAllCooldowns()
{
	TArray<uint8> ActionTypes;

	{
		FScopeLock Lock(&CooldownLock);
		ActiveCooldowns.GetKeys(ActionTypes);
	}

	for (uint8 ActionType : ActionTypes)
	{
		ClearCooldown(ActionType);
	}
}

void UOdysseyActionDispatcher::UpdateCooldowns(float DeltaTime)
{
	TArray<uint8> CompletedCooldowns;

	{
		FScopeLock Lock(&CooldownLock);

		for (auto& Pair : ActiveCooldowns)
		{
			FCooldownTimer& Timer = Pair.Value;

			Timer.RemainingTime -= DeltaTime;
			Timer.TimeSinceLastTick += DeltaTime;

			// Check for tick notification
			if (Timer.bNotifyOnTick && Timer.TimeSinceLastTick >= Timer.TickInterval)
			{
				Timer.TimeSinceLastTick = 0.0f;

				// Publish tick event
				if (EventBus)
				{
					EventBus->PublishCooldownEvent(
						EOdysseyEventType::CooldownTick,
						Timer.ActionType,
						Timer.TotalDuration,
						FMath::Max(0.0f, Timer.RemainingTime)
					);
				}
			}

			// Check for completion
			if (Timer.RemainingTime <= 0.0f)
			{
				CompletedCooldowns.Add(Pair.Key);
			}
		}
	}

	// Handle completed cooldowns outside lock
	for (uint8 ActionType : CompletedCooldowns)
	{
		ClearCooldown(ActionType);

		if (bLoggingEnabled)
		{
			UE_LOG(LogTemp, Log, TEXT("OdysseyActionDispatcher: Cooldown completed for action %d"), ActionType);
		}
	}
}

// ============================================================================
// Undo/Redo
// ============================================================================

bool UOdysseyActionDispatcher::Undo()
{
	if (!CommandHistory)
	{
		return false;
	}

	return CommandHistory->Undo();
}

bool UOdysseyActionDispatcher::Redo()
{
	if (!CommandHistory)
	{
		return false;
	}

	return CommandHistory->Redo();
}

bool UOdysseyActionDispatcher::CanUndo() const
{
	return CommandHistory && CommandHistory->CanUndo();
}

bool UOdysseyActionDispatcher::CanRedo() const
{
	return CommandHistory && CommandHistory->CanRedo();
}

// ============================================================================
// Button Manager Integration
// ============================================================================

void UOdysseyActionDispatcher::SetButtonManager(UOdysseyActionButtonManager* Manager)
{
	ButtonManager = Manager;
}

void UOdysseyActionDispatcher::RegisterDefaultHandlers()
{
	// Interact handler
	RegisterHandler(
		static_cast<uint8>(EActionButtonType::Interact),
		FName("DefaultInteract"),
		FActionHandlerDelegate::CreateLambda([this](const FActionEventPayload& Payload, AActor* Source) -> UOdysseyActionCommand*
		{
			return UInteractCommand::Create(Source, Payload.Target.Get(), EventBus);
		}),
		0
	);

	// Cargo handler
	RegisterHandler(
		static_cast<uint8>(EActionButtonType::Cargo),
		FName("DefaultCargo"),
		FActionHandlerDelegate::CreateLambda([this](const FActionEventPayload& Payload, AActor* Source) -> UOdysseyActionCommand*
		{
			return UOpenCargoCommand::Create(Source, EventBus);
		}),
		0
	);

	// Scout mode handler
	RegisterHandler(
		static_cast<uint8>(EActionButtonType::Scout),
		FName("DefaultScout"),
		FActionHandlerDelegate::CreateLambda([this](const FActionEventPayload& Payload, AActor* Source) -> UOdysseyActionCommand*
		{
			UScoutModeCommand* Command = UScoutModeCommand::Create(Source, EventBus);
			Command->SetButtonManager(ButtonManager);
			return Command;
		}),
		0
	);

	// Attack mode handler
	RegisterHandler(
		static_cast<uint8>(EActionButtonType::Attack),
		FName("DefaultAttack"),
		FActionHandlerDelegate::CreateLambda([this](const FActionEventPayload& Payload, AActor* Source) -> UOdysseyActionCommand*
		{
			UAttackModeCommand* Command = UAttackModeCommand::Create(Source, EventBus);
			Command->SetButtonManager(ButtonManager);
			return Command;
		}),
		0
	);

	// Special attack handler
	RegisterHandler(
		static_cast<uint8>(EActionButtonType::SpecialAttack),
		FName("DefaultSpecialAttack"),
		FActionHandlerDelegate::CreateLambda([this](const FActionEventPayload& Payload, AActor* Source) -> UOdysseyActionCommand*
		{
			return USpecialAttackCommand::Create(Source, Payload.Target.Get(), EventBus);
		}),
		0
	);

	// Thruster boost handler
	RegisterHandler(
		static_cast<uint8>(EActionButtonType::ThrusterBoost),
		FName("DefaultThrusterBoost"),
		FActionHandlerDelegate::CreateLambda([this](const FActionEventPayload& Payload, AActor* Source) -> UOdysseyActionCommand*
		{
			UThrusterBoostCommand* Command = UThrusterBoostCommand::Create(Source, EventBus);
			Command->SetButtonManager(ButtonManager);
			return Command;
		}),
		0
	);

	UE_LOG(LogTemp, Log, TEXT("OdysseyActionDispatcher: Registered %d default handlers"), 6);
}

// ============================================================================
// Metrics
// ============================================================================

FDispatcherMetrics UOdysseyActionDispatcher::GetMetrics() const
{
	FScopeLock Lock(&MetricsLock);
	return Metrics;
}

void UOdysseyActionDispatcher::ResetMetrics()
{
	FScopeLock Lock(&MetricsLock);
	int32 HandlerCount = Metrics.RegisteredHandlers;
	int32 CooldownCount = Metrics.ActiveCooldowns;
	Metrics.Reset();
	Metrics.RegisteredHandlers = HandlerCount;
	Metrics.ActiveCooldowns = CooldownCount;
}

void UOdysseyActionDispatcher::SetLoggingEnabled(bool bEnabled)
{
	bLoggingEnabled = bEnabled;
}

// ============================================================================
// Event Handlers
// ============================================================================

void UOdysseyActionDispatcher::OnActionRequested(const FOdysseyEventPayload& Payload)
{
	// This is called when an action is requested via the event bus
	// We can use this to intercept and process actions

	const FActionEventPayload* ActionPayload = static_cast<const FActionEventPayload*>(&Payload);
	if (ActionPayload)
	{
		// Note: We don't dispatch here to avoid infinite recursion
		// This handler is for monitoring/logging purposes

		if (bLoggingEnabled)
		{
			UE_LOG(LogTemp, Verbose, TEXT("OdysseyActionDispatcher: ActionRequested event received for action %d"),
				ActionPayload->ActionType);
		}
	}
}
