// OdysseyActionCommand.cpp
// Command pattern implementation

#include "OdysseyActionCommand.h"
#include "OdysseyEventBus.h"
#include "OdysseyActionButton.h"
#include "OdysseyCharacter.h"
#include "Engine/Engine.h"

// ============================================================================
// UOdysseyActionCommand - Base Command
// ============================================================================

UOdysseyActionCommand::UOdysseyActionCommand()
	: CommandName(TEXT("BaseCommand"))
	, ActionType(0)
	, State(ECommandState::Created)
	, bCanUndo(false)
	, ExecutionTime(0.0)
	, EnergyCost(0)
	, CooldownDuration(0.0f)
	, EventBus(nullptr)
{
}

ECommandResult UOdysseyActionCommand::Execute()
{
	// Validate first
	EActionFailureReason FailureReason;
	State = ECommandState::Validating;

	if (!Validate(FailureReason))
	{
		State = ECommandState::Failed;
		PublishActionEvent(EOdysseyEventType::ActionFailed, FailureReason);
		return ECommandResult::Failed;
	}

	// Execute
	State = ECommandState::Executing;
	double StartTime = FPlatformTime::Seconds();

	ECommandResult Result = ExecuteInternal();

	ExecutionTime = FPlatformTime::Seconds() - StartTime;

	if (Result == ECommandResult::Success)
	{
		State = ECommandState::Executed;
		PublishActionEvent(EOdysseyEventType::ActionExecuted);
	}
	else if (Result == ECommandResult::Failed)
	{
		State = ECommandState::Failed;
		PublishActionEvent(EOdysseyEventType::ActionFailed, EActionFailureReason::Custom);
	}
	else if (Result == ECommandResult::Cancelled)
	{
		State = ECommandState::Cancelled;
		PublishActionEvent(EOdysseyEventType::ActionCancelled);
	}

	return Result;
}

bool UOdysseyActionCommand::Undo()
{
	if (!bCanUndo || State != ECommandState::Executed)
	{
		return false;
	}

	State = ECommandState::Undoing;

	if (UndoInternal())
	{
		State = ECommandState::Undone;
		return true;
	}

	State = ECommandState::Executed;	// Revert state
	return false;
}

ECommandResult UOdysseyActionCommand::Redo()
{
	if (State != ECommandState::Undone)
	{
		return ECommandResult::Failed;
	}

	return Execute();
}

bool UOdysseyActionCommand::Validate(EActionFailureReason& OutFailureReason)
{
	OutFailureReason = EActionFailureReason::None;
	return ValidateInternal(OutFailureReason);
}

void UOdysseyActionCommand::Cancel()
{
	if (State == ECommandState::Executing || State == ECommandState::Validating)
	{
		State = ECommandState::Cancelled;
		PublishActionEvent(EOdysseyEventType::ActionCancelled);
	}
}

void UOdysseyActionCommand::PublishActionEvent(EOdysseyEventType EventType, EActionFailureReason FailureReason)
{
	if (!EventBus)
	{
		return;
	}

	if (EventType == EOdysseyEventType::ActionFailed)
	{
		auto Payload = MakeShared<FActionFailedEventPayload>();
		Payload->Initialize(EventType, SourceActor.Get());
		Payload->ActionType = ActionType;
		Payload->ActionName = FName(*CommandName);
		Payload->EnergyCost = EnergyCost;
		Payload->CooldownDuration = CooldownDuration;
		Payload->FailureReason = FailureReason;

		EventBus->PublishEvent(Payload);
	}
	else
	{
		auto Payload = MakeShared<FActionEventPayload>();
		Payload->Initialize(EventType, SourceActor.Get());
		Payload->ActionType = ActionType;
		Payload->ActionName = FName(*CommandName);
		Payload->EnergyCost = EnergyCost;
		Payload->CooldownDuration = CooldownDuration;
		Payload->Target = TargetActor;

		EventBus->PublishEvent(Payload);
	}
}

// ============================================================================
// UOdysseyCommandQueue
// ============================================================================

UOdysseyCommandQueue::UOdysseyCommandQueue()
	: EventBus(nullptr)
	, MaxQueueSize(32)
{
}

