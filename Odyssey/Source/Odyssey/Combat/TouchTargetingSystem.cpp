// TouchTargetingSystem.cpp
// Phase 3: Touch-based targeting implementation

#include "TouchTargetingSystem.h"
#include "NPCHealthComponent.h"
#include "NPCBehaviorComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "CollisionQueryParams.h"
#include "EngineUtils.h"

UTouchTargetingSystem::UTouchTargetingSystem()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f; // 10 Hz -- sufficient for target tracking

	CurrentReticleState = EReticleState::Hidden;
	AutoTargetTimer = 0.0f;
	CachedCamera = nullptr;
}

void UTouchTargetingSystem::BeginPlay()
{
	Super::BeginPlay();
	CachedCamera = ResolveCamera();
}

void UTouchTargetingSystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearTarget();
	Super::EndPlay(EndPlayReason);
}

void UTouchTargetingSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Refresh existing target state
	if (HasValidTarget())
	{
		RefreshCurrentTarget();
	}

	// Auto-targeting scan
	TickAutoTargeting(DeltaTime);

	// Derive the visual reticle state
	UpdateReticleState();
}

// ============================================================================
// Touch Input
// ============================================================================

bool UTouchTargetingSystem::HandleTouch(FVector2D ScreenPosition)
{
	FVector WorldHit;
	AActor* HitActor = RaycastFromScreen(ScreenPosition, WorldHit);

	if (HitActor && ValidateTarget(HitActor))
	{
		OnTouchTargetHit.Broadcast(HitActor, ScreenPosition);
		return SelectTarget(HitActor, true); // already validated
	}

	// Touch hit empty space -- clear target
	ClearTarget();
	return false;
}

// ============================================================================
// Target Management
// ============================================================================

bool UTouchTargetingSystem::SelectTarget(AActor* TargetActor, bool bSkipValidation)
{
	if (!TargetActor)
	{
		ClearTarget();
		return false;
	}

	if (!bSkipValidation && !ValidateTarget(TargetActor))
	{
		return false;
	}

	FCombatTargetSnapshot Snapshot = BuildSnapshot(TargetActor);
	SetTargetInternal(Snapshot);
	return true;
}

void UTouchTargetingSystem::ClearTarget()
{
	if (CurrentTarget.IsValid())
	{
		FCombatTargetSnapshot Previous = CurrentTarget;
		CurrentTarget = FCombatTargetSnapshot();
		CurrentReticleState = EReticleState::Hidden;
		OnTargetChanged.Broadcast(Previous, CurrentTarget);
	}
}

bool UTouchTargetingSystem::AutoSelectBestTarget()
{
	TArray<AActor*> Candidates = GatherCandidates();
	if (Candidates.Num() == 0)
	{
		return false;
	}

	AActor* Best = nullptr;
	float BestScore = -1.0f;

	for (AActor* Candidate : Candidates)
	{
		if (!ValidateTarget(Candidate))
		{
			continue;
		}

		const float Score = ScoreTarget(Candidate);
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = Candidate;
		}
	}

	if (Best && Best != GetCurrentTarget())
	{
		return SelectTarget(Best, true);
	}

	return false;
}

// ============================================================================
// Queries
// ============================================================================

AActor* UTouchTargetingSystem::GetCurrentTarget() const
{
	return CurrentTarget.GetActor();
}

bool UTouchTargetingSystem::HasValidTarget() const
{
	return CurrentTarget.IsValid() && !CurrentTarget.GetActor()->IsPendingKillPending();
}

float UTouchTargetingSystem::GetDistanceToTarget() const
{
	return HasValidTarget() ? CurrentTarget.Distance : TNumericLimits<float>::Max();
}

bool UTouchTargetingSystem::GetTargetScreenPosition(FVector2D& OutScreenPos) const
{
	if (!HasValidTarget())
	{
		return false;
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return false;
	}

	return UGameplayStatics::ProjectWorldToScreen(PC, CurrentTarget.WorldLocation, OutScreenPos, false);
}

// ============================================================================
// Internal: Raycasting
// ============================================================================

