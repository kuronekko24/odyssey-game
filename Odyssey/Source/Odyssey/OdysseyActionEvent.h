// OdysseyActionEvent.h
// Core event definitions and base classes for the Odyssey event-driven action system
// Replaces switch-case action dispatch with extensible event architecture

#pragma once

#include "CoreMinimal.h"
#include "HAL/Platform.h"
#include "OdysseyActionEvent.generated.h"

// Forward declarations
class UOdysseyEventBus;
class UOdysseyActionDispatcher;

/**
 * Event type identifiers for O(1) lookup and routing
 * Using uint32 for efficient hashing and comparison
 */
UENUM(BlueprintType)
enum class EOdysseyEventType : uint8
{
	None = 0,

	// Action lifecycle events
	ActionRequested = 1,		// When button is pressed, before validation
	ActionValidating = 2,		// During validation checks
	ActionExecuted = 3,			// When action successfully executes
	ActionFailed = 4,			// When action cannot execute (cooldown, energy, etc.)
	ActionCancelled = 5,		// When action is cancelled mid-execution

	// Cooldown events
	CooldownStarted = 10,
	CooldownTick = 11,			// For UI progress updates
	CooldownCompleted = 12,

	// Energy events
	EnergyChanged = 20,
	EnergyDepleted = 21,
	EnergyRestored = 22,		// When energy regenerates to full

	// Ability state events
	AbilityActivated = 30,
	AbilityDeactivated = 31,
	AbilityToggled = 32,

	// Combat events
	AttackStarted = 40,
	AttackHit = 41,
	AttackMissed = 42,
	DamageDealt = 43,
	DamageReceived = 44,

	// Movement events
	ThrusterBoostStarted = 50,
	ThrusterBoostEnded = 51,

	// Interaction events
	InteractionStarted = 60,
	InteractionCompleted = 61,
	InteractionCancelled = 62,

	// System events
	EventBusInitialized = 100,
	EventBusShutdown = 101,
	ActionRegistered = 102,
	ActionUnregistered = 103,

	// Custom event range starts at 200 for game-specific extensions
	CustomEventStart = 200,

	Max = 255
};

/**
 * Failure reasons for ActionFailed events
 */
UENUM(BlueprintType)
enum class EActionFailureReason : uint8
{
	None = 0,
	OnCooldown,
	InsufficientEnergy,
	InvalidTarget,
	Disabled,
	Busy,
	Interrupted,
	RequirementNotMet,
	Custom
};

/**
 * Event priority for processing order
 * Higher priority events are processed first
 */
UENUM(BlueprintType)
enum class EOdysseyEventPriority : uint8
{
	Low = 0,
	Normal = 50,
	High = 100,
	Critical = 200,		// For system-level events that must be processed first
	Immediate = 255		// Bypass queue, process synchronously
};