void UOdysseyCommandQueue::Initialize(UOdysseyEventBus* InEventBus, int32 MaxSize)
{
	EventBus = InEventBus;
	MaxQueueSize = MaxSize;
	PendingCommands.Reserve(MaxSize);
}

bool UOdysseyCommandQueue::Enqueue(UOdysseyActionCommand* Command)
{
	if (!Command)
	{
		return false;
	}

	FScopeLock Lock(&QueueLock);

	if (PendingCommands.Num() >= MaxQueueSize)
	{
		UE_LOG(LogTemp, Warning, TEXT("OdysseyCommandQueue: Queue full, dropping command %s"), *Command->GetCommandName());
		return false;
	}

	Command->SetEventBus(EventBus);
	PendingCommands.Add(Command);

	return true;
}

int32 UOdysseyCommandQueue::ProcessCommands(int32 MaxCommands)
{
	TArray<UOdysseyActionCommand*> CommandsToProcess;

	{
		FScopeLock Lock(&QueueLock);

		int32 NumToProcess = MaxCommands > 0 ? FMath::Min(MaxCommands, PendingCommands.Num()) : PendingCommands.Num();

		for (int32 i = 0; i < NumToProcess; ++i)
		{
			CommandsToProcess.Add(PendingCommands[i]);
		}

		PendingCommands.RemoveAt(0, NumToProcess);
	}

	// Execute outside lock
	int32 Processed = 0;
	for (UOdysseyActionCommand* Command : CommandsToProcess)
	{
		if (Command)
		{
			Command->Execute();
			Processed++;
		}
	}

	return Processed;
}

void UOdysseyCommandQueue::Clear()
{
	FScopeLock Lock(&QueueLock);
	PendingCommands.Empty();
}

int32 UOdysseyCommandQueue::GetPendingCount() const
{
	FScopeLock Lock(&QueueLock);
	return PendingCommands.Num();
}

bool UOdysseyCommandQueue::IsFull() const
{
	FScopeLock Lock(&QueueLock);
	return PendingCommands.Num() >= MaxQueueSize;
}

// ============================================================================
// UOdysseyCommandHistory
// ============================================================================

UOdysseyCommandHistory::UOdysseyCommandHistory()
	: CurrentIndex(-1)
	, MaxSize(50)
{
}

void UOdysseyCommandHistory::Initialize(int32 MaxHistorySize)
{
	MaxSize = MaxHistorySize;
	History.Reserve(MaxSize);
}

void UOdysseyCommandHistory::RecordCommand(UOdysseyActionCommand* Command)
{
	if (!Command || !Command->CanUndo())
	{
		return;
	}

	// Clear any redo history
	if (CurrentIndex < History.Num() - 1)
	{
		History.RemoveAt(CurrentIndex + 1, History.Num() - CurrentIndex - 1);
	}

	// Add new command
	History.Add(Command);
	CurrentIndex = History.Num() - 1;

	// Trim if exceeding max size
	if (History.Num() > MaxSize)
	{
		History.RemoveAt(0);
		CurrentIndex--;
	}
}

bool UOdysseyCommandHistory::Undo()
{
	if (!CanUndo())
	{
		return false;
	}

	UOdysseyActionCommand* Command = History[CurrentIndex];
	if (Command && Command->Undo())
	{
		CurrentIndex--;
		return true;
	}

	return false;
}

bool UOdysseyCommandHistory::Redo()
{
	if (!CanRedo())
	{
		return false;
	}

	UOdysseyActionCommand* Command = History[CurrentIndex + 1];
	if (Command && Command->Redo() == ECommandResult::Success)
	{
		CurrentIndex++;
		return true;
	}

	return false;
}

bool UOdysseyCommandHistory::CanUndo() const
{
	return CurrentIndex >= 0 && History.Num() > 0;
}

bool UOdysseyCommandHistory::CanRedo() const
{
	return CurrentIndex < History.Num() - 1;
}

void UOdysseyCommandHistory::Clear()
{
	History.Empty();
	CurrentIndex = -1;
}

// ============================================================================
// Concrete Command Implementations
// ============================================================================

