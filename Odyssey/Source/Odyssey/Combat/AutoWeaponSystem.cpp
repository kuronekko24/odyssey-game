// AutoWeaponSystem.cpp
// Phase 3: Automatic weapon engagement implementation

#include "AutoWeaponSystem.h"
#include "TouchTargetingSystem.h"
#include "NPCHealthComponent.h"
#include "OdysseyActionButton.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"

UAutoWeaponSystem::UAutoWeaponSystem()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f; // 20 Hz for responsive combat feel

	EngagementState = ECombatEngagementState::Idle;
	bAutoFireEnabled = true;
	bWeaponEnabled = true;
	TimeSinceLastShot = 0.0f;
	TargetingSystem = nullptr;
	EnergySource = nullptr;
}

void UAutoWeaponSystem::BeginPlay()
{
	Super::BeginPlay();

	// Auto-resolve sibling components on the same actor
	if (GetOwner())
	{
		if (!TargetingSystem)
		{
			TargetingSystem = GetOwner()->FindComponentByClass<UTouchTargetingSystem>();
		}
		EnergySource = GetOwner()->FindComponentByClass<UOdysseyActionButtonManager>();
	}
}

void UAutoWeaponSystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TransitionState(ECombatEngagementState::Idle);
	Super::EndPlay(EndPlayReason);
}

void UAutoWeaponSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bWeaponEnabled)
	{
		return;
	}

	TimeSinceLastShot += DeltaTime;

	// Update engagement state machine
	UpdateEngagementState();

	// Auto-fire logic
	if (bAutoFireEnabled && EngagementState == ECombatEngagementState::Firing)
	{
		if (CanFire())
		{
			FCombatFireResult Result = FireInternal();
			if (Result.bFired)
			{
				OnWeaponFired.Broadcast(Result);
			}
		}
	}

	// Track engagement duration
	if (EngagementState == ECombatEngagementState::Firing ||
		EngagementState == ECombatEngagementState::Locked)
	{
		SessionStats.EngagementDuration += DeltaTime;
	}
}

// ============================================================================
// Weapon Control
// ============================================================================

FCombatFireResult UAutoWeaponSystem::FireOnce()
{
	if (!CanFire())
	{
		FCombatFireResult Fail;
		Fail.FailReason = FName("CannotFire");
		return Fail;
	}

	FCombatFireResult Result = FireInternal();
	if (Result.bFired)
	{
		OnWeaponFired.Broadcast(Result);
	}
	return Result;
}

void UAutoWeaponSystem::SetAutoFireEnabled(bool bEnabled)
{
	bAutoFireEnabled = bEnabled;
}

void UAutoWeaponSystem::SetWeaponEnabled(bool bEnabled)
{
	bWeaponEnabled = bEnabled;
	if (!bEnabled)
	{
		TransitionState(ECombatEngagementState::Idle);
	}
}

// ============================================================================
// State Queries
// ============================================================================

bool UAutoWeaponSystem::CanFire() const
{
	if (!bWeaponEnabled)
	{
		return false;
	}

	// Rate of fire cooldown
	const float Cooldown = 1.0f / FMath::Max(Config.FireRate, 0.1f);
	if (TimeSinceLastShot < Cooldown)
	{
		return false;
	}

	// Must have targeting system with a valid target
	if (!TargetingSystem || !TargetingSystem->HasValidTarget())
	{
		return false;
	}

	// Target must be in weapon range
	const float Dist = TargetingSystem->GetDistanceToTarget();
	if (Dist > Config.EngagementRange)
	{
		return false;
	}

	// Energy check (non-blocking -- just a query)
	if (Config.EnergyCost > 0 && EnergySource)
	{
		if (EnergySource->GetCurrentEnergy() < Config.EnergyCost)
		{
			return false;
		}
	}

	return true;
}

float UAutoWeaponSystem::GetCooldownProgress() const
{
	const float Cooldown = 1.0f / FMath::Max(Config.FireRate, 0.1f);
	return FMath::Clamp(TimeSinceLastShot / Cooldown, 0.0f, 1.0f);
}

void UAutoWeaponSystem::SetTargetingSystem(UTouchTargetingSystem* System)
{
	TargetingSystem = System;
}

// ============================================================================
// Internal: Firing Logic
// ============================================================================

FCombatFireResult UAutoWeaponSystem::FireInternal()
{
	FCombatFireResult Result;

	if (!TargetingSystem || !TargetingSystem->HasValidTarget())
	{
		Result.FailReason = FName("NoTarget");
		return Result;
	}

	// Consume energy
	if (!TryConsumeEnergy())
	{
		Result.FailReason = FName("NoEnergy");
		return Result;
	}

	const FCombatTargetSnapshot& TargetSnap = TargetingSystem->GetCurrentTargetSnapshot();
	const FVector MuzzleOrigin = GetMuzzleLocation();
	FVector AimDir = CalculateAimDirection(TargetSnap);
	AimDir = ApplySpread(AimDir);

	// Reset cooldown
	TimeSinceLastShot = 0.0f;
	Result.bFired = true;
	SessionStats.ShotsFired++;

	// Hitscan
	FHitResult Hit;
	const bool bHit = PerformHitscan(MuzzleOrigin, AimDir, Hit);

	if (bHit && Hit.GetActor())
	{
		Result.bHit = true;
		Result.HitActor = Hit.GetActor();
		Result.ImpactLocation = Hit.ImpactPoint;
		SessionStats.ShotsHit++;

		// Calculate damage
		const bool bCrit = RollCritical();
		float Damage = Config.BaseDamage;
		if (bCrit)
		{
			Damage *= Config.CritMultiplier;
			Result.bCritical = true;
			SessionStats.CriticalHits++;
		}

		// Apply damage
		Result.DamageDealt = ApplyDamage(Hit.GetActor(), Damage, bCrit);
		SessionStats.TotalDamageDealt += Result.DamageDealt;

		// Check for kill
		if (UNPCHealthComponent* HC = Hit.GetActor()->FindComponentByClass<UNPCHealthComponent>())
		{
			if (HC->IsDead())
			{
				Result.bKillingBlow = true;
				SessionStats.EnemiesDestroyed++;
			}
		}
	}
	else
	{
		// Miss -- record the projected endpoint
		Result.ImpactLocation = MuzzleOrigin + AimDir * Config.EngagementRange;
	}

	return Result;
}

