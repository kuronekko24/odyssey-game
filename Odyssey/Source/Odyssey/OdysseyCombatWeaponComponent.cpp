// OdysseyCombatWeaponComponent.cpp

#include "OdysseyCombatWeaponComponent.h"
#include "OdysseyCombatTargetingComponent.h"
#include "OdysseyActionDispatcher.h"
#include "OdysseyActionButton.h"
#include "NPCHealthComponent.h"
#include "Components/AudioComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UOdysseyCombatWeaponComponent::UOdysseyCombatWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f; // 20 FPS for responsive combat

	// Default weapon configuration
	WeaponType = EWeaponType::Laser;
	FireMode = EWeaponFireMode::Automatic;
	WeaponId = 0;
	bWeaponEnabled = true;
	bAutoFireEnabled = true;
	AutoFireUpdateFrequency = 0.1f;
	bBroadcastWeaponEvents = true;

	// Default weapon stats
	WeaponStats.Damage = 25.0f;
	WeaponStats.RateOfFire = 3.0f;
	WeaponStats.Range = 1500.0f;
	WeaponStats.Accuracy = 0.95f;
	WeaponStats.EnergyCostPerShot = 10;
	WeaponStats.CriticalChance = 0.1f;
	WeaponStats.CriticalMultiplier = 2.0f;

	// Default projectile config
	ProjectileConfig.Speed = 2500.0f;
	ProjectileConfig.Lifetime = 3.0f;
	ProjectileConfig.bIsHoming = false;

	// Default mount offset
	MountOffset = FVector(100.0f, 0.0f, 0.0f); // Forward of ship

	// Initialize runtime state
	CurrentState = EWeaponState::Ready;
	LastFireTime = 0.0f;
	CurrentChargeLevel = 0.0f;
	ChargeStartTime = 0.0f;
	LastAutoFireUpdateTime = 0.0f;
	TargetingComponent = nullptr;
	EventBus = nullptr;
	AudioComponent = nullptr;
}

void UOdysseyCombatWeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeWeapon();
}

void UOdysseyCombatWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShutdownWeapon();
	Super::EndPlay(EndPlayReason);
}

void UOdysseyCombatWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update weapon state
	UpdateWeaponState(DeltaTime);

	// Update auto-firing
	if (bAutoFireEnabled && CurrentState == EWeaponState::Ready)
	{
		UpdateAutoFiring(DeltaTime);
	}
}

// ============================================================================
// Weapon Firing Interface
// ============================================================================

FWeaponFireResult UOdysseyCombatWeaponComponent::FireWeapon()
{
	AActor* Target = GetCurrentTarget();
	if (Target)
	{
		return FireAtTarget(Target);
	}

	// No target, return failure
	FWeaponFireResult Result;
	Result.FailureReason = FName("NoTarget");
	return Result;
}

FWeaponFireResult UOdysseyCombatWeaponComponent::FireAtTarget(AActor* Target)
{
	if (!Target)
	{
		FWeaponFireResult Result;
		Result.FailureReason = FName("InvalidTarget");
		return Result;
	}

	FVector TargetLocation = Target->GetActorLocation();
	return FireInternal(Target, TargetLocation);
}

FWeaponFireResult UOdysseyCombatWeaponComponent::FireAtLocation(FVector TargetLocation)
{
	return FireInternal(nullptr, TargetLocation);
}

bool UOdysseyCombatWeaponComponent::StartCharging()
{
	if (CurrentState != EWeaponState::Ready)
	{
		return false;
	}

	if (!HasEnoughEnergy())
	{
		return false;
	}

	ChangeWeaponState(EWeaponState::Charging);
	ChargeStartTime = FPlatformTime::Seconds();
	CurrentChargeLevel = 0.0f;

	OnWeaponChargeStarted();
	return true;
}