// --- UInteractCommand ---

UInteractCommand::UInteractCommand()
{
	CommandName = TEXT("Interact");
	ActionType = static_cast<uint8>(EActionButtonType::Interact);
	bCanUndo = false;
	EnergyCost = 5;
	CooldownDuration = 0.5f;
}

UInteractCommand* UInteractCommand::Create(AActor* Source, AActor* Target, UOdysseyEventBus* InEventBus)
{
	UInteractCommand* Command = NewObject<UInteractCommand>();
	Command->SetSource(Source);
	Command->SetTarget(Target);
	Command->SetEventBus(InEventBus);
	return Command;
}

ECommandResult UInteractCommand::ExecuteInternal()
{
	if (AOdysseyCharacter* Character = Cast<AOdysseyCharacter>(SourceActor.Get()))
	{
		Character->TryInteract();
		UE_LOG(LogTemp, Log, TEXT("InteractCommand: Executed interaction"));
		return ECommandResult::Success;
	}

	return ECommandResult::Failed;
}

bool UInteractCommand::ValidateInternal(EActionFailureReason& OutFailureReason)
{
	if (!SourceActor.IsValid())
	{
		OutFailureReason = EActionFailureReason::Custom;
		return false;
	}

	return true;
}

// --- UOpenCargoCommand ---

UOpenCargoCommand::UOpenCargoCommand()
	: bWasCargoOpen(false)
{
	CommandName = TEXT("OpenCargo");
	ActionType = static_cast<uint8>(EActionButtonType::Cargo);
	bCanUndo = true;
	EnergyCost = 0;
	CooldownDuration = 1.0f;
}

UOpenCargoCommand* UOpenCargoCommand::Create(AActor* Source, UOdysseyEventBus* InEventBus)
{
	UOpenCargoCommand* Command = NewObject<UOpenCargoCommand>();
	Command->SetSource(Source);
	Command->SetEventBus(InEventBus);
	return Command;
}

ECommandResult UOpenCargoCommand::ExecuteInternal()
{
	// Store previous state for undo
	bWasCargoOpen = false;	// Would query actual cargo state here

	UE_LOG(LogTemp, Log, TEXT("OpenCargoCommand: Opening cargo bay"));

	// Publish interaction event
	if (EventBus)
	{
		auto Payload = MakeShared<FInteractionEventPayload>();
		Payload->Initialize(EOdysseyEventType::InteractionStarted, SourceActor.Get());
		Payload->InteractionType = FName("OpenCargo");
		EventBus->PublishEvent(Payload);
	}

	return ECommandResult::Success;
}

bool UOpenCargoCommand::UndoInternal()
{
	UE_LOG(LogTemp, Log, TEXT("OpenCargoCommand: Closing cargo bay (undo)"));
	return true;
}

// --- UScoutModeCommand ---

UScoutModeCommand::UScoutModeCommand()
	: ButtonManager(nullptr)
	, bPreviousState(false)
{
	CommandName = TEXT("ScoutMode");
	ActionType = static_cast<uint8>(EActionButtonType::Scout);
	bCanUndo = true;
	EnergyCost = 15;
	CooldownDuration = 2.0f;
}

UScoutModeCommand* UScoutModeCommand::Create(AActor* Source, UOdysseyEventBus* InEventBus)
{
	UScoutModeCommand* Command = NewObject<UScoutModeCommand>();
	Command->SetSource(Source);
	Command->SetEventBus(InEventBus);
	return Command;
}

ECommandResult UScoutModeCommand::ExecuteInternal()
{
	// Store previous state
	bPreviousState = false;	// Would query from ButtonManager

	UE_LOG(LogTemp, Log, TEXT("ScoutModeCommand: Toggling scout mode"));

	// Publish ability event
	if (EventBus)
	{
		EventBus->PublishAbilityEvent(
			EOdysseyEventType::AbilityToggled,
			ActionType,
			FName("ScoutMode"),
			!bPreviousState,
			0.0f	// No duration for toggle
		);
	}

	return ECommandResult::Success;
}

