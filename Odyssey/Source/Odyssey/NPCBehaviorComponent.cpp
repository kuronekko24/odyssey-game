// NPCBehaviorComponent.cpp
// AI state machine implementation with performance-tiered updates
// Event-driven state transitions via OdysseyEventBus

#include "NPCBehaviorComponent.h"
#include "NPCShip.h"
#include "OdysseyCharacter.h"
#include "OdysseyEventBus.h"
#include "OdysseyActionDispatcher.h"
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

	// Performance tier defaults
	CurrentPerformanceTier = EPerformanceTier::High;

	// High tier: full fidelity
	HighTierSettings.UpdateFrequency = 10.0f;
	HighTierSettings.DetectionUpdateFrequency = 3.0f;
	HighTierSettings.bEnablePatrolling = true;
	HighTierSettings.bEnableLineOfSightChecks = true;
	HighTierSettings.DetectionRangeMultiplier = 1.0f;

	// Medium tier: reduced fidelity
	MediumTierSettings.UpdateFrequency = 5.0f;
	MediumTierSettings.DetectionUpdateFrequency = 1.5f;
	MediumTierSettings.bEnablePatrolling = true;
	MediumTierSettings.bEnableLineOfSightChecks = false;
	MediumTierSettings.DetectionRangeMultiplier = 0.8f;

	// Low tier: minimal updates
	LowTierSettings.UpdateFrequency = 2.0f;
	LowTierSettings.DetectionUpdateFrequency = 0.5f;
	LowTierSettings.bEnablePatrolling = false;
	LowTierSettings.bEnableLineOfSightChecks = false;
	LowTierSettings.DetectionRangeMultiplier = 0.5f;

	// Start with high tier active
	ActivePerformanceSettings = HighTierSettings;

	LastUpdateTime = 0.0f;
	LastDetectionTime = 0.0f;
	OwnerNPC = nullptr;
	EventBus = nullptr;
}

void UNPCBehaviorComponent::BeginPlay()
{
	Super::BeginPlay();

	// Cache owner reference
	OwnerNPC = Cast<ANPCShip>(GetOwner());

	if (!OwnerNPC)
	{
		UE_LOG(LogTemp, Warning, TEXT("NPCBehaviorComponent: Owner is not an NPCShip. AI behavior disabled."));
		PrimaryComponentTick.bCanEverTick = false;
		return;
	}

	// Initialize event bus connection
	EventBus = UOdysseyEventBus::Get();
	InitializeEventSubscriptions();

	// Initialize state change time
	StateChangeTime = GetWorld()->GetTimeSeconds();

	// Apply current performance tier settings
	ApplyPerformanceSettings();

	// If we have patrol points, start patrolling
	if (PatrolConfig.PatrolPoints.Num() > 0 && ActivePerformanceSettings.bEnablePatrolling)
	{
		ChangeState(ENPCState::Patrolling);
	}

	UE_LOG(LogTemp, Log, TEXT("NPCBehaviorComponent initialized for: %s (Tier: %d)"),
		*OwnerNPC->GetName(),
		static_cast<int32>(CurrentPerformanceTier));
}

void UNPCBehaviorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CleanupEventSubscriptions();
	Super::EndPlay(EndPlayReason);
}

void UNPCBehaviorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Dead state: no updates at all to save cycles
	if (CurrentState == ENPCState::Dead)
	{
		return;
	}

	// Performance optimization: limit update frequency based on tier
	if (!ShouldUpdate(DeltaTime))
	{
		return;
	}

	// Update detection system at lower frequency than state updates
	if (ShouldUpdateDetection(DeltaTime))
	{
		PerformDetectionUpdate();
		LastDetectionTime = GetWorld()->GetTimeSeconds();
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
			// Already handled above, but kept for completeness
			break;
	}

	LastUpdateTime = GetWorld()->GetTimeSeconds();
}

// ============================================================================
// State Management
// ============================================================================

void UNPCBehaviorComponent::ChangeState(ENPCState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	ENPCState OldState = CurrentState;

	// Exit current state
	ExitState(OldState);

	// Transition
	PreviousState = OldState;
	CurrentState = NewState;
	StateChangeTime = GetWorld()->GetTimeSeconds();

	// Enter new state
	EnterState(NewState);

	// Broadcast via event bus
	BroadcastStateChangeEvent(OldState, NewState);

	// Fire multicast delegates
	OnNPCStateChanged.Broadcast(OldState, NewState);

	// Fire Blueprint implementable event
	OnStateChanged(OldState, NewState);

	UE_LOG(LogTemp, Log, TEXT("NPCBehaviorComponent: %s state %d -> %d"),
		OwnerNPC ? *OwnerNPC->GetName() : TEXT("Unknown"),
		static_cast<int32>(OldState), static_cast<int32>(NewState));
}

