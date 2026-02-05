#include "NPCBehaviorComponent.h"
#include "NPCShip.h"
#include "OdysseyCharacter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"

UNPCBehaviorComponent::UNPCBehaviorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Initialize state
	CurrentState = ENPCState::Idle;
	PreviousState = ENPCState::Idle;
	StateChangeTime = 0.0f;

	// Combat configuration
	DetectionRadius = 1000.0f;
	EngagementRange = 800.0f;
	DisengagementRange = 1200.0f;
	AttackCooldown = 2.0f;
	bIsHostile = true;

	// Patrol initialization
	CurrentPatrolIndex = 0;
	PatrolWaitTimer = 0.0f;

	// Performance settings (mobile optimized)
	UpdateFrequency = 10.0f;  // 10 Hz update rate
	DetectionUpdateFrequency = 2.0f;  // 2 Hz detection updates

	LastUpdateTime = 0.0f;
	LastDetectionTime = 0.0f;
	OwnerNPC = nullptr;
}

void UNPCBehaviorComponent::BeginPlay()
{
	Super::BeginPlay();

	// Cache owner reference
	OwnerNPC = Cast<ANPCShip>(GetOwner());
	
	if (!OwnerNPC)
	{
		UE_LOG(LogTemp, Warning, TEXT("NPCBehaviorComponent: Owner is not an NPCShip!"));
	}

	// Initialize state change time
	StateChangeTime = GetWorld()->GetTimeSeconds();

	// Initialize patrol if we have patrol points
	if (PatrolConfig.PatrolPoints.Num() > 0)
	{
		ChangeState(ENPCState::Patrolling);
	}

	UE_LOG(LogTemp, Warning, TEXT("NPCBehaviorComponent initialized for: %s"), 
		OwnerNPC ? *OwnerNPC->GetName() : TEXT("Unknown"));
}

void UNPCBehaviorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Performance optimization: limit update frequency
	if (!ShouldUpdate(DeltaTime))
	{
		return;
	}

	// Update detection system at lower frequency
	if (ShouldUpdateDetection(DeltaTime))
	{
		PerformDetectionUpdate();
	}

	// Update current state
	switch (CurrentState)
	{
		case ENPCState::Idle:
			UpdateIdleState(DeltaTime);
			break;
		case ENPCState::Patrolling:
			UpdatePatrolState(DeltaTime);
			break;
		case ENPCState::Engaging:
			UpdateEngagingState(DeltaTime);
			break;
		case ENPCState::Dead:
			UpdateDeadState(DeltaTime);
			break;
	}

	LastUpdateTime = GetWorld()->GetTimeSeconds();
}

void UNPCBehaviorComponent::ChangeState(ENPCState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	PreviousState = CurrentState;
	CurrentState = NewState;
	StateChangeTime = GetWorld()->GetTimeSeconds();

	// Handle state-specific logic
	switch (NewState)
	{
		case ENPCState::Engaging:
			OnEngagementStarted(EngagementData.Target);
			EngagementData.EngagementStartTime = StateChangeTime;
			break;
		case ENPCState::Idle:
			if (PreviousState == ENPCState::Engaging)
			{
				OnEngagementEnded(EngagementData.Target);
				ResetEngagementData();
			}
			break;
		case ENPCState::Dead:
			ResetEngagementData();
			break;
		case ENPCState::Patrolling:
			ResetEngagementData();
			break;
	}

	// Fire event
	OnStateChanged(PreviousState, NewState);

	UE_LOG(LogTemp, Log, TEXT("NPCBehaviorComponent: %s changed state from %d to %d"), 
		OwnerNPC ? *OwnerNPC->GetName() : TEXT("Unknown"), 
		(int32)PreviousState, (int32)NewState);
}

float UNPCBehaviorComponent::GetTimeInCurrentState() const
{
	return GetWorld()->GetTimeSeconds() - StateChangeTime;
}

void UNPCBehaviorComponent::SetTarget(AOdysseyCharacter* NewTarget)
{
	if (EngagementData.Target != NewTarget)
	{
		AOdysseyCharacter* OldTarget = EngagementData.Target;
		EngagementData.Target = NewTarget;

		if (OldTarget && !NewTarget)
		{
			OnTargetLost(OldTarget);
		}
		else if (NewTarget)
		{
			OnTargetAcquired(NewTarget);
			UpdateEngagementData();
		}
	}
}

bool UNPCBehaviorComponent::HasValidTarget() const
{
	return EngagementData.Target && IsValid(EngagementData.Target) && !EngagementData.Target->IsPendingKill();
}