FWeaponFireResult UOdysseyCombatWeaponComponent::ReleaseChargedShot()
{
	if (CurrentState != EWeaponState::Charging)
	{
		FWeaponFireResult Result;
		Result.FailureReason = FName("NotCharging");
		return Result;
	}

	// Get target and fire
	AActor* Target = GetCurrentTarget();
	if (Target)
	{
		// Modify damage based on charge level
		float OriginalDamage = WeaponStats.Damage;
		WeaponStats.Damage *= (1.0f + CurrentChargeLevel); // Up to 2x damage when fully charged

		FWeaponFireResult Result = FireAtTarget(Target);

		// Restore original damage
		WeaponStats.Damage = OriginalDamage;

		OnWeaponChargeCompleted();
		return Result;
	}

	CancelCharging();
	FWeaponFireResult Result;
	Result.FailureReason = FName("NoTarget");
	return Result;
}

void UOdysseyCombatWeaponComponent::CancelCharging()
{
	if (CurrentState == EWeaponState::Charging)
	{
		ChangeWeaponState(EWeaponState::Ready);
		CurrentChargeLevel = 0.0f;
		ChargeStartTime = 0.0f;
	}
}

// ============================================================================
// Automatic Firing System
// ============================================================================

void UOdysseyCombatWeaponComponent::SetAutoFireEnabled(bool bEnabled)
{
	if (bAutoFireEnabled != bEnabled)
	{
		bAutoFireEnabled = bEnabled;
		OnAutoFireStateChanged(bEnabled);
	}
}

void UOdysseyCombatWeaponComponent::UpdateAutoFiring(float DeltaTime)
{
	if (!bAutoFireEnabled || !bWeaponEnabled)
	{
		return;
	}

	float CurrentTime = FPlatformTime::Seconds();
	
	// Check update frequency for performance
	if (CurrentTime - LastAutoFireUpdateTime < AutoFireUpdateFrequency)
	{
		return;
	}
	
	LastAutoFireUpdateTime = CurrentTime;

	// Check if we can auto-fire
	if (CanAutoFire())
	{
		FireWeapon();
	}
}

bool UOdysseyCombatWeaponComponent::CanAutoFire() const
{
	// Check basic conditions
	if (!bAutoFireEnabled || !bWeaponEnabled || CurrentState != EWeaponState::Ready)
	{
		return false;
	}

	// Check energy
	if (!HasEnoughEnergy())
	{
		return false;
	}

	// Check rate of fire cooldown
	float TimeSinceLastFire = FPlatformTime::Seconds() - LastFireTime;
	float FireCooldown = 1.0f / WeaponStats.RateOfFire;
	if (TimeSinceLastFire < FireCooldown)
	{
		return false;
	}

	// Check for valid target
	AActor* Target = GetCurrentTarget();
	if (!Target)
	{
		return false;
	}

	// Check if target is in range
	if (!IsTargetInRange(Target))
	{
		return false;
	}

	return true;
}

// ============================================================================
// Weapon State Management
// ============================================================================

bool UOdysseyCombatWeaponComponent::CanFire() const
{
	return bWeaponEnabled && 
		   CurrentState == EWeaponState::Ready && 
		   HasEnoughEnergy();
}

float UOdysseyCombatWeaponComponent::GetReloadProgress() const
{
	if (CurrentState != EWeaponState::Reloading)
	{
		return 1.0f;
	}

	float TimeSinceLastFire = FPlatformTime::Seconds() - LastFireTime;
	float ReloadTime = 1.0f / WeaponStats.RateOfFire;
	
	return FMath::Clamp(TimeSinceLastFire / ReloadTime, 0.0f, 1.0f);
}

float UOdysseyCombatWeaponComponent::GetTimeUntilNextShot() const
{
	if (CurrentState == EWeaponState::Ready)
	{
		return 0.0f;
	}

	float TimeSinceLastFire = FPlatformTime::Seconds() - LastFireTime;
	float ReloadTime = 1.0f / WeaponStats.RateOfFire;
	
	return FMath::Max(0.0f, ReloadTime - TimeSinceLastFire);
}

// ============================================================================
// Weapon Configuration
// ============================================================================