AActor* UTouchTargetingSystem::RaycastFromScreen(FVector2D ScreenPosition, FVector& OutWorldHit) const
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return nullptr;
	}

	FVector WorldOrigin, WorldDirection;
	if (!UGameplayStatics::DeprojectScreenToWorld(PC, ScreenPosition, WorldOrigin, WorldDirection))
	{
		return nullptr;
	}

	// Use a sphere sweep for mobile-friendly expanded hit detection.
	// The sphere radius is derived from the touch radius config, projected
	// into world space at a representative depth.
	const float SweepRadius = Config.TouchRadiusPixels * 2.0f; // Approximate world-units expansion

	FVector End = WorldOrigin + WorldDirection * Config.MaxRange;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.bTraceComplex = false;
	QueryParams.bReturnPhysicalMaterial = false;

	FHitResult Hit;
	bool bHit = GetWorld()->SweepSingleByChannel(
		Hit,
		WorldOrigin,
		End,
		FQuat::Identity,
		ECC_Pawn, // NPC ships should be on the Pawn channel
		FCollisionShape::MakeSphere(SweepRadius),
		QueryParams
	);

	if (bHit && Hit.GetActor())
	{
		OutWorldHit = Hit.ImpactPoint;
		return Hit.GetActor();
	}

	// Fallback: simple line trace on WorldDynamic
	bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		WorldOrigin,
		End,
		ECC_WorldDynamic,
		QueryParams
	);

	if (bHit && Hit.GetActor())
	{
		OutWorldHit = Hit.ImpactPoint;
		return Hit.GetActor();
	}

	OutWorldHit = End;
	return nullptr;
}

// ============================================================================
// Internal: Candidate Gathering & Scoring
// ============================================================================

TArray<AActor*> UTouchTargetingSystem::GatherCandidates() const
{
	TArray<AActor*> Result;

	if (!GetOwner())
	{
		return Result;
	}

	const FVector Origin = GetOwner()->GetActorLocation();

	// Use actor iteration with distance check (cheaper than overlap on sparse worlds)
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || Actor == GetOwner() || Actor->IsPendingKillPending())
		{
			continue;
		}

		// Quick distance reject
		const float DistSq = FVector::DistSquared(Origin, Actor->GetActorLocation());
		if (DistSq > FMath::Square(Config.MaxRange))
		{
			continue;
		}

		// Tag check
		bool bHasTag = false;
		for (const FName& Tag : Config.ValidTargetTags)
		{
			if (Actor->ActorHasTag(Tag))
			{
				bHasTag = true;
				break;
			}
		}

		if (bHasTag)
		{
			Result.Add(Actor);
		}
	}

	return Result;
}

FCombatTargetSnapshot UTouchTargetingSystem::BuildSnapshot(AActor* Actor) const
{
	FCombatTargetSnapshot Snap;
	if (!Actor || !GetOwner())
	{
		return Snap;
	}

	Snap.Actor = Actor;
	Snap.WorldLocation = Actor->GetActorLocation();
	Snap.Velocity = Actor->GetVelocity();
	Snap.Distance = FVector::Dist(GetOwner()->GetActorLocation(), Snap.WorldLocation);
	Snap.SnapshotTime = GetWorld()->GetTimeSeconds();

	// Health
	if (UNPCHealthComponent* HC = Actor->FindComponentByClass<UNPCHealthComponent>())
	{
		Snap.HealthFraction = HC->GetHealthPercentage();
	}

	// Behavior / hostility
	if (UNPCBehaviorComponent* BC = Actor->FindComponentByClass<UNPCBehaviorComponent>())
	{
		Snap.bIsHostile = BC->IsHostile();
	}

	// Line of sight
	Snap.bHasLineOfSight = CheckLineOfSight(Actor);

	// Priority score
	Snap.PriorityScore = ScoreTarget(Actor);

	return Snap;
}

bool UTouchTargetingSystem::ValidateTarget(AActor* Actor) const
{
	if (!Actor || Actor->IsPendingKillPending() || Actor == GetOwner())
	{
		return false;
	}

	// Tag check
	bool bHasTag = false;
	for (const FName& Tag : Config.ValidTargetTags)
	{
		if (Actor->ActorHasTag(Tag))
		{
			bHasTag = true;
			break;
		}
	}
	if (!bHasTag)
	{
		return false;
	}

	// Same-team reject
	if (Actor->ActorHasTag(FName("Player")) || Actor->ActorHasTag(FName("Ally")))
	{
		return false;
	}

	// Range check
	if (GetOwner())
	{
		const float Dist = FVector::Dist(GetOwner()->GetActorLocation(), Actor->GetActorLocation());
		if (Dist > Config.MaxRange)
		{
			return false;
		}
	}

	// Dead check
	if (UNPCHealthComponent* HC = Actor->FindComponentByClass<UNPCHealthComponent>())
	{
		if (HC->IsDead())
		{
			return false;
		}
	}

	return true;
}

