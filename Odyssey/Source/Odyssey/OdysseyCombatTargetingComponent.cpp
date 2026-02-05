// OdysseyCombatTargetingComponent.cpp

#include "OdysseyCombatTargetingComponent.h"
#include "OdysseyActionDispatcher.h"
#include "NPCHealthComponent.h"
#include "NPCBehaviorComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

UOdysseyCombatTargetingComponent::UOdysseyCombatTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f; // 10 FPS for performance

	// Default configuration
	TargetingMode = ETargetingMode::Assisted;
	MaxTargetingRange = 2000.0f;
	AutoTargetUpdateFrequency = 0.5f;
	DistancePriorityBias = 1.0f;
	HealthPriorityBias = 0.5f;
	PlayerTeamId = 0;
	bBroadcastTargetingEvents = true;

	// Default line of sight channels
	LineOfSightChannels.Add(ECC_WorldStatic);
	LineOfSightChannels.Add(ECC_WorldDynamic);

	// Default valid target tags
	ValidTargetTags.Add(FName("Enemy"));
	ValidTargetTags.Add(FName("NPC"));

	// Invalid target tags
	InvalidTargetTags.Add(FName("Player"));
	InvalidTargetTags.Add(FName("Ally"));

	// Initialize runtime state
	LastAutoTargetUpdateTime = 0.0f;
	CachedCameraComponent = nullptr;
	EventBus = nullptr;
}

void UOdysseyCombatTargetingComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeTargeting();
}

void UOdysseyCombatTargetingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShutdownTargeting();
	Super::EndPlay(EndPlayReason);
}

void UOdysseyCombatTargetingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update target information
	if (HasValidTarget())
	{
		UpdateTargetInformation();
	}

	// Update auto-targeting if enabled
	if (TargetingMode != ETargetingMode::Manual)
	{
		UpdateAutoTargeting(DeltaTime);
	}
}

// ============================================================================
// Touch Targeting Interface
// ============================================================================

FTouchTargetResult UOdysseyCombatTargetingComponent::HandleTouchTargeting(FVector2D TouchLocation)
{
	FTouchTargetResult Result;
	Result.TouchScreenLocation = TouchLocation;

	// Perform screen-to-world raycast
	FVector WorldLocation;
	AActor* HitActor;
	if (!ScreenToWorldRaycast(TouchLocation, WorldLocation, HitActor))
	{
		Result.ValidationResult = ETargetValidation::InvalidActor;
		return Result;
	}

	Result.bValidTouch = true;
	Result.TouchWorldLocation = WorldLocation;
	Result.TouchedActor = HitActor;

	// If we hit an actor, try to select it as target
	if (HitActor)
	{
		Result.ValidationResult = ValidateTarget(HitActor);
		if (Result.ValidationResult == ETargetValidation::Valid)
		{
			SelectTarget(HitActor, false); // Already validated
		}
	}

	return Result;
}

bool UOdysseyCombatTargetingComponent::ScreenToWorldRaycast(FVector2D ScreenLocation, FVector& WorldLocation, AActor*& HitActor)
{
	UCameraComponent* Camera = GetCameraComponent();
	if (!Camera)
	{
		return false;
	}

	// Get player controller
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
	{
		return false;
	}

	// Deproject screen coordinates to world space
	FVector WorldDirection;
	if (!UGameplayStatics::DeprojectScreenToWorld(PlayerController, ScreenLocation, WorldLocation, WorldDirection))
	{
		return false;
	}

	// Perform raycast
	FHitResult HitResult;
	FVector StartLocation = WorldLocation;
	FVector EndLocation = StartLocation + (WorldDirection * MaxTargetingRange);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.bTraceComplex = false;

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECC_WorldDynamic,
		QueryParams
	);

	if (bHit)
	{
		WorldLocation = HitResult.Location;
		HitActor = HitResult.GetActor();
		return true;
	}

	HitActor = nullptr;
	return false;
}