void UOdysseyCombatWeaponComponent::SetWeaponType(EWeaponType NewType)
{
	WeaponType = NewType;
	
	// Update weapon stats based on type
	switch (NewType)
	{
	case EWeaponType::Laser:
		WeaponStats.Damage = 20.0f;
		WeaponStats.RateOfFire = 4.0f;
		WeaponStats.Range = 2000.0f;
		WeaponStats.EnergyCostPerShot = 5;
		break;
		
	case EWeaponType::Plasma:
		WeaponStats.Damage = 35.0f;
		WeaponStats.RateOfFire = 2.0f;
		WeaponStats.Range = 1500.0f;
		WeaponStats.EnergyCostPerShot = 15;
		break;
		
	case EWeaponType::Kinetic:
		WeaponStats.Damage = 45.0f;
		WeaponStats.RateOfFire = 1.5f;
		WeaponStats.Range = 1800.0f;
		WeaponStats.EnergyCostPerShot = 8;
		break;
		
	case EWeaponType::Missile:
		WeaponStats.Damage = 80.0f;
		WeaponStats.RateOfFire = 0.5f;
		WeaponStats.Range = 2500.0f;
		WeaponStats.EnergyCostPerShot = 25;
		ProjectileConfig.bIsHoming = true;
		break;
		
	default:
		break;
	}
}

void UOdysseyCombatWeaponComponent::SetFireMode(EWeaponFireMode NewMode)
{
	FireMode = NewMode;
}

void UOdysseyCombatWeaponComponent::SetWeaponStats(const FWeaponStats& NewStats)
{
	WeaponStats = NewStats;
}

void UOdysseyCombatWeaponComponent::SetWeaponEnabled(bool bEnabled)
{
	bWeaponEnabled = bEnabled;
	
	if (!bEnabled && CurrentState == EWeaponState::Charging)
	{
		CancelCharging();
	}
	
	if (!bEnabled)
	{
		ChangeWeaponState(EWeaponState::Disabled);
	}
	else if (CurrentState == EWeaponState::Disabled)
	{
		ChangeWeaponState(EWeaponState::Ready);
	}
}

// ============================================================================
// Targeting Integration
// ============================================================================

void UOdysseyCombatWeaponComponent::SetTargetingComponent(UOdysseyCombatTargetingComponent* TargetingComp)
{
	TargetingComponent = TargetingComp;
}

AActor* UOdysseyCombatWeaponComponent::GetCurrentTarget() const
{
	if (TargetingComponent)
	{
		return TargetingComponent->GetCurrentTarget();
	}
	
	return nullptr;
}

bool UOdysseyCombatWeaponComponent::IsTargetInRange(AActor* Target) const
{
	if (!Target || !GetOwner())
	{
		return false;
	}

	float Distance = FVector::Dist(GetOwner()->GetActorLocation(), Target->GetActorLocation());
	return Distance <= WeaponStats.Range;
}

// ============================================================================
// Effects and Feedback
// ============================================================================

void UOdysseyCombatWeaponComponent::PlayMuzzleFlash()
{
	if (EffectsConfig.MuzzleFlashEffect.IsValid())
	{
		FVector MuzzleLocation = GetMountWorldLocation();
		FRotator MuzzleRotation = GetMountWorldRotation();
		
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			EffectsConfig.MuzzleFlashEffect.LoadSynchronous(),
			MuzzleLocation,
			MuzzleRotation
		);
	}
}

void UOdysseyCombatWeaponComponent::SpawnProjectileTrail(FVector Start, FVector End)
{
	if (EffectsConfig.ProjectileTrailEffect.IsValid())
	{
		FVector Direction = (End - Start).GetSafeNormal();
		FRotator TrailRotation = Direction.Rotation();
		
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			EffectsConfig.ProjectileTrailEffect.LoadSynchronous(),
			Start,
			TrailRotation
		);
	}
}

void UOdysseyCombatWeaponComponent::PlayImpactEffect(FVector Location, AActor* HitActor)
{
	if (EffectsConfig.ImpactEffect.IsValid())
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			EffectsConfig.ImpactEffect.LoadSynchronous(),
			Location
		);
	}
}