/**
 * Unique event identifier for tracking and debugging
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FOdysseyEventId
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	uint64 Id;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	double Timestamp;

	FOdysseyEventId()
		: Id(0)
		, Timestamp(0.0)
	{
	}

	static FOdysseyEventId Generate();

	bool IsValid() const { return Id != 0; }

	bool operator==(const FOdysseyEventId& Other) const { return Id == Other.Id; }
	bool operator!=(const FOdysseyEventId& Other) const { return Id != Other.Id; }

	FString ToString() const { return FString::Printf(TEXT("Event_%llu"), Id); }

private:
	static TAtomic<uint64> NextEventId;
};

/**
 * Base event payload structure
 * All event data structures inherit from this
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FOdysseyEventPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FOdysseyEventId EventId;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	EOdysseyEventType EventType;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	EOdysseyEventPriority Priority;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	double CreationTime;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	TWeakObjectPtr<AActor> Source;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	bool bConsumed;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	bool bCancellable;

	FOdysseyEventPayload()
		: EventType(EOdysseyEventType::None)
		, Priority(EOdysseyEventPriority::Normal)
		, CreationTime(0.0)
		, bConsumed(false)
		, bCancellable(true)
	{
	}

	void Initialize(EOdysseyEventType InType, AActor* InSource = nullptr, EOdysseyEventPriority InPriority = EOdysseyEventPriority::Normal)
	{
		EventId = FOdysseyEventId::Generate();
		EventType = InType;
		Priority = InPriority;
		CreationTime = FPlatformTime::Seconds();
		Source = InSource;
		bConsumed = false;
	}

	void Consume() { bConsumed = true; }
	bool IsConsumed() const { return bConsumed; }

	virtual ~FOdysseyEventPayload() = default;
};

/**
 * Action-specific event data
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FActionEventPayload : public FOdysseyEventPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Action Event")
	uint8 ActionType;	// EActionButtonType cast to uint8 for decoupling

	UPROPERTY(BlueprintReadOnly, Category = "Action Event")
	FName ActionName;

	UPROPERTY(BlueprintReadOnly, Category = "Action Event")
	int32 EnergyCost;

	UPROPERTY(BlueprintReadOnly, Category = "Action Event")
	float CooldownDuration;

	UPROPERTY(BlueprintReadOnly, Category = "Action Event")
	TWeakObjectPtr<AActor> Target;

	UPROPERTY(BlueprintReadOnly, Category = "Action Event")
	FVector TargetLocation;

	UPROPERTY(BlueprintReadOnly, Category = "Action Event")
	bool bHasTargetLocation;

	FActionEventPayload()
		: ActionType(0)
		, ActionName(NAME_None)
		, EnergyCost(0)
		, CooldownDuration(0.0f)
		, TargetLocation(FVector::ZeroVector)
		, bHasTargetLocation(false)
	{
	}
};

/**
 * Action failure event data
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FActionFailedEventPayload : public FActionEventPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Failure Event")
	EActionFailureReason FailureReason;

	UPROPERTY(BlueprintReadOnly, Category = "Failure Event")
	FString FailureMessage;

	UPROPERTY(BlueprintReadOnly, Category = "Failure Event")
	float RemainingCooldown;

	UPROPERTY(BlueprintReadOnly, Category = "Failure Event")
	int32 RequiredEnergy;

	UPROPERTY(BlueprintReadOnly, Category = "Failure Event")
	int32 CurrentEnergy;

	FActionFailedEventPayload()
		: FailureReason(EActionFailureReason::None)
		, RemainingCooldown(0.0f)
		, RequiredEnergy(0)
		, CurrentEnergy(0)
	{
	}
};

/**
 * Cooldown event data
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FCooldownEventPayload : public FOdysseyEventPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Cooldown Event")
	uint8 ActionType;

	UPROPERTY(BlueprintReadOnly, Category = "Cooldown Event")
	FName ActionName;

	UPROPERTY(BlueprintReadOnly, Category = "Cooldown Event")
	float TotalDuration;

	UPROPERTY(BlueprintReadOnly, Category = "Cooldown Event")
	float RemainingTime;

	UPROPERTY(BlueprintReadOnly, Category = "Cooldown Event")
	float Progress;	// 0.0 to 1.0

	FCooldownEventPayload()
		: ActionType(0)
		, ActionName(NAME_None)
		, TotalDuration(0.0f)
		, RemainingTime(0.0f)
		, Progress(0.0f)
	{
	}
};

/**
 * Energy change event data
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FEnergyEventPayload : public FOdysseyEventPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Energy Event")
	float PreviousEnergy;

	UPROPERTY(BlueprintReadOnly, Category = "Energy Event")
	float CurrentEnergy;

	UPROPERTY(BlueprintReadOnly, Category = "Energy Event")
	float MaxEnergy;

	UPROPERTY(BlueprintReadOnly, Category = "Energy Event")
	float DeltaEnergy;

	UPROPERTY(BlueprintReadOnly, Category = "Energy Event")
	FName ChangeReason;	// "ActionCost", "Regeneration", "Pickup", etc.

	FEnergyEventPayload()
		: PreviousEnergy(0.0f)
		, CurrentEnergy(0.0f)
		, MaxEnergy(100.0f)
		, DeltaEnergy(0.0f)
		, ChangeReason(NAME_None)
	{
	}

	float GetEnergyPercentage() const
	{
		return MaxEnergy > 0.0f ? CurrentEnergy / MaxEnergy : 0.0f;
	}
};

/**
 * Ability state event data
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FAbilityEventPayload : public FOdysseyEventPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Ability Event")
	uint8 AbilityType;

	UPROPERTY(BlueprintReadOnly, Category = "Ability Event")
	FName AbilityName;

	UPROPERTY(BlueprintReadOnly, Category = "Ability Event")
	bool bIsActive;

	UPROPERTY(BlueprintReadOnly, Category = "Ability Event")
	float Duration;		// For timed abilities

	UPROPERTY(BlueprintReadOnly, Category = "Ability Event")
	float RemainingTime;

	UPROPERTY(BlueprintReadOnly, Category = "Ability Event")
	TMap<FName, float> ModifierValues;	// Ability-specific modifiers

	FAbilityEventPayload()
		: AbilityType(0)
		, AbilityName(NAME_None)
		, bIsActive(false)
		, Duration(0.0f)
		, RemainingTime(0.0f)
	{
	}
};

/**
 * Combat event data
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FCombatEventPayload : public FOdysseyEventPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	TWeakObjectPtr<AActor> Attacker;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	TWeakObjectPtr<AActor> Target;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	float DamageAmount;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	FName DamageType;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	FVector HitLocation;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	bool bIsCritical;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	bool bWasBlocked;

	FCombatEventPayload()
		: DamageAmount(0.0f)
		, DamageType(NAME_None)
		, HitLocation(FVector::ZeroVector)
		, bIsCritical(false)
		, bWasBlocked(false)
	{
	}
};

/**
 * Thruster boost event data
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FThrusterBoostEventPayload : public FOdysseyEventPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Thruster Event")
	float BoostDuration;

	UPROPERTY(BlueprintReadOnly, Category = "Thruster Event")
	float RemainingTime;

	UPROPERTY(BlueprintReadOnly, Category = "Thruster Event")
	float SpeedMultiplier;

	FThrusterBoostEventPayload()
		: BoostDuration(0.0f)
		, RemainingTime(0.0f)
		, SpeedMultiplier(1.0f)
	{
	}
};

/**
 * Interaction event data
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FInteractionEventPayload : public FOdysseyEventPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Interaction Event")
	TWeakObjectPtr<AActor> InteractableActor;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction Event")
	FName InteractionType;	// "Mining", "Crafting", "Trading", etc.

	UPROPERTY(BlueprintReadOnly, Category = "Interaction Event")
	float InteractionProgress;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction Event")
	TMap<FName, int32> ResultItems;	// Items gained from interaction

	FInteractionEventPayload()
		: InteractionType(NAME_None)
		, InteractionProgress(0.0f)
	{
	}
};

/**
 * Event handler delegate types
 * Using multicast delegates for flexible subscription
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOdysseyEventDelegate, const FOdysseyEventPayload&, Payload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FActionEventDelegate, const FActionEventPayload&, Payload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FActionFailedEventDelegate, const FActionFailedEventPayload&, Payload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCooldownEventDelegate, const FCooldownEventPayload&, Payload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEnergyEventDelegate, const FEnergyEventPayload&, Payload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityEventDelegate, const FAbilityEventPayload&, Payload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCombatEventDelegate, const FCombatEventPayload&, Payload);

/**
 * Non-dynamic delegate types for C++ subscribers (more efficient)
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOdysseyEventNativeDelegate, const FOdysseyEventPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FActionEventNativeDelegate, const FActionEventPayload&);
DECLARE_MULTICAST_DELEGATE_OneParam(FEnergyEventNativeDelegate, const FEnergyEventPayload&);

/**
 * Event subscription handle for unsubscription
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FOdysseyEventHandle
{
	GENERATED_BODY()

	uint64 HandleId;
	EOdysseyEventType EventType;

	FOdysseyEventHandle()
		: HandleId(0)
		, EventType(EOdysseyEventType::None)
	{
	}

	FOdysseyEventHandle(uint64 InHandleId, EOdysseyEventType InEventType)
		: HandleId(InHandleId)
		, EventType(InEventType)
	{
	}

	bool IsValid() const { return HandleId != 0; }

	void Reset()
	{
		HandleId = 0;
		EventType = EOdysseyEventType::None;
	}
};

/**
 * Event filter for selective subscription
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FOdysseyEventFilter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	TArray<EOdysseyEventType> AllowedEventTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	TArray<uint8> AllowedActionTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	TWeakObjectPtr<AActor> RequiredSource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	EOdysseyEventPriority MinimumPriority;

	FOdysseyEventFilter()
		: MinimumPriority(EOdysseyEventPriority::Low)
	{
	}

	bool Matches(const FOdysseyEventPayload& Payload) const
	{
		// Check event type filter
		if (AllowedEventTypes.Num() > 0 && !AllowedEventTypes.Contains(Payload.EventType))
		{
			return false;
		}

		// Check priority
		if (static_cast<uint8>(Payload.Priority) < static_cast<uint8>(MinimumPriority))
		{
			return false;
		}

		// Check source filter
		if (RequiredSource.IsValid() && Payload.Source != RequiredSource)
		{
			return false;
		}

		return true;
	}
};

/**
 * Event metrics for performance monitoring
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FOdysseyEventMetrics
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	int64 TotalEventsPublished;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	int64 TotalEventsProcessed;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	int64 EventsDropped;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	double AverageProcessingTimeMs;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	double PeakProcessingTimeMs;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	int32 CurrentQueueDepth;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	int32 PeakQueueDepth;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	int32 ActiveSubscribers;

	FOdysseyEventMetrics()
		: TotalEventsPublished(0)
		, TotalEventsProcessed(0)
		, EventsDropped(0)
		, AverageProcessingTimeMs(0.0)
		, PeakProcessingTimeMs(0.0)
		, CurrentQueueDepth(0)
		, PeakQueueDepth(0)
		, ActiveSubscribers(0)
	{
	}

	void Reset()
	{
		TotalEventsPublished = 0;
		TotalEventsProcessed = 0;
		EventsDropped = 0;
		AverageProcessingTimeMs = 0.0;
		PeakProcessingTimeMs = 0.0;
		CurrentQueueDepth = 0;
		PeakQueueDepth = 0;
		// Don't reset ActiveSubscribers
	}
};