bool UScoutModeCommand::UndoInternal()
{
	UE_LOG(LogTemp, Log, TEXT("ScoutModeCommand: Reverting scout mode to %s"), bPreviousState ? TEXT("ON") : TEXT("OFF"));

	if (EventBus)
	{
		EventBus->PublishAbilityEvent(
			EOdysseyEventType::AbilityToggled,
			ActionType,
			FName("ScoutMode"),
			bPreviousState,
			0.0f
		);
	}

	return true;
}

// --- UAttackModeCommand ---

UAttackModeCommand::UAttackModeCommand()
	: ButtonManager(nullptr)
	, bPreviousState(false)
{
	CommandName = TEXT("AttackMode");
	ActionType = static_cast<uint8>(EActionButtonType::Attack);
	bCanUndo = true;
	EnergyCost = 20;
	CooldownDuration = 1.5f;
}

UAttackModeCommand* UAttackModeCommand::Create(AActor* Source, UOdysseyEventBus* InEventBus)
{
	UAttackModeCommand* Command = NewObject<UAttackModeCommand>();
	Command->SetSource(Source);
	Command->SetEventBus(InEventBus);
	return Command;
}

ECommandResult UAttackModeCommand::ExecuteInternal()
{
	bPreviousState = false;	// Would query from ButtonManager

	UE_LOG(LogTemp, Log, TEXT("AttackModeCommand: Toggling attack mode"));

	if (EventBus)
	{
		EventBus->PublishAbilityEvent(
			EOdysseyEventType::AbilityToggled,
			ActionType,
			FName("AttackMode"),
			!bPreviousState,
			0.0f
		);
	}

	return ECommandResult::Success;
}

bool UAttackModeCommand::UndoInternal()
{
	UE_LOG(LogTemp, Log, TEXT("AttackModeCommand: Reverting attack mode to %s"), bPreviousState ? TEXT("ON") : TEXT("OFF"));

	if (EventBus)
	{
		EventBus->PublishAbilityEvent(
			EOdysseyEventType::AbilityToggled,
			ActionType,
			FName("AttackMode"),
			bPreviousState,
			0.0f
		);
	}

	return true;
}

// --- USpecialAttackCommand ---

USpecialAttackCommand::USpecialAttackCommand()
	: DamageAmount(50.0f)
{
	CommandName = TEXT("SpecialAttack");
	ActionType = static_cast<uint8>(EActionButtonType::SpecialAttack);
	bCanUndo = false;
	EnergyCost = 40;
	CooldownDuration = 10.0f;
}

USpecialAttackCommand* USpecialAttackCommand::Create(AActor* Source, AActor* Target, UOdysseyEventBus* InEventBus)
{
	USpecialAttackCommand* Command = NewObject<USpecialAttackCommand>();
	Command->SetSource(Source);
	Command->SetTarget(Target);
	Command->SetEventBus(InEventBus);
	return Command;
}

ECommandResult USpecialAttackCommand::ExecuteInternal()
{
	UE_LOG(LogTemp, Log, TEXT("SpecialAttackCommand: Executing special attack with %.1f damage"), DamageAmount);

	// Publish combat events
	if (EventBus)
	{
		auto AttackPayload = MakeShared<FCombatEventPayload>();
		AttackPayload->Initialize(EOdysseyEventType::AttackStarted, SourceActor.Get());
		AttackPayload->Attacker = SourceActor;
		AttackPayload->Target = TargetActor;
		AttackPayload->DamageAmount = DamageAmount;
		AttackPayload->DamageType = FName("SpecialAttack");
		EventBus->PublishEvent(AttackPayload);

		// If target exists, also publish damage dealt
		if (TargetActor.IsValid())
		{
			auto DamagePayload = MakeShared<FCombatEventPayload>();
			DamagePayload->Initialize(EOdysseyEventType::DamageDealt, SourceActor.Get());
			DamagePayload->Attacker = SourceActor;
			DamagePayload->Target = TargetActor;
			DamagePayload->DamageAmount = DamageAmount;
			DamagePayload->DamageType = FName("SpecialAttack");
			DamagePayload->HitLocation = TargetActor->GetActorLocation();
			EventBus->PublishEvent(DamagePayload);
		}
	}

	return ECommandResult::Success;
}

