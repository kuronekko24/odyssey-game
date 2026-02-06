// NPCBehaviorComponent.h
// AI state machine and behavior management for NPC ships
// Performance-conscious design with tiered update frequencies for mobile platforms
// Integrates with OdysseyEventBus for event-driven state transitions

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "OdysseyActionEvent.h"
#include "OdysseyMobileOptimizer.h"
#include "NPCBehaviorComponent.generated.h"

// Forward declarations
class ANPCShip;
class AOdysseyCharacter;
class UOdysseyEventBus;
class UOdysseyActionDispatcher;

/**
 * NPC AI State enumeration
 * Simple state machine for combat-ready NPCs
 */
UENUM(BlueprintType)
enum class ENPCState : uint8
{
	Idle,			// Default state - ship is stationary, scanning for targets
	Patrolling,		// Moving along patrol routes, reactive to events
	Engaging,		// In combat with a target, using combat systems
	Dead			// Ship is destroyed/disabled, cleanup and respawn handling
};

/**
 * NPC engagement data structure
 * Contains information about current combat target
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FNPCEngagementData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Engagement")
	TWeakObjectPtr<AOdysseyCharacter> Target;

	UPROPERTY(BlueprintReadOnly, Category = "Engagement")
	float EngagementStartTime;

	UPROPERTY(BlueprintReadOnly, Category = "Engagement")
	float LastAttackTime;

	UPROPERTY(BlueprintReadOnly, Category = "Engagement")
	float DistanceToTarget;

	UPROPERTY(BlueprintReadOnly, Category = "Engagement")
	int32 AttackCount;

	UPROPERTY(BlueprintReadOnly, Category = "Engagement")
	float TotalDamageDealt;

	FNPCEngagementData()
		: EngagementStartTime(0.0f)
		, LastAttackTime(0.0f)
		, DistanceToTarget(0.0f)
		, AttackCount(0)
		, TotalDamageDealt(0.0f)
	{
	}

	void Reset()
	{
		Target.Reset();
		EngagementStartTime = 0.0f;
		LastAttackTime = 0.0f;
		DistanceToTarget = 0.0f;
		AttackCount = 0;
		TotalDamageDealt = 0.0f;
	}
};

/**
 * NPC patrol configuration
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FNPCPatrolConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	TArray<FVector> PatrolPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	float PatrolSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	float PatrolRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	bool bLoopPatrol;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	float WaitTimeAtPoint;

	FNPCPatrolConfig()
		: PatrolSpeed(300.0f)
		, PatrolRadius(100.0f)
		, bLoopPatrol(true)
		, WaitTimeAtPoint(2.0f)
	{
	}
};

/**
 * Performance-tiered update settings for NPC behavior
 * Different tiers allow mobile devices to scale NPC complexity
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FNPCBehaviorPerformanceSettings
{
	GENERATED_BODY()

	/** State machine update frequency in Hz */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	float UpdateFrequency;

	/** Detection scan frequency in Hz */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	float DetectionUpdateFrequency;

	/** Whether patrol movement is enabled at this tier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	bool bEnablePatrolling;

	/** Whether to do line-of-sight checks (expensive) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	bool bEnableLineOfSightChecks;

	/** Maximum detection range override for this tier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	float DetectionRangeMultiplier;

	FNPCBehaviorPerformanceSettings()
		: UpdateFrequency(10.0f)
		, DetectionUpdateFrequency(2.0f)
		, bEnablePatrolling(true)
		, bEnableLineOfSightChecks(true)
		, DetectionRangeMultiplier(1.0f)
	{
	}
};

/**
 * NPC state change event payload for event bus integration
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FNPCStateChangeEventPayload : public FOdysseyEventPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "NPC Event")
	ENPCState PreviousState;

	UPROPERTY(BlueprintReadOnly, Category = "NPC Event")
	ENPCState NewState;

	UPROPERTY(BlueprintReadOnly, Category = "NPC Event")
	FName NPCShipName;

	UPROPERTY(BlueprintReadOnly, Category = "NPC Event")
	TWeakObjectPtr<AActor> EngagementTarget;

	FNPCStateChangeEventPayload()
		: PreviousState(ENPCState::Idle)
		, NewState(ENPCState::Idle)
		, NPCShipName(NAME_None)
	{
	}
};

/**
 * Delegate for state changes that C++ subscribers can bind to directly
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNPCStateChangedDelegate, ENPCState, OldState, ENPCState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNPCTargetChangedDelegate, AOdysseyCharacter*, NewTarget);

/**
 * NPC Behavior Component
 * Manages AI state machine and behavior for NPCs
 *
 * Architecture:
 * - 4-state machine: Idle -> Patrolling -> Engaging -> Dead
 * - Event-driven transitions via OdysseyEventBus
 * - Performance-tiered updates for mobile optimization
 * - Action system integration for NPC decision making
 * - Configurable detection, patrol, and combat parameters
 *
 * State Machine:
 * - Idle:       Stationary, scanning for player presence at detection frequency
 * - Patrolling: Following patrol routes, reactive to detection events
 * - Engaging:   In combat, attack on cooldown, tracks target distance
 * - Dead:       No updates, waiting for respawn signal from owning ship
 */