void UOdysseyCombatWeaponComponent::PlayFireSound()
{
	if (EffectsConfig.FireSound.IsValid())
	{
		if (!AudioComponent)
		{
			AudioComponent = UGameplayStatics::SpawnSoundAttached(
				EffectsConfig.FireSound.LoadSynchronous(),
				GetOwner()->GetRootComponent()
			);
		}
		else
		{
			AudioComponent->SetSound(EffectsConfig.FireSound.LoadSynchronous());
			AudioComponent->Play();
		}
	}
}

// ============================================================================
// Energy Integration
// ============================================================================

bool UOdysseyCombatWeaponComponent::HasEnoughEnergy() const
{
	// Check with action button manager for energy
	UOdysseyActionButtonManager* ActionManager = GetOwner()->FindComponentByClass<UOdysseyActionButtonManager>();
	if (ActionManager)
	{
		return ActionManager->GetCurrentEnergy() >= WeaponStats.EnergyCostPerShot;
	}
	
	return true; // Default to true if no energy system
}

bool UOdysseyCombatWeaponComponent::ConsumeEnergyForShot()
{
	UOdysseyActionButtonManager* ActionManager = GetOwner()->FindComponentByClass<UOdysseyActionButtonManager>();
	if (ActionManager)
	{
		return ActionManager->SpendEnergy(WeaponStats.EnergyCostPerShot);
	}
	
	return true; // Default to true if no energy system
}

// ============================================================================
// Event System Integration
// ============================================================================

UOdysseyEventBus* UOdysseyCombatWeaponComponent::GetEventBus()
{
	if (!EventBus)
	{
		EventBus = UOdysseyActionDispatcher::GetEventBus(GetWorld());
	}
	return EventBus;
}

// ============================================================================
// Internal Methods
// ============================================================================

void UOdysseyCombatWeaponComponent::InitializeWeapon()
{
	// Initialize event subscriptions
	InitializeEventSubscriptions();

	// Find targeting component on owner
	if (GetOwner())
	{
		TargetingComponent = GetOwner()->FindComponentByClass<UOdysseyCombatTargetingComponent>();
	}
}

void UOdysseyCombatWeaponComponent::ShutdownWeapon()
{
	// Clean up event subscriptions
	CleanupEventSubscriptions();
}

void UOdysseyCombatWeaponComponent::UpdateWeaponState(float DeltaTime)
{
	float CurrentTime = FPlatformTime::Seconds();

	switch (CurrentState)
	{
	case EWeaponState::Reloading:
		{
			float TimeSinceLastFire = CurrentTime - LastFireTime;
			float ReloadTime = 1.0f / WeaponStats.RateOfFire;
			
			if (TimeSinceLastFire >= ReloadTime)
			{
				ChangeWeaponState(EWeaponState::Ready);
			}
		}
		break;
		
	case EWeaponState::Charging:
		{
			float ChargeTime = CurrentTime - ChargeStartTime;
			float MaxChargeTime = 2.0f; // 2 seconds to full charge
			
			CurrentChargeLevel = FMath::Clamp(ChargeTime / MaxChargeTime, 0.0f, 1.0f);
		}
		break;
		
	default:
		break;
	}
}

FVector UOdysseyCombatWeaponComponent::CalculateFireDirection(AActor* Target) const
{
	if (!Target || !GetOwner())
	{
		return GetOwner()->GetActorForwardVector();
	}

	FVector StartLocation = GetMountWorldLocation();
	FVector TargetLocation = Target->GetActorLocation();
	
	// Add some prediction for moving targets
	// Simple prediction - could be enhanced with velocity prediction
	FVector Direction = (TargetLocation - StartLocation).GetSafeNormal();
	
	// Apply accuracy factor
	if (WeaponStats.Accuracy < 1.0f)
	{
		float AccuracyError = (1.0f - WeaponStats.Accuracy) * 10.0f; // 10 degrees max error
		FRotator ErrorRotation(
			FMath::RandRange(-AccuracyError, AccuracyError),
			FMath::RandRange(-AccuracyError, AccuracyError),
			0.0f
		);
		Direction = ErrorRotation.RotateVector(Direction);
	}
	
	return Direction;
}