bool USpecialAttackCommand::ValidateInternal(EActionFailureReason& OutFailureReason)
{
	if (!SourceActor.IsValid())
	{
		OutFailureReason = EActionFailureReason::Custom;
		return false;
	}

	// Special attack can work without target (area effect)
	return true;
}

// --- UThrusterBoostCommand ---

UThrusterBoostCommand::UThrusterBoostCommand()
	: ButtonManager(nullptr)
	, BoostDuration(3.0f)
	, SpeedMultiplier(2.0f)
{
	CommandName = TEXT("ThrusterBoost");
	ActionType = static_cast<uint8>(EActionButtonType::ThrusterBoost);
	bCanUndo = false;
	EnergyCost = 25;
	CooldownDuration = 8.0f;
}

UThrusterBoostCommand* UThrusterBoostCommand::Create(AActor* Source, UOdysseyEventBus* InEventBus, float Duration)
{
	UThrusterBoostCommand* Command = NewObject<UThrusterBoostCommand>();
	Command->SetSource(Source);
	Command->SetEventBus(InEventBus);
	Command->SetBoostDuration(Duration);
	return Command;
}

ECommandResult UThrusterBoostCommand::ExecuteInternal()
{
	UE_LOG(LogTemp, Log, TEXT("ThrusterBoostCommand: Activating thruster boost for %.1f seconds"), BoostDuration);

	if (EventBus)
	{
		auto Payload = MakeShared<FThrusterBoostEventPayload>();
		Payload->Initialize(EOdysseyEventType::ThrusterBoostStarted, SourceActor.Get());
		Payload->BoostDuration = BoostDuration;
		Payload->RemainingTime = BoostDuration;
		Payload->SpeedMultiplier = SpeedMultiplier;
		EventBus->PublishEvent(Payload);

		// Also publish as ability activated
		EventBus->PublishAbilityEvent(
			EOdysseyEventType::AbilityActivated,
			ActionType,
			FName("ThrusterBoost"),
			true,
			BoostDuration
		);
	}

	return ECommandResult::Success;
}

// --- UEnergyCostCommand ---

UEnergyCostCommand::UEnergyCostCommand()
	: InnerCommand(nullptr)
	, ButtonManager(nullptr)
	, EnergyToSpend(0)
	, bEnergySpent(false)
{
	CommandName = TEXT("EnergyCostWrapper");
	bCanUndo = false;	// Determined by inner command
}

UEnergyCostCommand* UEnergyCostCommand::Create(UOdysseyActionCommand* InInnerCommand, int32 Cost, UOdysseyActionButtonManager* InButtonManager)
{
	UEnergyCostCommand* Command = NewObject<UEnergyCostCommand>();
	Command->InnerCommand = InInnerCommand;
	Command->ButtonManager = InButtonManager;
	Command->EnergyToSpend = Cost;
	Command->EnergyCost = Cost;

	if (InInnerCommand)
	{
		Command->CommandName = FString::Printf(TEXT("EnergyCost(%s)"), *InInnerCommand->GetCommandName());
		Command->ActionType = InInnerCommand->GetActionType();
		Command->bCanUndo = InInnerCommand->CanUndo();
	}

	return Command;
}

ECommandResult UEnergyCostCommand::ExecuteInternal()
{
	if (!InnerCommand || !ButtonManager)
	{
		return ECommandResult::Failed;
	}

	// Spend energy
	if (!ButtonManager->SpendEnergy(EnergyToSpend))
	{
		return ECommandResult::Failed;
	}

	bEnergySpent = true;

	// Execute inner command
	ECommandResult Result = InnerCommand->Execute();

	// If inner command failed, refund energy
	if (Result == ECommandResult::Failed)
	{
		// Note: In a real implementation, we'd refund the energy here
		bEnergySpent = false;
	}

	return Result;
}

bool UEnergyCostCommand::UndoInternal()
{
	if (!InnerCommand)
	{
		return false;
	}

	// Undo inner command
	if (!InnerCommand->Undo())
	{
		return false;
	}

	// Refund energy would happen here
	bEnergySpent = false;

	return true;
}