bool UNPCBehaviorComponent::IsTargetInRange() const
{
	if (!HasValidTarget())
	{
		return false;
	}

	float Distance = GetDistanceToLocation(EngagementData.Target->GetActorLocation());
	return Distance <= EngagementRange;
}

void UNPCBehaviorComponent::ClearTarget()
{
	if (HasValidTarget())
	{
		OnTargetLost(EngagementData.Target);
	}
	ResetEngagementData();
}

void UNPCBehaviorComponent::SetPatrolPoints(const TArray<FVector>& NewPatrolPoints)
{
	PatrolConfig.PatrolPoints = NewPatrolPoints;
	CurrentPatrolIndex = 0;
	PatrolWaitTimer = 0.0f;

	// If we were idle and now have patrol points, start patrolling
	if (CurrentState == ENPCState::Idle && PatrolConfig.PatrolPoints.Num() > 0)
	{
		ChangeState(ENPCState::Patrolling);
	}
}

FVector UNPCBehaviorComponent::GetCurrentPatrolTarget() const
{
	if (PatrolConfig.PatrolPoints.Num() == 0)
	{
		return GetOwnerLocation();
	}

	int32 SafeIndex = FMath::Clamp(CurrentPatrolIndex, 0, PatrolConfig.PatrolPoints.Num() - 1);
	return PatrolConfig.PatrolPoints[SafeIndex];
}

void UNPCBehaviorComponent::AdvanceToNextPatrolPoint()
{
	if (PatrolConfig.PatrolPoints.Num() == 0)
	{
		return;
	}

	OnPatrolPointReached(CurrentPatrolIndex, GetCurrentPatrolTarget());

	CurrentPatrolIndex++;
	
	if (CurrentPatrolIndex >= PatrolConfig.PatrolPoints.Num())
	{
		if (PatrolConfig.bLoopPatrol)
		{
			CurrentPatrolIndex = 0;
		}
		else
		{
			CurrentPatrolIndex = PatrolConfig.PatrolPoints.Num() - 1;
			ChangeState(ENPCState::Idle);
		}
	}

	PatrolWaitTimer = 0.0f;
}

AOdysseyCharacter* UNPCBehaviorComponent::FindNearestPlayer()
{
	if (!GetWorld())
	{
		return nullptr;
	}

	AOdysseyCharacter* NearestPlayer = nullptr;
	float NearestDistance = DetectionRadius + 1.0f;
	FVector OwnerLoc = GetOwnerLocation();

	// Find all player characters
	TArray<AActor*> PlayerActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AOdysseyCharacter::StaticClass(), PlayerActors);

	for (AActor* Actor : PlayerActors)
	{
		AOdysseyCharacter* Player = Cast<AOdysseyCharacter>(Actor);
		if (Player && Player != OwnerNPC) // Don't target ourselves if we're also an OdysseyCharacter
		{
			float Distance = FVector::Dist(OwnerLoc, Player->GetActorLocation());
			if (Distance < DetectionRadius && Distance < NearestDistance)
			{
				NearestPlayer = Player;
				NearestDistance = Distance;
			}
		}
	}

	return NearestPlayer;
}

bool UNPCBehaviorComponent::IsPlayerInDetectionRange(AOdysseyCharacter* Player) const
{
	if (!Player)
	{
		return false;
	}

	float Distance = GetDistanceToLocation(Player->GetActorLocation());
	return Distance <= DetectionRadius;
}

void UNPCBehaviorComponent::UpdateIdleState(float DeltaTime)
{
	// In idle state, just perform detection
	// If we have patrol points and no target, start patrolling
	if (PatrolConfig.PatrolPoints.Num() > 0 && !HasValidTarget())
	{
		ChangeState(ENPCState::Patrolling);
	}
}

void UNPCBehaviorComponent::UpdatePatrolState(float DeltaTime)
{
	// Handle patrol waiting
	if (PatrolWaitTimer < PatrolConfig.WaitTimeAtPoint)
	{
		PatrolWaitTimer += DeltaTime;
		return;
	}

	FVector CurrentTarget = GetCurrentPatrolTarget();
	FVector OwnerLoc = GetOwnerLocation();
	float DistanceToTarget = FVector::Dist(OwnerLoc, CurrentTarget);

	// Move towards patrol point
	if (DistanceToTarget > PatrolConfig.PatrolRadius)
	{
		MoveTowardsTarget(CurrentTarget, PatrolConfig.PatrolSpeed);
	}
	else
	{
		// Reached patrol point
		AdvanceToNextPatrolPoint();
		PatrolWaitTimer = PatrolConfig.WaitTimeAtPoint; // Start waiting
	}
}

