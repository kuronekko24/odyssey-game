// OdysseyActionCommand.h
// Command pattern implementation for Odyssey action system
// Enables undo/redo, queuing, and thread-safe action execution

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HAL/ThreadSafeBool.h"
#include "OdysseyActionEvent.h"
#include "OdysseyActionCommand.generated.h"

// Forward declarations
class UOdysseyEventBus;
class UOdysseyActionButtonManager;
class AOdysseyCharacter;

/**
 * Command execution result
 */
UENUM(BlueprintType)
enum class ECommandResult : uint8
{
	Success,
	Failed,
	Pending,		// Command is queued/deferred
	Cancelled,
	Undone
};

/**
 * Command state for lifecycle tracking
 */
UENUM(BlueprintType)
enum class ECommandState : uint8
{
	Created,
	Validating,
	Executing,
	Executed,
	Undoing,
	Undone,
	Failed,
	Cancelled
};

/**
 * Base command interface
 * All action commands inherit from this
 */
UCLASS(Abstract, BlueprintType)
class ODYSSEY_API UOdysseyActionCommand : public UObject
{
	GENERATED_BODY()

public:
	UOdysseyActionCommand();
	virtual ~UOdysseyActionCommand() = default;

	/**
	 * Execute the command
	 * @return Result of execution
	 */
	UFUNCTION(BlueprintCallable, Category = "Command")
	virtual ECommandResult Execute();

	/**
	 * Undo the command (if supported)
	 * @return True if undo was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Command")
	virtual bool Undo();

	/**
	 * Redo the command (re-execute after undo)
	 * @return Result of re-execution
	 */
	UFUNCTION(BlueprintCallable, Category = "Command")
	virtual ECommandResult Redo();

	/**
	 * Validate that the command can be executed
	 * @param OutFailureReason Reason for validation failure
	 * @return True if command can execute
	 */
	UFUNCTION(BlueprintCallable, Category = "Command")
	virtual bool Validate(EActionFailureReason& OutFailureReason);

	/**
	 * Cancel command execution (if in progress)
	 */
	UFUNCTION(BlueprintCallable, Category = "Command")
	virtual void Cancel();

	// Properties
	UFUNCTION(BlueprintCallable, Category = "Command")
	bool CanUndo() const { return bCanUndo; }

	UFUNCTION(BlueprintCallable, Category = "Command")
	ECommandState GetState() const { return State; }

	UFUNCTION(BlueprintCallable, Category = "Command")
	FString GetCommandName() const { return CommandName; }

	UFUNCTION(BlueprintCallable, Category = "Command")
	double GetExecutionTime() const { return ExecutionTime; }

	UFUNCTION(BlueprintCallable, Category = "Command")
	uint8 GetActionType() const { return ActionType; }

	// Event bus integration
	void SetEventBus(UOdysseyEventBus* InEventBus) { EventBus = InEventBus; }

	// Context setters
	void SetSource(AActor* InSource) { SourceActor = InSource; }
	void SetTarget(AActor* InTarget) { TargetActor = InTarget; }

protected:
	// Override in derived classes
	virtual ECommandResult ExecuteInternal() { return ECommandResult::Success; }
	virtual bool UndoInternal() { return false; }
	virtual bool ValidateInternal(EActionFailureReason& OutFailureReason) { return true; }

	// Helper to publish events
	void PublishActionEvent(EOdysseyEventType EventType, EActionFailureReason FailureReason = EActionFailureReason::None);

	UPROPERTY(BlueprintReadOnly, Category = "Command")
	FString CommandName;

	UPROPERTY(BlueprintReadOnly, Category = "Command")
	uint8 ActionType;

	UPROPERTY(BlueprintReadOnly, Category = "Command")
	ECommandState State;

	UPROPERTY(BlueprintReadOnly, Category = "Command")
	bool bCanUndo;

	UPROPERTY(BlueprintReadOnly, Category = "Command")
	double ExecutionTime;

	UPROPERTY(BlueprintReadOnly, Category = "Command")
	int32 EnergyCost;

	UPROPERTY(BlueprintReadOnly, Category = "Command")
	float CooldownDuration;

	UPROPERTY()
	TWeakObjectPtr<AActor> SourceActor;

	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY()
	UOdysseyEventBus* EventBus;
};

/**
 * Command queue for thread-safe command processing
 */
UCLASS(BlueprintType)
class ODYSSEY_API UOdysseyCommandQueue : public UObject
{
	GENERATED_BODY()

public:
	UOdysseyCommandQueue();

	/**
	 * Initialize the command queue
	 * @param InEventBus Event bus for command events
	 * @param MaxSize Maximum queue size
	 */
	UFUNCTION(BlueprintCallable, Category = "Command Queue")
	void Initialize(UOdysseyEventBus* InEventBus, int32 MaxSize = 32);