bool UOdysseyCombatTargetingComponent::SelectTarget(AActor* TargetActor, bool bValidateTarget)
{
	if (!TargetActor)
	{
		ClearTarget();
		return false;
	}

	// Validate target if requested
	if (bValidateTarget)
	{
		ETargetValidation ValidationResult = ValidateTarget(TargetActor);
		if (ValidationResult != ETargetValidation::Valid)
		{
			OnTargetInvalidated(TargetActor, ValidationResult);
			return false;
		}
	}

	AActor* PreviousTarget = CurrentTarget.TargetActor;

	// Update target info
	CurrentTarget.TargetActor = TargetActor;
	CurrentTarget.SelectionTime = FPlatformTime::Seconds();
	UpdateTargetInformation();

	// Broadcast events
	if (bBroadcastTargetingEvents)
	{
		BroadcastTargetChangeEvent(PreviousTarget, TargetActor);
	}

	OnTargetSelected(TargetActor);

	return true;
}

void UOdysseyCombatTargetingComponent::ClearTarget()
{
	AActor* PreviousTarget = CurrentTarget.TargetActor;
	
	if (PreviousTarget)
	{
		// Reset target info
		CurrentTarget = FTargetInfo();

		// Broadcast events
		if (bBroadcastTargetingEvents)
		{
			BroadcastTargetChangeEvent(PreviousTarget, nullptr);
		}

		OnTargetCleared(PreviousTarget);
	}
}

// ============================================================================
// Automatic Targeting
// ============================================================================

bool UOdysseyCombatTargetingComponent::AutoSelectTarget()
{
	TArray<AActor*> Candidates = FindPotentialTargets();
	if (Candidates.Num() == 0)
	{
		return false;
	}

	AActor* BestTarget = GetBestTarget(Candidates);
	if (BestTarget && BestTarget != CurrentTarget.TargetActor)
	{
		SelectTarget(BestTarget, false); // Already validated in GetBestTarget
		OnAutoTargetFound(BestTarget);
		return true;
	}

	return false;
}