void UNPCBehaviorComponent::EnterState(ENPCState NewState)
{
	switch (NewState)
	{
		case ENPCState::Idle:
			// Nothing special on idle entry
			break;

		case ENPCState::Patrolling:
			PatrolWaitTimer = 0.0f;
			break;

		case ENPCState::Engaging:
			EngagementData.EngagementStartTime = GetWorld()->GetTimeSeconds();
			if (AOdysseyCharacter* Target = GetCurrentTarget())
			{
				OnEngagementStarted(Target);
			}
			break;

		case ENPCState::Dead:
			EngagementData.Reset();
			// Stop all movement immediately
			if (OwnerNPC)
			{
				if (UCharacterMovementComponent* MovementComp = OwnerNPC->GetCharacterMovement())
				{
					MovementComp->StopMovementImmediately();
				}
			}
			break;
	}
}

void UNPCBehaviorComponent::ExitState(ENPCState OldState)
{
	switch (OldState)
	{
		case ENPCState::Engaging:
			if (AOdysseyCharacter* Target = GetCurrentTarget())
			{
				OnEngagementEnded(Target);
			}
			break;

		case ENPCState::Patrolling:
			// Nothing special
			break;

		case ENPCState::Idle:
			// Nothing special
			break;

		case ENPCState::Dead:
			// Nothing special - respawn is handled by ANPCShip
			break;
	}
}

float UNPCBehaviorComponent::GetTimeInCurrentState() const
{
	if (!GetWorld())
	{
		return 0.0f;
	}
	return GetWorld()->GetTimeSeconds() - StateChangeTime;
}

FString UNPCBehaviorComponent::GetStateDisplayName() const
{
	switch (CurrentState)
	{
		case ENPCState::Idle:       return TEXT("Idle");
		case ENPCState::Patrolling: return TEXT("Patrolling");
		case ENPCState::Engaging:   return TEXT("Engaging");
		case ENPCState::Dead:       return TEXT("Dead");
		default:                    return TEXT("Unknown");
	}
}

// ============================================================================
// Combat System
// ============================================================================

void UNPCBehaviorComponent::SetTarget(AOdysseyCharacter* NewTarget)
{
	AOdysseyCharacter* OldTarget = GetCurrentTarget();

	if (OldTarget == NewTarget)
	{
		return;
	}

	if (OldTarget && !NewTarget)
	{
		OnTargetLost(OldTarget);
	}

	EngagementData.Target = NewTarget;

	if (NewTarget)
	{
		OnTargetAcquired(NewTarget);
		UpdateEngagementData();
	}

	// Fire delegate
	OnNPCTargetChanged.Broadcast(NewTarget);
}

AOdysseyCharacter* UNPCBehaviorComponent::GetCurrentTarget() const
{
	return EngagementData.Target.IsValid() ? EngagementData.Target.Get() : nullptr;
}

bool UNPCBehaviorComponent::HasValidTarget() const
{
	AOdysseyCharacter* Target = GetCurrentTarget();
	if (!Target || !IsValid(Target))
	{
		return false;
	}

	// Check if target NPC is alive
	if (ANPCShip* TargetNPC = Cast<ANPCShip>(Target))
	{
		return TargetNPC->IsAlive();
	}

	return true;
}

bool UNPCBehaviorComponent::IsTargetInRange() const
{
	if (!HasValidTarget())
	{
		return false;
	}

	return EngagementData.DistanceToTarget <= EngagementRange;
}

void UNPCBehaviorComponent::ClearTarget()
{
	AOdysseyCharacter* OldTarget = GetCurrentTarget();
	if (OldTarget)
	{
		OnTargetLost(OldTarget);
	}
	EngagementData.Reset();
	OnNPCTargetChanged.Broadcast(nullptr);
}

float UNPCBehaviorComponent::GetDistanceToTarget() const
{
	return EngagementData.DistanceToTarget;
}