	/**
	 * Enqueue a command for execution
	 * @param Command Command to enqueue
	 * @return True if command was queued
	 */
	UFUNCTION(BlueprintCallable, Category = "Command Queue")
	bool Enqueue(UOdysseyActionCommand* Command);

	/**
	 * Process queued commands
	 * @param MaxCommands Maximum commands to process (0 = all)
	 * @return Number of commands processed
	 */
	UFUNCTION(BlueprintCallable, Category = "Command Queue")
	int32 ProcessCommands(int32 MaxCommands = 0);

	/**
	 * Clear all pending commands
	 */
	UFUNCTION(BlueprintCallable, Category = "Command Queue")
	void Clear();

	/**
	 * Get number of pending commands
	 */
	UFUNCTION(BlueprintCallable, Category = "Command Queue")
	int32 GetPendingCount() const;

	/**
	 * Check if queue is full
	 */
	UFUNCTION(BlueprintCallable, Category = "Command Queue")
	bool IsFull() const;

private:
	UPROPERTY()
	TArray<UOdysseyActionCommand*> PendingCommands;

	UPROPERTY()
	UOdysseyEventBus* EventBus;

	int32 MaxQueueSize;
	mutable FCriticalSection QueueLock;
};

/**
 * Command history for undo/redo support
 */
UCLASS(BlueprintType)
class ODYSSEY_API UOdysseyCommandHistory : public UObject
{
	GENERATED_BODY()

public:
	UOdysseyCommandHistory();

	/**
	 * Initialize the history
	 * @param MaxHistorySize Maximum commands to keep in history
	 */
	UFUNCTION(BlueprintCallable, Category = "Command History")
	void Initialize(int32 MaxHistorySize = 50);

	/**
	 * Record an executed command
	 */
	UFUNCTION(BlueprintCallable, Category = "Command History")
	void RecordCommand(UOdysseyActionCommand* Command);

	/**
	 * Undo the last command
	 * @return True if undo was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Command History")
	bool Undo();

	/**
	 * Redo the last undone command
	 * @return True if redo was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Command History")
	bool Redo();

	/**
	 * Check if undo is available
	 */
	UFUNCTION(BlueprintCallable, Category = "Command History")
	bool CanUndo() const;

	/**
	 * Check if redo is available
	 */
	UFUNCTION(BlueprintCallable, Category = "Command History")
	bool CanRedo() const;

	/**
	 * Get the current history position
	 */
	UFUNCTION(BlueprintCallable, Category = "Command History")
	int32 GetHistoryPosition() const { return CurrentIndex; }

	/**
	 * Get the total history count
	 */
	UFUNCTION(BlueprintCallable, Category = "Command History")
	int32 GetHistoryCount() const { return History.Num(); }

	/**
	 * Clear all history
	 */
	UFUNCTION(BlueprintCallable, Category = "Command History")
	void Clear();

private:
	UPROPERTY()
	TArray<UOdysseyActionCommand*> History;

	int32 CurrentIndex;
	int32 MaxSize;
};

// ============================================================================
// Concrete Command Implementations
// ============================================================================

/**
 * Interact action command
 */
UCLASS(BlueprintType)
class ODYSSEY_API UInteractCommand : public UOdysseyActionCommand
{
	GENERATED_BODY()

public:
	UInteractCommand();

	static UInteractCommand* Create(AActor* Source, AActor* Target, UOdysseyEventBus* EventBus);

protected:
	virtual ECommandResult ExecuteInternal() override;
	virtual bool ValidateInternal(EActionFailureReason& OutFailureReason) override;
};

/**
 * Open cargo/inventory command
 */
UCLASS(BlueprintType)
class ODYSSEY_API UOpenCargoCommand : public UOdysseyActionCommand
{
	GENERATED_BODY()

public:
	UOpenCargoCommand();

	static UOpenCargoCommand* Create(AActor* Source, UOdysseyEventBus* EventBus);

protected:
	virtual ECommandResult ExecuteInternal() override;
	virtual bool UndoInternal() override;

private:
	bool bWasCargoOpen;
};

/**
 * Scout mode toggle command
 */
UCLASS(BlueprintType)
class ODYSSEY_API UScoutModeCommand : public UOdysseyActionCommand
{
	GENERATED_BODY()

public:
	UScoutModeCommand();

	static UScoutModeCommand* Create(AActor* Source, UOdysseyEventBus* EventBus);

	void SetButtonManager(UOdysseyActionButtonManager* Manager) { ButtonManager = Manager; }

protected:
	virtual ECommandResult ExecuteInternal() override;
	virtual bool UndoInternal() override;

private:
	UPROPERTY()
	UOdysseyActionButtonManager* ButtonManager;

	bool bPreviousState;
};

/**
 * Attack mode toggle command
 */
