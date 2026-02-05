#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "NPCBehaviorComponent.generated.h"

// Forward declarations
class ANPCShip;
class AOdysseyCharacter;

/**
 * NPC AI State enumeration
 * Simple state machine for combat-ready NPCs
 */
UENUM(BlueprintType)
enum class ENPCState : uint8
{
	Idle,			// Default state - ship is stationary or drifting
	Patrolling,		// Moving along patrol routes
	Engaging,		// In combat with a target
	Dead			// Ship is destroyed/disabled
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
	AOdysseyCharacter* Target;

	UPROPERTY(BlueprintReadOnly, Category = "Engagement")
	float EngagementStartTime;

	UPROPERTY(BlueprintReadOnly, Category = "Engagement")
	float LastAttackTime;

	UPROPERTY(BlueprintReadOnly, Category = "Engagement")
	float DistanceToTarget;

	FNPCEngagementData()
	{
		Target = nullptr;
		EngagementStartTime = 0.0f;
		LastAttackTime = 0.0f;
		DistanceToTarget = 0.0f;
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
	{
		PatrolSpeed = 300.0f;
		PatrolRadius = 100.0f;
		bLoopPatrol = true;
		WaitTimeAtPoint = 2.0f;
	}
};

/**
 * NPC Behavior Component
 * Manages AI state machine and behavior for NPCs
 * Designed for performance on mobile platforms
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
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

	// === Performance Optimization ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	float UpdateFrequency;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	float DetectionUpdateFrequency;

	// === Internal Timers ===
	float LastUpdateTime;
	float LastDetectionTime;

	// === Cached References ===
	UPROPERTY()
	ANPCShip* OwnerNPC;

public:
	// === Initialization ===
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// === State Management ===
	UFUNCTION(BlueprintCallable, Category = "AI State")
	void ChangeState(ENPCState NewState);

	UFUNCTION(BlueprintCallable, Category = "AI State")
	ENPCState GetCurrentState() const { return CurrentState; }

	UFUNCTION(BlueprintCallable, Category = "AI State")
	float GetTimeInCurrentState() const;

	// === Combat System ===
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetTarget(AOdysseyCharacter* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	AOdysseyCharacter* GetCurrentTarget() const { return EngagementData.Target; }

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool HasValidTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool IsTargetInRange() const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ClearTarget();

	// === Patrol System ===
	UFUNCTION(BlueprintCallable, Category = "Patrol")
	void SetPatrolPoints(const TArray<FVector>& NewPatrolPoints);

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	FVector GetCurrentPatrolTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	void AdvanceToNextPatrolPoint();

	// === Detection System ===
	UFUNCTION(BlueprintCallable, Category = "Detection")
	AOdysseyCharacter* FindNearestPlayer();

	UFUNCTION(BlueprintCallable, Category = "Detection")
	bool IsPlayerInDetectionRange(AOdysseyCharacter* Player) const;

	// === Events ===
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

	// === Configuration ===
	UFUNCTION(BlueprintCallable, Category = "Configuration")
	void SetHostile(bool bNewHostile) { bIsHostile = bNewHostile; }

	UFUNCTION(BlueprintCallable, Category = "Configuration")
	bool IsHostile() const { return bIsHostile; }

protected:
	// === State Handlers ===
	void UpdateIdleState(float DeltaTime);
	void UpdatePatrolState(float DeltaTime);
	void UpdateEngagingState(float DeltaTime);
	void UpdateDeadState(float DeltaTime);

	// === Internal Logic ===
	void PerformDetectionUpdate();
	void UpdateEngagementData();
	void MoveTowardsTarget(const FVector& TargetLocation, float Speed);
	bool ShouldDisengage() const;

	// === Performance Optimization ===
	bool ShouldUpdate(float DeltaTime) const;
	bool ShouldUpdateDetection(float DeltaTime) const;

private:
	// === Utility Functions ===
	float GetDistanceToLocation(const FVector& Location) const;
	FVector GetOwnerLocation() const;
	void ResetEngagementData();
};