FVector UOdysseyCombatWeaponComponent::CalculateFireDirection(FVector TargetLocation) const
{
	if (!GetOwner())
	{
		return FVector::ForwardVector;
	}

	FVector StartLocation = GetMountWorldLocation();
	return (TargetLocation - StartLocation).GetSafeNormal();
}

bool UOdysseyCombatWeaponComponent::PerformHitScan(FVector StartLocation, FVector Direction, float MaxRange, FHitResult& HitResult)
{
	FVector EndLocation = StartLocation + (Direction * MaxRange);
	
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.bTraceComplex = false;

	return GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		ECC_WorldDynamic,
		QueryParams
	);
}

float UOdysseyCombatWeaponComponent::CalculateDamage(bool& bIsCritical)
{
	float Damage = WeaponStats.Damage;
	
	// Check for critical hit
	bIsCritical = FMath::RandRange(0.0f, 1.0f) <= WeaponStats.CriticalChance;
	if (bIsCritical)
	{
		Damage *= WeaponStats.CriticalMultiplier;
	}
	
	return Damage;
}

bool UOdysseyCombatWeaponComponent::ApplyDamageToTarget(AActor* Target, float Damage, bool bIsCritical)
{
	if (!Target)
	{
		return false;
	}

	// Try to apply damage through health component
	UNPCHealthComponent* HealthComp = Target->FindComponentByClass<UNPCHealthComponent>();
	if (HealthComp)
	{
		FName DamageType = FName("Weapon"); // Could be more specific based on weapon type
		HealthComp->TakeDamage(Damage, GetOwner(), DamageType);
		return true;
	}

	return false;
}

void UOdysseyCombatWeaponComponent::ChangeWeaponState(EWeaponState NewState)
{
	if (CurrentState != NewState)
	{
		EWeaponState OldState = CurrentState;
		CurrentState = NewState;
		OnWeaponStateChanged(OldState, NewState);
	}
}

FVector UOdysseyCombatWeaponComponent::GetMountWorldLocation() const
{
	if (!GetOwner())
	{
		return FVector::ZeroVector;
	}

	FTransform OwnerTransform = GetOwner()->GetActorTransform();
	return OwnerTransform.TransformPosition(MountOffset);
}

FRotator UOdysseyCombatWeaponComponent::GetMountWorldRotation() const
{
	if (!GetOwner())
	{
		return FRotator::ZeroRotator;
	}

	return GetOwner()->GetActorRotation();
}

void UOdysseyCombatWeaponComponent::BroadcastWeaponEvent(EOdysseyEventType EventType, AActor* Target)
{
	if (!bBroadcastWeaponEvents)
	{
		return;
	}

	UOdysseyEventBus* Bus = GetEventBus();
	if (!Bus)
	{
		return;
	}

	FWeaponEventPayload EventPayload;
	EventPayload.Initialize(EventType, GetOwner());
	EventPayload.WeaponType = WeaponType;
	EventPayload.WeaponState = CurrentState;
	EventPayload.WeaponId = WeaponId;
	EventPayload.ChargeLevel = CurrentChargeLevel;
	EventPayload.Target = Target;

	// Convert to base event payload for publishing
	FOdysseyEventPayload BasePayload;
	BasePayload.Initialize(EventType, GetOwner());

	Bus->PublishEvent(BasePayload);
}

void UOdysseyCombatWeaponComponent::InitializeEventSubscriptions()
{
	// Subscribe to targeting events, energy events, etc.
	// Implementation would depend on the full event bus system
}

void UOdysseyCombatWeaponComponent::CleanupEventSubscriptions()
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

void UOdysseyCombatWeaponComponent::OnTargetChangedEvent(const FOdysseyEventPayload& Payload)
{
	// React to target changes
}