UCLASS(BlueprintType)
class ODYSSEY_API UAttackModeCommand : public UOdysseyActionCommand
{
	GENERATED_BODY()

public:
	UAttackModeCommand();

	static UAttackModeCommand* Create(AActor* Source, UOdysseyEventBus* EventBus);

	void SetButtonManager(UOdysseyActionButtonManager* Manager) { ButtonManager = Manager; }

protected:
	virtual ECommandResult ExecuteInternal() override;
	virtual bool UndoInternal() override;

private:
	UPROPERTY()
	UOdysseyActionButtonManager* ButtonManager;

	bool bPreviousState;
};

/**
 * Special attack command
 */
UCLASS(BlueprintType)
class ODYSSEY_API USpecialAttackCommand : public UOdysseyActionCommand
{
	GENERATED_BODY()

public:
	USpecialAttackCommand();

	static USpecialAttackCommand* Create(AActor* Source, AActor* Target, UOdysseyEventBus* EventBus);

	void SetDamageAmount(float Damage) { DamageAmount = Damage; }

protected:
	virtual ECommandResult ExecuteInternal() override;
	virtual bool ValidateInternal(EActionFailureReason& OutFailureReason) override;

private:
	float DamageAmount;
};

/**
 * Thruster boost command
 */
UCLASS(BlueprintType)
class ODYSSEY_API UThrusterBoostCommand : public UOdysseyActionCommand
{
	GENERATED_BODY()

public:
	UThrusterBoostCommand();

	static UThrusterBoostCommand* Create(AActor* Source, UOdysseyEventBus* EventBus, float Duration = 3.0f);

	void SetBoostDuration(float Duration) { BoostDuration = Duration; }
	void SetSpeedMultiplier(float Multiplier) { SpeedMultiplier = Multiplier; }
	void SetButtonManager(UOdysseyActionButtonManager* Manager) { ButtonManager = Manager; }

protected:
	virtual ECommandResult ExecuteInternal() override;

private:
	UPROPERTY()
	UOdysseyActionButtonManager* ButtonManager;

	float BoostDuration;
	float SpeedMultiplier;
};

/**
 * Generic energy cost command wrapper
 * Wraps any command with energy cost validation and deduction
 */
UCLASS(BlueprintType)
class ODYSSEY_API UEnergyCostCommand : public UOdysseyActionCommand
{
	GENERATED_BODY()

public:
	UEnergyCostCommand();

	static UEnergyCostCommand* Create(UOdysseyActionCommand* InnerCommand, int32 Cost, UOdysseyActionButtonManager* ButtonManager);

	void SetInnerCommand(UOdysseyActionCommand* Command) { InnerCommand = Command; }
	void SetButtonManager(UOdysseyActionButtonManager* Manager) { ButtonManager = Manager; }

protected:
	virtual ECommandResult ExecuteInternal() override;
	virtual bool UndoInternal() override;
	virtual bool ValidateInternal(EActionFailureReason& OutFailureReason) override;

private:
	UPROPERTY()
	UOdysseyActionCommand* InnerCommand;

	UPROPERTY()
	UOdysseyActionButtonManager* ButtonManager;

	int32 EnergyToSpend;
	bool bEnergySpent;
};

/**
 * Composite command for executing multiple commands in sequence
 */
UCLASS(BlueprintType)
class ODYSSEY_API UCompositeCommand : public UOdysseyActionCommand
{
	GENERATED_BODY()

public:
	UCompositeCommand();

	void AddCommand(UOdysseyActionCommand* Command);
	void ClearCommands();

protected:
	virtual ECommandResult ExecuteInternal() override;
	virtual bool UndoInternal() override;
	virtual bool ValidateInternal(EActionFailureReason& OutFailureReason) override;

private:
	UPROPERTY()
	TArray<UOdysseyActionCommand*> Commands;

	int32 LastExecutedIndex;
};

/**
 * Delayed command that executes after a specified time
 */
UCLASS(BlueprintType)
class ODYSSEY_API UDelayedCommand : public UOdysseyActionCommand
{
	GENERATED_BODY()

public:
	UDelayedCommand();

	static UDelayedCommand* Create(UOdysseyActionCommand* InnerCommand, float Delay);

	void SetDelay(float InDelay) { DelaySeconds = InDelay; }

	/**
	 * Check if delay has elapsed
	 */
	UFUNCTION(BlueprintCallable, Category = "Delayed Command")
	bool IsReady() const;

	/**
	 * Get remaining delay time
	 */
	UFUNCTION(BlueprintCallable, Category = "Delayed Command")
	float GetRemainingDelay() const;

protected:
	virtual ECommandResult ExecuteInternal() override;

private:
	UPROPERTY()
	UOdysseyActionCommand* InnerCommand;

	float DelaySeconds;
	double StartTime;
};