// ============================================================================
// Internal: Engagement State Machine
// ============================================================================

void UAutoWeaponSystem::UpdateEngagementState()
{
	const bool bHasTarget = TargetingSystem && TargetingSystem->HasValidTarget();
	const float Dist = bHasTarget ? TargetingSystem->GetDistanceToTarget() : TNumericLimits<float>::Max();
	const bool bInRange = Dist <= Config.EngagementRange;

	switch (EngagementState)
	{
	case ECombatEngagementState::Idle:
	case ECombatEngagementState::Scanning:
		if (bHasTarget && bInRange)
		{
			TransitionState(ECombatEngagementState::Firing);
		}
		else if (bHasTarget)
		{
			TransitionState(ECombatEngagementState::Locked);
		}
		else
		{
			TransitionState(ECombatEngagementState::Scanning);
		}
		break;

	case ECombatEngagementState::Locked:
		if (!bHasTarget)
		{
			TransitionState(ECombatEngagementState::Scanning);
		}
		else if (bInRange)
		{
			TransitionState(ECombatEngagementState::Firing);
		}
		break;

	case ECombatEngagementState::Firing:
		if (!bHasTarget)
		{
			TransitionState(ECombatEngagementState::Scanning);
		}
		else if (!bInRange)
		{
			TransitionState(ECombatEngagementState::Locked);
		}
		break;

	default:
		break;
	}
}

void UAutoWeaponSystem::TransitionState(ECombatEngagementState NewState)
{
	if (EngagementState == NewState)
	{
		return;
	}

	ECombatEngagementState Old = EngagementState;
	EngagementState = NewState;
	OnEngagementStateChanged.Broadcast(Old, NewState);
}

// ============================================================================
// Internal: Aim & Ballistics
// ============================================================================

FVector UAutoWeaponSystem::GetMuzzleLocation() const
{
	if (!GetOwner())
	{
		return FVector::ZeroVector;
	}

	return GetOwner()->GetActorTransform().TransformPosition(Config.MuzzleOffset);
}

FVector UAutoWeaponSystem::CalculateAimDirection(const FCombatTargetSnapshot& Target) const
{
	FVector MuzzlePos = GetMuzzleLocation();
	FVector TargetPos = Target.WorldLocation;

	// Lead-target prediction for projectile weapons
	if (Config.ProjectileSpeed > 0.0f && !Target.Velocity.IsNearlyZero())
	{
		const float Distance = FVector::Dist(MuzzlePos, TargetPos);
		const float TimeToTarget = Distance / Config.ProjectileSpeed;
		TargetPos += Target.Velocity * TimeToTarget;
	}

	return (TargetPos - MuzzlePos).GetSafeNormal();
}

FVector UAutoWeaponSystem::ApplySpread(FVector Direction) const
{
	if (Config.Accuracy >= 1.0f)
	{
		return Direction;
	}

	// Convert inaccuracy to a cone half-angle in degrees
	const float MaxSpreadDeg = (1.0f - Config.Accuracy) * 8.0f;
	const float SpreadRad = FMath::DegreesToRadians(FMath::RandRange(0.0f, MaxSpreadDeg));
	const float SpinRad = FMath::RandRange(0.0f, 2.0f * PI);

	// Build a random direction within the spread cone
	const FRotator DirRot = Direction.Rotation();
	const FVector Right = FRotationMatrix(DirRot).GetUnitAxis(EAxis::Y);
	const FVector Up = FRotationMatrix(DirRot).GetUnitAxis(EAxis::Z);

	return (Direction + (Right * FMath::Sin(SpinRad) + Up * FMath::Cos(SpinRad)) * FMath::Sin(SpreadRad)).GetSafeNormal();
}

bool UAutoWeaponSystem::RollCritical() const
{
	return FMath::FRand() < Config.CritChance;
}

bool UAutoWeaponSystem::TryConsumeEnergy()
{
	if (Config.EnergyCost <= 0)
	{
		return true; // Free firing
	}

	if (EnergySource)
	{
		return EnergySource->SpendEnergy(Config.EnergyCost);
	}

	return true; // No energy system attached -- default to allowing fire
}

bool UAutoWeaponSystem::PerformHitscan(FVector Origin, FVector Direction, FHitResult& OutHit) const
{
	FVector End = Origin + Direction * Config.EngagementRange;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());
	Params.bTraceComplex = false;

	return GetWorld()->LineTraceSingleByChannel(
		OutHit,
		Origin,
		End,
		ECC_WorldDynamic,
		Params
	);
}

float UAutoWeaponSystem::ApplyDamage(AActor* Target, float Damage, bool bCritical)
{
	if (!Target)
	{
		return 0.0f;
	}

	UNPCHealthComponent* HC = Target->FindComponentByClass<UNPCHealthComponent>();
	if (HC)
	{
		FName DmgType = bCritical ? FName("WeaponCritical") : FName("Weapon");
		return HC->TakeDamage(Damage, GetOwner(), DmgType);
	}

	return 0.0f;
}