void UOdysseyCombatWeaponComponent::OnEnergyChangedEvent(const FOdysseyEventPayload& Payload)
{
	// React to energy changes - might need to stop auto-firing if out of energy
	if (!HasEnoughEnergy() && CurrentState != EWeaponState::OutOfAmmo)
	{
		ChangeWeaponState(EWeaponState::OutOfAmmo);
	}
	else if (HasEnoughEnergy() && CurrentState == EWeaponState::OutOfAmmo)
	{
		ChangeWeaponState(EWeaponState::Ready);
	}
}

FWeaponFireResult UOdysseyCombatWeaponComponent::FireInternal(AActor* Target, FVector TargetLocation)
{
	FWeaponFireResult Result;

	// Validate fire conditions
	FName FailureReason;
	if (!ValidateFireConditions(FailureReason))
	{
		Result.FailureReason = FailureReason;
		return Result;
	}

	// Consume energy
	if (!ConsumeEnergyForShot())
	{
		Result.FailureReason = FName("InsufficientEnergy");
		return Result;
	}

	// Calculate fire direction
	FVector FireDirection;
	if (Target)
	{
		FireDirection = CalculateFireDirection(Target);
	}
	else
	{
		FireDirection = CalculateFireDirection(TargetLocation);
	}

	FVector StartLocation = GetMountWorldLocation();
	
	// Perform hit scan or spawn projectile based on weapon type
	bool bHit = false;
	FHitResult HitResult;
	
	if (WeaponType == EWeaponType::Laser || WeaponType == EWeaponType::Kinetic)
	{
		// Instant hit weapons
		bHit = PerformHitScan(StartLocation, FireDirection, WeaponStats.Range, HitResult);
	}
	else
	{
		// Projectile weapons - for now, we'll simulate instant hit
		// In a full implementation, you'd spawn projectile actors
		bHit = PerformHitScan(StartLocation, FireDirection, WeaponStats.Range, HitResult);
	}

	// Calculate damage
	bool bIsCritical;
	float Damage = CalculateDamage(bIsCritical);

	// Apply damage if we hit something
	if (bHit && HitResult.GetActor())
	{
		Result.bHitTarget = ApplyDamageToTarget(HitResult.GetActor(), Damage, bIsCritical);
		Result.HitActor = HitResult.GetActor();
		Result.HitLocation = HitResult.Location;
		Result.DamageDealt = Damage;
		Result.bWasCritical = bIsCritical;

		// Play impact effects
		PlayImpactEffect(HitResult.Location, HitResult.GetActor());

		OnTargetHit(HitResult.GetActor(), Damage, bIsCritical);
	}

	// Play visual and audio effects
	PlayMuzzleFlash();
	PlayFireSound();
	
	if (bHit)
	{
		SpawnProjectileTrail(StartLocation, HitResult.Location);
	}
	else
	{
		FVector EndLocation = StartLocation + (FireDirection * WeaponStats.Range);
		SpawnProjectileTrail(StartLocation, EndLocation);
	}

	// Update weapon state
	LastFireTime = FPlatformTime::Seconds();
	ChangeWeaponState(EWeaponState::Reloading);

	// Fire successful
	Result.bFireSuccessful = true;
	
	// Broadcast events
	BroadcastWeaponEvent(EOdysseyEventType::AttackHit, Target);
	OnWeaponFired(Target, Result.bHitTarget, Result.DamageDealt);

	return Result;
}

bool UOdysseyCombatWeaponComponent::ValidateFireConditions(FName& FailureReason)
{
	if (!bWeaponEnabled)
	{
		FailureReason = FName("WeaponDisabled");
		return false;
	}

	if (CurrentState != EWeaponState::Ready && CurrentState != EWeaponState::Charging)
	{
		FailureReason = FName("WeaponNotReady");
		return false;
	}

	if (!HasEnoughEnergy())
	{
		FailureReason = FName("InsufficientEnergy");
		return false;
	}

	float TimeSinceLastFire = FPlatformTime::Seconds() - LastFireTime;
	float FireCooldown = 1.0f / WeaponStats.RateOfFire;
	if (TimeSinceLastFire < FireCooldown)
	{
		FailureReason = FName("OnCooldown");
		return false;
	}

	return true;
}