bool UEnergyCostCommand::ValidateInternal(EActionFailureReason& OutFailureReason)
{
	if (!ButtonManager)
	{
		OutFailureReason = EActionFailureReason::Custom;
		return false;
	}

	// Check if we can afford the energy cost
	if (!ButtonManager->CanAffordAction(static_cast<EActionButtonType>(ActionType)))
	{
		OutFailureReason = EActionFailureReason::InsufficientEnergy;
		return false;
	}

	// Validate inner command
	if (InnerCommand)
	{
		return InnerCommand->Validate(OutFailureReason);
	}

	return true;
}

// --- UCompositeCommand ---

UCompositeCommand::UCompositeCommand()
	: LastExecutedIndex(-1)
{
	CommandName = TEXT("CompositeCommand");
	bCanUndo = true;
}

void UCompositeCommand::AddCommand(UOdysseyActionCommand* Command)
{
	if (Command)
	{
		Commands.Add(Command);

		// Can only undo if all commands can undo
		if (!Command->CanUndo())
		{
			bCanUndo = false;
		}
	}
}

void UCompositeCommand::ClearCommands()
{
	Commands.Empty();
	LastExecutedIndex = -1;
	bCanUndo = true;
}

ECommandResult UCompositeCommand::ExecuteInternal()
{
	LastExecutedIndex = -1;

	for (int32 i = 0; i < Commands.Num(); ++i)
	{
		if (Commands[i])
		{
			ECommandResult Result = Commands[i]->Execute();

			if (Result != ECommandResult::Success)
			{
				// Rollback executed commands
				for (int32 j = i - 1; j >= 0; --j)
				{
					if (Commands[j] && Commands[j]->CanUndo())
					{
						Commands[j]->Undo();
					}
				}
				return Result;
			}

			LastExecutedIndex = i;
		}
	}

	return ECommandResult::Success;
}

bool UCompositeCommand::UndoInternal()
{
	// Undo in reverse order
	for (int32 i = LastExecutedIndex; i >= 0; --i)
	{
		if (Commands[i] && Commands[i]->CanUndo())
		{
			if (!Commands[i]->Undo())
			{
				return false;
			}
		}
	}

	LastExecutedIndex = -1;
	return true;
}

bool UCompositeCommand::ValidateInternal(EActionFailureReason& OutFailureReason)
{
	for (UOdysseyActionCommand* Command : Commands)
	{
		if (Command && !Command->Validate(OutFailureReason))
		{
			return false;
		}
	}

	return true;
}

// --- UDelayedCommand ---

UDelayedCommand::UDelayedCommand()
	: InnerCommand(nullptr)
	, DelaySeconds(0.0f)
	, StartTime(0.0)
{
	CommandName = TEXT("DelayedCommand");
}

UDelayedCommand* UDelayedCommand::Create(UOdysseyActionCommand* InInnerCommand, float Delay)
{
	UDelayedCommand* Command = NewObject<UDelayedCommand>();
	Command->InnerCommand = InInnerCommand;
	Command->DelaySeconds = Delay;
	Command->StartTime = FPlatformTime::Seconds();

	if (InInnerCommand)
	{
		Command->CommandName = FString::Printf(TEXT("Delayed(%s, %.2fs)"), *InInnerCommand->GetCommandName(), Delay);
		Command->ActionType = InInnerCommand->GetActionType();
		Command->bCanUndo = InInnerCommand->CanUndo();
	}

	return Command;
}

bool UDelayedCommand::IsReady() const
{
	return GetRemainingDelay() <= 0.0f;
}

float UDelayedCommand::GetRemainingDelay() const
{
	double Elapsed = FPlatformTime::Seconds() - StartTime;
	return FMath::Max(0.0f, DelaySeconds - static_cast<float>(Elapsed));
}

ECommandResult UDelayedCommand::ExecuteInternal()
{
	if (!IsReady())
	{
		return ECommandResult::Pending;
	}

	if (!InnerCommand)
	{
		return ECommandResult::Failed;
	}

	return InnerCommand->Execute();
}