UCLASS(ClassGroup=(Odyssey), meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UNPCBehaviorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPCBehaviorComponent();

protected:
	// === Core State Management ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI State")
	ENPCState CurrentState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI State")
	ENPCState PreviousState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI State")
	float StateChangeTime;

	// === Combat Configuration ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DetectionRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float EngagementRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DisengagementRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackCooldown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bIsHostile;

	// === Patrol Configuration ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	FNPCPatrolConfig PatrolConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Patrol")
	int32 CurrentPatrolIndex;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Patrol")
	float PatrolWaitTimer;

	// === Current Engagement Data ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	FNPCEngagementData EngagementData;

	// === Performance Tier Settings ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	FNPCBehaviorPerformanceSettings HighTierSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	FNPCBehaviorPerformanceSettings MediumTierSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	FNPCBehaviorPerformanceSettings LowTierSettings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Performance")
	EPerformanceTier CurrentPerformanceTier;

	// === Active Performance Settings (resolved from tier) ===
	FNPCBehaviorPerformanceSettings ActivePerformanceSettings;

	// === Internal Timers ===
	float LastUpdateTime;
	float LastDetectionTime;

	// === Cached References ===
	UPROPERTY()
	ANPCShip* OwnerNPC;

	UPROPERTY()
	UOdysseyEventBus* EventBus;

	// === Event Handles for cleanup ===
	TArray<FOdysseyEventHandle> EventSubscriptionHandles;

public:
	// === Initialization ===
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// === State Management ===
	UFUNCTION(BlueprintCallable, Category = "AI State")
	void ChangeState(ENPCState NewState);

	UFUNCTION(BlueprintCallable, Category = "AI State")
	ENPCState GetCurrentState() const { return CurrentState; }

	UFUNCTION(BlueprintCallable, Category = "AI State")
	ENPCState GetPreviousState() const { return PreviousState; }

	UFUNCTION(BlueprintCallable, Category = "AI State")
	float GetTimeInCurrentState() const;

	UFUNCTION(BlueprintCallable, Category = "AI State")
	FString GetStateDisplayName() const;

	// === Combat System ===
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetTarget(AOdysseyCharacter* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	AOdysseyCharacter* GetCurrentTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool HasValidTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool IsTargetInRange() const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ClearTarget();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	float GetDistanceToTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool CanAttack() const;

	// === Patrol System ===
	UFUNCTION(BlueprintCallable, Category = "Patrol")
	void SetPatrolPoints(const TArray<FVector>& NewPatrolPoints);

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	FVector GetCurrentPatrolTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	void AdvanceToNextPatrolPoint();

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	bool HasPatrolRoute() const { return PatrolConfig.PatrolPoints.Num() > 0; }

	// === Detection System ===
	UFUNCTION(BlueprintCallable, Category = "Detection")
	AOdysseyCharacter* FindNearestHostileTarget();

	UFUNCTION(BlueprintCallable, Category = "Detection")
	bool IsActorInDetectionRange(AActor* Actor) const;

	// === Performance Tier Management ===
	UFUNCTION(BlueprintCallable, Category = "Performance")
	void SetPerformanceTier(EPerformanceTier NewTier);

	UFUNCTION(BlueprintCallable, Category = "Performance")
	EPerformanceTier GetPerformanceTier() const { return CurrentPerformanceTier; }

	UFUNCTION(BlueprintCallable, Category = "Performance")
	float GetEffectiveDetectionRadius() const;

	// === Configuration ===
	UFUNCTION(BlueprintCallable, Category = "Configuration")
	void SetHostile(bool bNewHostile) { bIsHostile = bNewHostile; }

	UFUNCTION(BlueprintCallable, Category = "Configuration")
	bool IsHostile() const { return bIsHostile; }

	// === Delegates ===
	UPROPERTY(BlueprintAssignable, Category = "AI Events")
	FNPCStateChangedDelegate OnNPCStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "AI Events")
	FNPCTargetChangedDelegate OnNPCTargetChanged;

	// === Blueprint Events ===
	UFUNCTION(BlueprintImplementableEvent, Category = "AI Events")
	void OnStateChanged(ENPCState OldState, ENPCState NewState);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Events")
	void OnTargetAcquired(AOdysseyCharacter* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Events")
	void OnTargetLost(AOdysseyCharacter* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Events")
	void OnEngagementStarted(AOdysseyCharacter* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Events")
	void OnEngagementEnded(AOdysseyCharacter* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "Patrol Events")
	void OnPatrolPointReached(int32 PatrolIndex, FVector PatrolPoint);

protected:
	// === State Handlers ===
	void UpdateIdleState(float DeltaTime);
	void UpdatePatrolState(float DeltaTime);
	void UpdateEngagingState(float DeltaTime);
	void UpdateDeadState(float DeltaTime);

	// === State Transition Logic ===
	void EnterState(ENPCState NewState);
	void ExitState(ENPCState OldState);

	// === Internal Logic ===
	void PerformDetectionUpdate();
	void UpdateEngagementData();
	void MoveTowardsTarget(const FVector& TargetLocation, float Speed);
	bool ShouldDisengage() const;
	void ExecuteAttack();

	// === Performance Optimization ===
	bool ShouldUpdate(float DeltaTime) const;
	bool ShouldUpdateDetection(float DeltaTime) const;
	void ApplyPerformanceSettings();
	const FNPCBehaviorPerformanceSettings& GetSettingsForTier(EPerformanceTier Tier) const;

	// === Event Bus Integration ===
	void InitializeEventSubscriptions();
	void CleanupEventSubscriptions();
	void BroadcastStateChangeEvent(ENPCState OldState, ENPCState NewState);
	void OnDamageReceivedEvent(const FOdysseyEventPayload& Payload);
	void OnPerformanceTierChangedEvent(const FOdysseyEventPayload& Payload);

private:
	// === Utility Functions ===
	float GetDistanceToLocation(const FVector& Location) const;
	FVector GetOwnerLocation() const;
};