AActor* UOdysseyCombatTargetingComponent::GetBestTarget(const TArray<AActor*>& Candidates)
{
	if (Candidates.Num() == 0)
	{
		return nullptr;
	}

	AActor* BestTarget = nullptr;
	float BestScore = -1.0f;

	for (AActor* Candidate : Candidates)
	{
		// Validate target
		if (ValidateTarget(Candidate) != ETargetValidation::Valid)
		{
			continue;
		}

		// Calculate score
		float Score = CalculateTargetScore(Candidate);
		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

TArray<AActor*> UOdysseyCombatTargetingComponent::FindPotentialTargets()
{
	TArray<AActor*> Targets;

	if (!GetOwner())
	{
		return Targets;
	}

	FVector OwnerLocation = GetOwner()->GetActorLocation();

	// Find actors with valid target tags within range
	UWorld* World = GetWorld();
	if (!World)
	{
		return Targets;
	}

	for (FActorIterator ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		AActor* Actor = *ActorIterator;
		if (!Actor || Actor == GetOwner())
		{
			continue;
		}

		// Check distance
		float Distance = FVector::Dist(OwnerLocation, Actor->GetActorLocation());
		if (Distance > MaxTargetingRange)
		{
			continue;
		}

		// Check if actor has valid target tags
		bool bHasValidTag = false;
		for (const FName& Tag : ValidTargetTags)
		{
			if (Actor->ActorHasTag(Tag))
			{
				bHasValidTag = true;
				break;
			}
		}

		if (bHasValidTag)
		{
			Targets.Add(Actor);
		}
	}

	return Targets;
}

ETargetPriority UOdysseyCombatTargetingComponent::CalculateTargetPriority(AActor* TargetActor)
{
	if (!TargetActor)
	{
		return ETargetPriority::None;
	}

	// Check if actor is hostile
	UNPCBehaviorComponent* BehaviorComp = TargetActor->FindComponentByClass<UNPCBehaviorComponent>();
	bool bIsHostile = BehaviorComp ? BehaviorComp->IsHostile() : false;

	// Get health info
	UNPCHealthComponent* HealthComp = TargetActor->FindComponentByClass<UNPCHealthComponent>();
	float HealthPercentage = HealthComp ? HealthComp->GetHealthPercentage() : 1.0f;

	// Calculate distance
	float Distance = FVector::Dist(GetOwner()->GetActorLocation(), TargetActor->GetActorLocation());

	// Determine priority
	if (bIsHostile)
	{
		if (Distance < MaxTargetingRange * 0.3f) // Close range
		{
			return HealthPercentage < 0.25f ? ETargetPriority::Critical : ETargetPriority::High;
		}
		else if (Distance < MaxTargetingRange * 0.6f) // Medium range
		{
			return ETargetPriority::Medium;
		}
		else // Long range
		{
			return ETargetPriority::Low;
		}
	}
	else
	{
		return ETargetPriority::Low;
	}
}

// ============================================================================
// Target Validation
// ============================================================================

ETargetValidation UOdysseyCombatTargetingComponent::ValidateTarget(AActor* TargetActor)
{
	if (!TargetActor)
	{
		return ETargetValidation::InvalidActor;
	}

	// Check if dead
	UNPCHealthComponent* HealthComp = TargetActor->FindComponentByClass<UNPCHealthComponent>();
	if (HealthComp && HealthComp->IsDead())
	{
		return ETargetValidation::Dead;
	}

	// Check if same team
	if (IsSameTeam(TargetActor))
	{
		return ETargetValidation::SameTeam;
	}

	// Check range
	if (!IsTargetInRange(TargetActor))
	{
		return ETargetValidation::OutOfRange;
	}

	// Check line of sight
	if (!HasLineOfSightToTarget(TargetActor))
	{
		return ETargetValidation::NoLineOfSight;
	}

	return ETargetValidation::Valid;
}

bool UOdysseyCombatTargetingComponent::IsTargetInRange(AActor* TargetActor)
{
	if (!TargetActor || !GetOwner())
	{
		return false;
	}

	float Distance = FVector::Dist(GetOwner()->GetActorLocation(), TargetActor->GetActorLocation());
	return Distance <= MaxTargetingRange;
}

bool UOdysseyCombatTargetingComponent::HasLineOfSightToTarget(AActor* TargetActor)
{
	if (!TargetActor || !GetOwner())
	{
		return false;
	}

	FVector StartLocation = GetOwner()->GetActorLocation();
	FVector EndLocation = TargetActor->GetActorLocation();

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.AddIgnoredActor(TargetActor);
	QueryParams.bTraceComplex = false;

	// Check all configured line of sight channels
	for (TEnumAsByte<ECollisionChannel> Channel : LineOfSightChannels)
	{
		FHitResult HitResult;
		bool bBlocked = GetWorld()->LineTraceSingleByChannel(
			HitResult,
			StartLocation,
			EndLocation,
			Channel,
			QueryParams
		);

		if (bBlocked)
		{
			return false; // Line of sight blocked
		}
	}

	return true;
}

bool UOdysseyCombatTargetingComponent::IsSameTeam(AActor* TargetActor)
{
	// Simple team check - can be expanded based on game needs
	if (TargetActor->ActorHasTag(FName("Player")) || TargetActor->ActorHasTag(FName("Ally")))
	{
		return true;
	}

	return false;
}

// ============================================================================
// Target Information
// ============================================================================

bool UOdysseyCombatTargetingComponent::HasValidTarget() const
{
	return CurrentTarget.TargetActor != nullptr && IsValid(CurrentTarget.TargetActor);
}

void UOdysseyCombatTargetingComponent::UpdateTargetInformation()
{
	if (!HasValidTarget() || !GetOwner())
	{
		return;
	}

	AActor* Target = CurrentTarget.TargetActor;

	// Update location and distance
	CurrentTarget.TargetLocation = Target->GetActorLocation();
	CurrentTarget.DistanceToTarget = FVector::Dist(GetOwner()->GetActorLocation(), CurrentTarget.TargetLocation);

	// Update health percentage
	UNPCHealthComponent* HealthComp = Target->FindComponentByClass<UNPCHealthComponent>();
	CurrentTarget.HealthPercentage = HealthComp ? HealthComp->GetHealthPercentage() : 1.0f;

	// Update hostility
	UNPCBehaviorComponent* BehaviorComp = Target->FindComponentByClass<UNPCBehaviorComponent>();
	CurrentTarget.bIsHostile = BehaviorComp ? BehaviorComp->IsHostile() : false;

	// Update line of sight
	CurrentTarget.bHasLineOfSight = HasLineOfSightToTarget(Target);

	// Update priority
	CurrentTarget.Priority = CalculateTargetPriority(Target);

	// Validate target and clear if invalid
	if (ValidateTarget(Target) != ETargetValidation::Valid)
	{
		ClearTarget();
	}
}

// ============================================================================
// Configuration
// ============================================================================

void UOdysseyCombatTargetingComponent::SetTargetingMode(ETargetingMode NewMode)
{
	if (TargetingMode != NewMode)
	{
		ETargetingMode OldMode = TargetingMode;
		TargetingMode = NewMode;
		OnTargetingModeChanged(OldMode, NewMode);
	}
}

void UOdysseyCombatTargetingComponent::SetMaxTargetingRange(float NewRange)
{
	MaxTargetingRange = FMath::Max(100.0f, NewRange);
}

// ============================================================================
// Event System Integration
// ============================================================================

UOdysseyEventBus* UOdysseyCombatTargetingComponent::GetEventBus()
{
	if (!EventBus)
	{
		EventBus = UOdysseyActionDispatcher::GetEventBus(GetWorld());
	}
	return EventBus;
}

// ============================================================================
// Mobile UI Integration
// ============================================================================

bool UOdysseyCombatTargetingComponent::GetTargetScreenPosition(FVector2D& ScreenPosition)
{
	if (!HasValidTarget())
	{
		return false;
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
	{
		return false;
	}

	return UGameplayStatics::ProjectWorldToScreen(PlayerController, CurrentTarget.TargetLocation, ScreenPosition, false);
}

bool UOdysseyCombatTargetingComponent::IsTargetOnScreen()
{
	FVector2D ScreenPosition;
	return GetTargetScreenPosition(ScreenPosition);
}

// ============================================================================
// Internal Methods
// ============================================================================

void UOdysseyCombatTargetingComponent::InitializeTargeting()
{
	// Cache camera component
	GetCameraComponent();

	// Initialize event subscriptions
	InitializeEventSubscriptions();
}

void UOdysseyCombatTargetingComponent::ShutdownTargeting()
{
	// Clear current target
	ClearTarget();

	// Clean up event subscriptions
	CleanupEventSubscriptions();
}

void UOdysseyCombatTargetingComponent::UpdateAutoTargeting(float DeltaTime)
{
	float CurrentTime = FPlatformTime::Seconds();
	
	// Check if it's time for an update
	if (CurrentTime - LastAutoTargetUpdateTime < AutoTargetUpdateFrequency)
	{
		return;
	}

	LastAutoTargetUpdateTime = CurrentTime;

	// If we don't have a target or current target is invalid, try to find one
	if (!HasValidTarget() || ValidateTarget(CurrentTarget.TargetActor) != ETargetValidation::Valid)
	{
		AutoSelectTarget();
	}
}

UCameraComponent* UOdysseyCombatTargetingComponent::GetCameraComponent()
{
	if (!CachedCameraComponent)
	{
		// Try to find camera component on owner
		if (GetOwner())
		{
			CachedCameraComponent = GetOwner()->FindComponentByClass<UCameraComponent>();
		}

		// If not found, try to get from player controller
		if (!CachedCameraComponent)
		{
			APlayerController* PC = GetWorld()->GetFirstPlayerController();
			if (PC && PC->GetPawn())
			{
				CachedCameraComponent = PC->GetPawn()->FindComponentByClass<UCameraComponent>();
			}
		}
	}

	return CachedCameraComponent;
}

float UOdysseyCombatTargetingComponent::CalculateTargetScore(AActor* TargetActor)
{
	if (!TargetActor || !GetOwner())
	{
		return 0.0f;
	}

	float Score = 0.0f;

	// Distance factor (closer = higher score)
	float Distance = FVector::Dist(GetOwner()->GetActorLocation(), TargetActor->GetActorLocation());
	float NormalizedDistance = 1.0f - (Distance / MaxTargetingRange);
	Score += NormalizedDistance * DistancePriorityBias;

	// Health factor (lower health = higher score)
	UNPCHealthComponent* HealthComp = TargetActor->FindComponentByClass<UNPCHealthComponent>();
	if (HealthComp)
	{
		float HealthFactor = 1.0f - HealthComp->GetHealthPercentage();
		Score += HealthFactor * HealthPriorityBias;
	}

	// Hostility factor
	UNPCBehaviorComponent* BehaviorComp = TargetActor->FindComponentByClass<UNPCBehaviorComponent>();
	if (BehaviorComp && BehaviorComp->IsHostile())
	{
		Score += 1.0f; // Bonus for hostile targets
	}

	// Priority factor
	ETargetPriority Priority = CalculateTargetPriority(TargetActor);
	Score += static_cast<float>(Priority) * 0.5f;

	return Score;
}

void UOdysseyCombatTargetingComponent::BroadcastTargetChangeEvent(AActor* PreviousTarget, AActor* NewTarget)
{
	UOdysseyEventBus* Bus = GetEventBus();
	if (!Bus)
	{
		return;
	}

	FTargetChangeEventPayload EventPayload;
	EventPayload.Initialize(EOdysseyEventType::ActionExecuted, GetOwner()); // Using ActionExecuted for target changes
	EventPayload.PreviousTarget = PreviousTarget;
	EventPayload.NewTarget = NewTarget;
	EventPayload.TargetingMode = TargetingMode;
	EventPayload.bIsAutoTarget = (TargetingMode != ETargetingMode::Manual);

	// Convert to base event payload for publishing
	FOdysseyEventPayload BasePayload;
	BasePayload.Initialize(EOdysseyEventType::ActionExecuted, GetOwner());

	Bus->PublishEvent(BasePayload);
}

void UOdysseyCombatTargetingComponent::OnTargetDied(AActor* DiedActor)
{
	if (CurrentTarget.TargetActor == DiedActor)
	{
		ClearTarget();
	}
}

void UOdysseyCombatTargetingComponent::InitializeEventSubscriptions()
{
	UOdysseyEventBus* Bus = GetEventBus();
	if (!Bus)
	{
		return;
	}

	// Subscribe to actor death events
	FOdysseyEventFilter Filter;
	Filter.AllowedEventTypes.Add(EOdysseyEventType::DamageReceived);
	
	// Note: In a real implementation, you'd use the event bus subscription system
	// For now, we'll rely on the UpdateTargetInformation to check target validity
}

void UOdysseyCombatTargetingComponent::CleanupEventSubscriptions()
{
	UOdysseyEventBus* Bus = GetEventBus();
	if (!Bus)
	{
		return;
	}

	// Unsubscribe from events
	for (const FOdysseyEventHandle& Handle : EventHandles)
	{
		// Bus->Unsubscribe(Handle); // Would implement in full event bus
	}
	EventHandles.Empty();
}

void UOdysseyCombatTargetingComponent::OnActorDiedEvent(const FOdysseyEventPayload& Payload)
{
	if (Payload.Source.IsValid())
	{
		OnTargetDied(Payload.Source.Get());
	}
}