bool UNPCBehaviorComponent::CanAttack() const
{
	if (!HasValidTarget() || !IsTargetInRange() || CurrentState != ENPCState::Engaging)
	{
		return false;
	}

	if (!GetWorld())
	{
		return false;
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();
	return (CurrentTime - EngagementData.LastAttackTime) >= AttackCooldown;
}

// ============================================================================
// Patrol System
// ============================================================================

void UNPCBehaviorComponent::SetPatrolPoints(const TArray<FVector>& NewPatrolPoints)
{
	PatrolConfig.PatrolPoints = NewPatrolPoints;
	CurrentPatrolIndex = 0;
	PatrolWaitTimer = 0.0f;

	// If we were idle and now have patrol points, start patrolling
	if (CurrentState == ENPCState::Idle && PatrolConfig.PatrolPoints.Num() > 0
		&& ActivePerformanceSettings.bEnablePatrolling)
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

// ============================================================================
// Detection System
// ============================================================================

AOdysseyCharacter* UNPCBehaviorComponent::FindNearestHostileTarget()
{
	if (!GetWorld())
	{
		return nullptr;
	}

	AOdysseyCharacter* NearestTarget = nullptr;
	float EffectiveRadius = GetEffectiveDetectionRadius();
	float NearestDistance = EffectiveRadius + 1.0f;
	FVector OwnerLoc = GetOwnerLocation();

	// Find all characters in the world
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AOdysseyCharacter::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		AOdysseyCharacter* Character = Cast<AOdysseyCharacter>(Actor);
		if (!Character || Character == OwnerNPC)
		{
			continue;
		}

		// Skip other dead NPC ships
		if (ANPCShip* OtherNPC = Cast<ANPCShip>(Character))
		{
			if (!OtherNPC->IsAlive())
			{
				continue;
			}
		}

		float Distance = FVector::Dist(OwnerLoc, Character->GetActorLocation());
		if (Distance < EffectiveRadius && Distance < NearestDistance)
		{
			NearestTarget = Character;
			NearestDistance = Distance;
		}
	}

	return NearestTarget;
}

bool UNPCBehaviorComponent::IsActorInDetectionRange(AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	float Distance = GetDistanceToLocation(Actor->GetActorLocation());
	return Distance <= GetEffectiveDetectionRadius();
}

// ============================================================================
// Performance Tier Management
// ============================================================================

void UNPCBehaviorComponent::SetPerformanceTier(EPerformanceTier NewTier)
{
	if (CurrentPerformanceTier == NewTier)
	{
		return;
	}

	EPerformanceTier OldTier = CurrentPerformanceTier;
	CurrentPerformanceTier = NewTier;
	ApplyPerformanceSettings();

	UE_LOG(LogTemp, Log, TEXT("NPCBehaviorComponent %s: Performance tier %d -> %d"),
		OwnerNPC ? *OwnerNPC->GetName() : TEXT("Unknown"),
		static_cast<int32>(OldTier), static_cast<int32>(NewTier));

	// If patrolling was disabled at new tier, transition to idle
	if (!ActivePerformanceSettings.bEnablePatrolling && CurrentState == ENPCState::Patrolling)
	{
		ChangeState(ENPCState::Idle);
	}
}

float UNPCBehaviorComponent::GetEffectiveDetectionRadius() const
{
	return DetectionRadius * ActivePerformanceSettings.DetectionRangeMultiplier;
}

void UNPCBehaviorComponent::ApplyPerformanceSettings()
{
	ActivePerformanceSettings = GetSettingsForTier(CurrentPerformanceTier);
}

const FNPCBehaviorPerformanceSettings& UNPCBehaviorComponent::GetSettingsForTier(EPerformanceTier Tier) const
{
	switch (Tier)
	{
		case EPerformanceTier::High:   return HighTierSettings;
		case EPerformanceTier::Medium: return MediumTierSettings;
		case EPerformanceTier::Low:    return LowTierSettings;
		default:                       return MediumTierSettings;
	}
}

// ============================================================================
// State Handlers
// ============================================================================

void UNPCBehaviorComponent::UpdateIdleState(float DeltaTime)
{
	// In idle: check if we should start patrolling
	if (HasPatrolRoute() && ActivePerformanceSettings.bEnablePatrolling && !HasValidTarget())
	{
		ChangeState(ENPCState::Patrolling);
	}
	// Detection is handled in PerformDetectionUpdate
}

void UNPCBehaviorComponent::UpdatePatrolState(float DeltaTime)
{
	if (!ActivePerformanceSettings.bEnablePatrolling)
	{
		ChangeState(ENPCState::Idle);
		return;
	}

	// Handle patrol waiting at waypoint
	if (PatrolWaitTimer > 0.0f)
	{
		PatrolWaitTimer -= DeltaTime;
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
		// Reached patrol point - start waiting then advance
		PatrolWaitTimer = PatrolConfig.WaitTimeAtPoint;
		AdvanceToNextPatrolPoint();
	}
}

void UNPCBehaviorComponent::UpdateEngagingState(float DeltaTime)
{
	if (!HasValidTarget())
	{
		// Target lost, return to previous non-combat state
		ENPCState ReturnState = (PreviousState != ENPCState::Engaging && PreviousState != ENPCState::Dead)
			? PreviousState
			: ENPCState::Idle;
		ClearTarget();
		ChangeState(ReturnState);
		return;
	}

	UpdateEngagementData();

	// Check if we should disengage (target too far)
	if (ShouldDisengage())
	{
		ENPCState ReturnState = (PreviousState != ENPCState::Engaging && PreviousState != ENPCState::Dead)
			? PreviousState
			: ENPCState::Idle;
		ClearTarget();
		ChangeState(ReturnState);
		return;
	}

	AOdysseyCharacter* Target = GetCurrentTarget();

	// Move towards target if not in engagement range
	if (!IsTargetInRange())
	{
		// Approach at 120% patrol speed for urgency
		MoveTowardsTarget(Target->GetActorLocation(), PatrolConfig.PatrolSpeed * 1.2f);
	}
	else
	{
		// In range: execute attack if cooldown allows
		if (CanAttack())
		{
			ExecuteAttack();
		}
	}
}

void UNPCBehaviorComponent::UpdateDeadState(float DeltaTime)
{
	// Dead NPCs do nothing. Respawn is triggered externally by ANPCShip.
}

// ============================================================================
// Internal Logic
// ============================================================================

void UNPCBehaviorComponent::PerformDetectionUpdate()
{
	if (!bIsHostile || CurrentState == ENPCState::Dead)
	{
		return;
	}

	// Already engaging something - check if we should switch targets
	if (CurrentState == ENPCState::Engaging)
	{
		if (!HasValidTarget())
		{
			// Current target lost, try to find new one
			AOdysseyCharacter* NewTarget = FindNearestHostileTarget();
			if (NewTarget)
			{
				SetTarget(NewTarget);
			}
			else
			{
				ClearTarget();
				ChangeState(ENPCState::Idle);
			}
		}
		return;
	}

	// Not in combat: scan for targets
	AOdysseyCharacter* NearestTarget = FindNearestHostileTarget();
	if (NearestTarget)
	{
		SetTarget(NearestTarget);
		ChangeState(ENPCState::Engaging);
	}
}

void UNPCBehaviorComponent::UpdateEngagementData()
{
	AOdysseyCharacter* Target = GetCurrentTarget();
	if (!Target)
	{
		return;
	}

	EngagementData.DistanceToTarget = GetDistanceToLocation(Target->GetActorLocation());
}

void UNPCBehaviorComponent::MoveTowardsTarget(const FVector& TargetLocation, float Speed)
{
	if (!OwnerNPC)
	{
		return;
	}

	FVector OwnerLoc = GetOwnerLocation();
	FVector Direction = (TargetLocation - OwnerLoc).GetSafeNormal();

	if (Direction.IsNearlyZero())
	{
		return;
	}

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

	return EngagementData.DistanceToTarget > DisengagementRange;
}

void UNPCBehaviorComponent::ExecuteAttack()
{
	if (!OwnerNPC || !HasValidTarget())
	{
		return;
	}

	AOdysseyCharacter* Target = GetCurrentTarget();

	// Record attack timing
	EngagementData.LastAttackTime = GetWorld()->GetTimeSeconds();
	EngagementData.AttackCount++;

	// Delegate actual attack execution to the owning ship
	// This keeps combat logic in ANPCShip where it has access to config/damage values
	OwnerNPC->AttackTarget(Target);

	// Broadcast attack event through event bus
	if (EventBus)
	{
		auto AttackPayload = MakeShared<FCombatEventPayload>();
		AttackPayload->Initialize(EOdysseyEventType::AttackStarted, OwnerNPC, EOdysseyEventPriority::Normal);
		AttackPayload->Attacker = OwnerNPC;
		AttackPayload->Target = Target;
		AttackPayload->DamageAmount = OwnerNPC->GetShipConfig().AttackDamage;
		EventBus->PublishEvent(AttackPayload);
	}

	UE_LOG(LogTemp, Log, TEXT("NPCBehaviorComponent: %s attacked %s (attack #%d)"),
		*OwnerNPC->GetName(), *Target->GetName(), EngagementData.AttackCount);
}

// ============================================================================
// Performance Optimization
// ============================================================================

bool UNPCBehaviorComponent::ShouldUpdate(float DeltaTime) const
{
	if (!GetWorld())
	{
		return false;
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();
	float UpdateInterval = 1.0f / FMath::Max(ActivePerformanceSettings.UpdateFrequency, 0.1f);
	return (CurrentTime - LastUpdateTime) >= UpdateInterval;
}

bool UNPCBehaviorComponent::ShouldUpdateDetection(float DeltaTime) const
{
	if (!GetWorld())
	{
		return false;
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();
	float DetectionInterval = 1.0f / FMath::Max(ActivePerformanceSettings.DetectionUpdateFrequency, 0.1f);
	return (CurrentTime - LastDetectionTime) >= DetectionInterval;
}

// ============================================================================
// Event Bus Integration
// ============================================================================

void UNPCBehaviorComponent::InitializeEventSubscriptions()
{
	if (!EventBus)
	{
		return;
	}

	// Subscribe to damage events (so we can react when our ship takes damage)
	FOdysseyEventFilter DamageFilter;
	DamageFilter.AllowedEventTypes.Add(EOdysseyEventType::DamageReceived);

	FOdysseyEventHandle DamageHandle = EventBus->Subscribe(
		EOdysseyEventType::DamageReceived,
		[this](const FOdysseyEventPayload& Payload) { OnDamageReceivedEvent(Payload); },
		DamageFilter
	);
	EventSubscriptionHandles.Add(DamageHandle);

	UE_LOG(LogTemp, Log, TEXT("NPCBehaviorComponent: Event subscriptions initialized for %s"),
		OwnerNPC ? *OwnerNPC->GetName() : TEXT("Unknown"));
}

void UNPCBehaviorComponent::CleanupEventSubscriptions()
{
	if (!EventBus)
	{
		return;
	}

	for (FOdysseyEventHandle& Handle : EventSubscriptionHandles)
	{
		EventBus->Unsubscribe(Handle);
	}
	EventSubscriptionHandles.Empty();
}

void UNPCBehaviorComponent::BroadcastStateChangeEvent(ENPCState OldState, ENPCState NewState)
{
	if (!EventBus)
	{
		return;
	}

	// Use the CustomEventStart range for NPC-specific events
	auto Payload = MakeShared<FNPCStateChangeEventPayload>();
	Payload->Initialize(EOdysseyEventType::CustomEventStart, OwnerNPC, EOdysseyEventPriority::Normal);
	Payload->PreviousState = OldState;
	Payload->NewState = NewState;
	Payload->NPCShipName = OwnerNPC ? FName(*OwnerNPC->GetName()) : NAME_None;

	if (HasValidTarget())
	{
		Payload->EngagementTarget = GetCurrentTarget();
	}

	EventBus->PublishEvent(Payload);
}

void UNPCBehaviorComponent::OnDamageReceivedEvent(const FOdysseyEventPayload& Payload)
{
	// Check if the damage event targets our owner
	if (!OwnerNPC || !Payload.Source.IsValid())
	{
		return;
	}

	// If we are idle or patrolling and we took damage, find the attacker and engage
	if (CurrentState == ENPCState::Idle || CurrentState == ENPCState::Patrolling)
	{
		AActor* DamageSource = Payload.Source.Get();
		if (AOdysseyCharacter* Attacker = Cast<AOdysseyCharacter>(DamageSource))
		{
			if (IsActorInDetectionRange(Attacker))
			{
				SetTarget(Attacker);
				ChangeState(ENPCState::Engaging);

				UE_LOG(LogTemp, Log, TEXT("NPCBehaviorComponent: %s reactive engagement from damage by %s"),
					*OwnerNPC->GetName(), *Attacker->GetName());
			}
		}
	}
}

void UNPCBehaviorComponent::OnPerformanceTierChangedEvent(const FOdysseyEventPayload& Payload)
{
	// This would be called when the global performance tier changes
	// The actual tier value would come from the payload or from querying the optimizer
}

// ============================================================================
// Utility Functions
// ============================================================================

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