float UTouchTargetingSystem::ScoreTarget(AActor* Actor) const
{
	if (!Actor || !GetOwner())
	{
		return 0.0f;
	}

	float Score = 0.0f;
	const float Dist = FVector::Dist(GetOwner()->GetActorLocation(), Actor->GetActorLocation());

	// Distance factor: closer = higher score (normalized 0..1)
	const float NormDist = 1.0f - FMath::Clamp(Dist / Config.MaxRange, 0.0f, 1.0f);
	Score += NormDist * Config.DistanceWeight;

	// Low health factor: lower health = higher score
	if (UNPCHealthComponent* HC = Actor->FindComponentByClass<UNPCHealthComponent>())
	{
		const float LowHealthFactor = 1.0f - HC->GetHealthPercentage();
		Score += LowHealthFactor * Config.LowHealthWeight;
	}

	// Hostility factor
	if (UNPCBehaviorComponent* BC = Actor->FindComponentByClass<UNPCBehaviorComponent>())
	{
		if (BC->IsHostile())
		{
			Score += Config.HostilityWeight;
		}
	}

	return Score;
}

bool UTouchTargetingSystem::CheckLineOfSight(AActor* Target) const
{
	if (!Target || !GetOwner())
	{
		return false;
	}

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());
	Params.AddIgnoredActor(Target);
	Params.bTraceComplex = false;

	FHitResult Hit;
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
		Hit,
		GetOwner()->GetActorLocation(),
		Target->GetActorLocation(),
		ECC_WorldStatic,
		Params
	);

	return !bBlocked;
}

// ============================================================================
// Internal: Target Tracking
// ============================================================================

void UTouchTargetingSystem::RefreshCurrentTarget()
{
	AActor* Actor = CurrentTarget.GetActor();
	if (!Actor || !ValidateTarget(Actor))
	{
		ClearTarget();
		return;
	}

	// Update mutable snapshot fields
	CurrentTarget.WorldLocation = Actor->GetActorLocation();
	CurrentTarget.Velocity = Actor->GetVelocity();
	CurrentTarget.Distance = GetOwner()
		? FVector::Dist(GetOwner()->GetActorLocation(), CurrentTarget.WorldLocation)
		: TNumericLimits<float>::Max();
	CurrentTarget.SnapshotTime = GetWorld()->GetTimeSeconds();

	if (UNPCHealthComponent* HC = Actor->FindComponentByClass<UNPCHealthComponent>())
	{
		CurrentTarget.HealthFraction = HC->GetHealthPercentage();
		if (HC->IsDead())
		{
			ClearTarget();
			return;
		}
	}

	CurrentTarget.bHasLineOfSight = CheckLineOfSight(Actor);
}

void UTouchTargetingSystem::TickAutoTargeting(float DeltaTime)
{
	AutoTargetTimer += DeltaTime;
	if (AutoTargetTimer < Config.AutoTargetInterval)
	{
		return;
	}
	AutoTargetTimer = 0.0f;

	// Only auto-select when we have no valid target
	if (!HasValidTarget())
	{
		AutoSelectBestTarget();
	}
}

void UTouchTargetingSystem::UpdateReticleState()
{
	if (!HasValidTarget())
	{
		CurrentReticleState = EReticleState::Searching;
		return;
	}

	if (CurrentTarget.Distance > Config.MaxRange)
	{
		CurrentReticleState = EReticleState::OutOfRange;
	}
	else if (!CurrentTarget.bHasLineOfSight)
	{
		CurrentReticleState = EReticleState::Locking;
	}
	else
	{
		CurrentReticleState = EReticleState::Locked;
	}
}

UCameraComponent* UTouchTargetingSystem::ResolveCamera() const
{
	// Owner first
	if (GetOwner())
	{
		if (UCameraComponent* C = GetOwner()->FindComponentByClass<UCameraComponent>())
		{
			return C;
		}
	}

	// Player pawn fallback
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC && PC->GetPawn())
	{
		return PC->GetPawn()->FindComponentByClass<UCameraComponent>();
	}

	return nullptr;
}

void UTouchTargetingSystem::SetTargetInternal(const FCombatTargetSnapshot& NewSnapshot)
{
	FCombatTargetSnapshot Previous = CurrentTarget;
	CurrentTarget = NewSnapshot;
	OnTargetChanged.Broadcast(Previous, CurrentTarget);
}