void UNPCBehaviorComponent::UpdateEngagingState(float DeltaTime)
{
	if (!HasValidTarget())
	{
		// No valid target, return to previous state
		ChangeState(PreviousState == ENPCState::Engaging ? ENPCState::Idle : PreviousState);
		return;
	}

	UpdateEngagementData();

	// Check if we should disengage
	if (ShouldDisengage())
	{
		ChangeState(PreviousState == ENPCState::Engaging ? ENPCState::Idle : PreviousState);
		return;
	}

	// Move towards target if not in range
	if (!IsTargetInRange())
	{
		MoveTowardsTarget(EngagementData.Target->GetActorLocation(), PatrolConfig.PatrolSpeed * 1.2f);
	}
	else
	{
		// In range - handle combat logic here
		float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - EngagementData.LastAttackTime >= AttackCooldown)
		{
			// Perform attack (this would trigger combat actions)
			EngagementData.LastAttackTime = CurrentTime;
			
			// This is where you would integrate with the action system
			UE_LOG(LogTemp, Log, TEXT("NPCBehaviorComponent: %s attacking target"), 
				OwnerNPC ? *OwnerNPC->GetName() : TEXT("Unknown"));
		}
	}
}

void UNPCBehaviorComponent::UpdateDeadState(float DeltaTime)
{
	// Dead NPCs don't do anything
	// This state is primarily for preventing further AI updates
}

void UNPCBehaviorComponent::PerformDetectionUpdate()
{
	if (!bIsHostile || CurrentState == ENPCState::Dead)
	{
		return;
	}

	// Find nearest player
	AOdysseyCharacter* NearestPlayer = FindNearestPlayer();

	if (NearestPlayer && CurrentState != ENPCState::Engaging)
	{
		// Found a target, engage
		SetTarget(NearestPlayer);
		ChangeState(ENPCState::Engaging);
	}
	else if (!NearestPlayer && CurrentState == ENPCState::Engaging)
	{
		// Lost target
		ClearTarget();
		ChangeState(ENPCState::Idle);
	}

	LastDetectionTime = GetWorld()->GetTimeSeconds();
}

void UNPCBehaviorComponent::UpdateEngagementData()
{
	if (!HasValidTarget())
	{
		return;
	}

	EngagementData.DistanceToTarget = GetDistanceToLocation(EngagementData.Target->GetActorLocation());
}

void UNPCBehaviorComponent::MoveTowardsTarget(const FVector& TargetLocation, float Speed)
{
	if (!OwnerNPC)
	{
		return;
	}

	FVector OwnerLoc = GetOwnerLocation();
	FVector Direction = (TargetLocation - OwnerLoc).GetSafeNormal();

	// Apply movement through character movement component
	if (UCharacterMovementComponent* MovementComp = OwnerNPC->GetCharacterMovement())
	{
		MovementComp->MaxWalkSpeed = Speed;
		OwnerNPC->AddMovementInput(Direction, 1.0f);
	}
}

bool UNPCBehaviorComponent::ShouldDisengage() const
{
	if (!HasValidTarget())
	{
		return true;
	}

	// Disengage if target is too far
	return EngagementData.DistanceToTarget > DisengagementRange;
}

bool UNPCBehaviorComponent::ShouldUpdate(float DeltaTime) const
{
	float CurrentTime = GetWorld()->GetTimeSeconds();
	return (CurrentTime - LastUpdateTime) >= (1.0f / UpdateFrequency);
}

bool UNPCBehaviorComponent::ShouldUpdateDetection(float DeltaTime) const
{
	float CurrentTime = GetWorld()->GetTimeSeconds();
	return (CurrentTime - LastDetectionTime) >= (1.0f / DetectionUpdateFrequency);
}

float UNPCBehaviorComponent::GetDistanceToLocation(const FVector& Location) const
{
	return FVector::Dist(GetOwnerLocation(), Location);
}

FVector UNPCBehaviorComponent::GetOwnerLocation() const
{
	if (OwnerNPC)
	{
		return OwnerNPC->GetActorLocation();
	}
	return FVector::ZeroVector;
}

void UNPCBehaviorComponent::ResetEngagementData()
{
	EngagementData.Target = nullptr;
	EngagementData.EngagementStartTime = 0.0f;
	EngagementData.LastAttackTime = 0.0f;
	EngagementData.DistanceToTarget = 0.0f;
}
