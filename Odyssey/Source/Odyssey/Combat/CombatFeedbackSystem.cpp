// CombatFeedbackSystem.cpp
// Phase 3: Visual feedback system implementation

#include "CombatFeedbackSystem.h"
#include "TouchTargetingSystem.h"
#include "AutoWeaponSystem.h"
#include "NPCHealthComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UCombatFeedbackSystem::UCombatFeedbackSystem()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.033f; // ~30 Hz for smooth UI updates

	PulseAccumulator = 0.0f;
	TargetingSystem = nullptr;
	WeaponSystem = nullptr;

	// Pre-allocate pools
	DamageNumbers.Reserve(16);
	HealthBars.Reserve(12);
	HitMarkers.Reserve(8);
}

void UCombatFeedbackSystem::BeginPlay()
{
	Super::BeginPlay();

	// Auto-resolve sibling components
	if (GetOwner())
	{
		if (!TargetingSystem)
		{
			TargetingSystem = GetOwner()->FindComponentByClass<UTouchTargetingSystem>();
		}
		if (!WeaponSystem)
		{
			SetWeaponSystem(GetOwner()->FindComponentByClass<UAutoWeaponSystem>());
		}
	}
}

void UCombatFeedbackSystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DamageNumbers.Empty();
	HealthBars.Empty();
	HitMarkers.Empty();
	Super::EndPlay(EndPlayReason);
}

void UCombatFeedbackSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	PulseAccumulator += DeltaTime;

	UpdateReticle(DeltaTime);
	UpdateDamageNumbers(DeltaTime);
	UpdateHealthBars(DeltaTime);
	UpdateHitMarkers(DeltaTime);
}

// ============================================================================
// Event Receivers
// ============================================================================

void UCombatFeedbackSystem::NotifyHit(FVector WorldLocation, float Damage, bool bCritical)
{
	SpawnDamageNumber(WorldLocation, Damage, bCritical);
	
	if (FeedbackConfig.bShowHitMarkers)
	{
		SpawnHitMarker(WorldLocation, bCritical);
	}
}

void UCombatFeedbackSystem::NotifyDamage(AActor* DamagedActor, float NewHealthFraction)
{
	if (!DamagedActor)
	{
		return;
	}

	const bool bIsTargeted = TargetingSystem && TargetingSystem->GetCurrentTarget() == DamagedActor;
	TrackHealthBar(DamagedActor, NewHealthFraction, bIsTargeted);
}

void UCombatFeedbackSystem::NotifyKill(AActor* KilledActor)
{
	// Remove health bar for killed actor
	for (int32 i = HealthBars.Num() - 1; i >= 0; --i)
	{
		if (HealthBars[i].Actor.Get() == KilledActor)
		{
			HealthBars.RemoveAtSwap(i);
			break;
		}
	}
}

// ============================================================================
// Reticle Update
// ============================================================================

void UCombatFeedbackSystem::UpdateReticle(float DeltaTime)
{
	if (!TargetingSystem)
	{
		ReticleData.State = EReticleState::Hidden;
		return;
	}

	ReticleData.State = TargetingSystem->GetReticleState();
	ReticleData.Size = FeedbackConfig.ReticleSize;
	ReticleData.PulsePhase = FMath::Sin(PulseAccumulator * 4.0f) * 0.5f + 0.5f;

	if (TargetingSystem->HasValidTarget())
	{
		FVector2D ScreenPos;
		const bool bOnScreen = TargetingSystem->GetTargetScreenPosition(ScreenPos);
		ReticleData.ScreenPosition = ScreenPos;
		ReticleData.bOnScreen = bOnScreen;

		const FCombatTargetSnapshot& Snap = TargetingSystem->GetCurrentTargetSnapshot();
		ReticleData.DistanceToTarget = Snap.Distance;
		ReticleData.TargetHealthFraction = Snap.HealthFraction;

		// Color based on state
		if (ReticleData.State == EReticleState::Locked || ReticleData.State == EReticleState::Firing)
		{
			ReticleData.Color = FeedbackConfig.ReticleLockedColor;
		}
		else
		{
			ReticleData.Color = FeedbackConfig.ReticleOutOfRangeColor;
		}
	}
	else
	{
		ReticleData.bOnScreen = false;
	}
}

// ============================================================================
// Damage Numbers
// ============================================================================

void UCombatFeedbackSystem::UpdateDamageNumbers(float DeltaTime)
{
	for (int32 i = DamageNumbers.Num() - 1; i >= 0; --i)
	{
		FFloatingDamageNumber& DN = DamageNumbers[i];
		DN.Age += DeltaTime;
		DN.NormalizedAge = FMath::Clamp(DN.Age / DN.Lifetime, 0.0f, 1.0f);

		if (DN.IsExpired())
		{
			DamageNumbers.RemoveAtSwap(i);
			continue;
		}

		// Animate: drift upward and sideways
		FVector AnimatedPos = DN.WorldOrigin + FVector(DN.DriftX * DN.NormalizedAge, 0.0f, 80.0f * DN.NormalizedAge);
		FVector2D ScreenPos;
		if (WorldToScreen(AnimatedPos, ScreenPos))
		{
			DN.ScreenPosition = ScreenPos;
		}
	}
}

void UCombatFeedbackSystem::SpawnDamageNumber(FVector WorldLocation, float Damage, bool bCritical)
{
	// Evict oldest if at capacity
	if (DamageNumbers.Num() >= FeedbackConfig.MaxDamageNumbers)
	{
		// Find oldest
		int32 OldestIdx = 0;
		float OldestAge = -1.0f;
		for (int32 i = 0; i < DamageNumbers.Num(); ++i)
		{
			if (DamageNumbers[i].Age > OldestAge)
			{
				OldestAge = DamageNumbers[i].Age;
				OldestIdx = i;
			}
		}
		DamageNumbers.RemoveAtSwap(OldestIdx);
	}

	FFloatingDamageNumber DN;
	DN.WorldOrigin = WorldLocation + FVector(0, 0, 40.0f); // Slight upward offset
	DN.DamageAmount = Damage;
	DN.bIsCritical = bCritical;
	DN.Lifetime = FeedbackConfig.DamageNumberLifetime;
	DN.DriftX = FMath::RandRange(-30.0f, 30.0f);

	// Initial screen position
	WorldToScreen(DN.WorldOrigin, DN.ScreenPosition);

	DamageNumbers.Add(DN);
	OnDamageNumberSpawned.Broadcast(Damage, bCritical);
}

// ============================================================================
// Health Bars
// ============================================================================

void UCombatFeedbackSystem::UpdateHealthBars(float DeltaTime)
{
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const float FadeOutTime = 4.0f; // Hide health bars 4 seconds after last damage

	for (int32 i = HealthBars.Num() - 1; i >= 0; --i)
	{
		FTrackedHealthBar& HB = HealthBars[i];

		// Remove invalid actors
		if (!HB.IsValid())
		{
			HealthBars.RemoveAtSwap(i);
			continue;
		}

		AActor* Actor = HB.Actor.Get();
		const bool bIsTarget = TargetingSystem && TargetingSystem->GetCurrentTarget() == Actor;
		HB.bIsTargeted = bIsTarget;

		// Refresh health
		if (UNPCHealthComponent* HC = Actor->FindComponentByClass<UNPCHealthComponent>())
		{
			HB.HealthFraction = HC->GetHealthPercentage();

			// Remove dead actors
			if (HC->IsDead())
			{
				HealthBars.RemoveAtSwap(i);
				continue;
			}
		}

		// Hide if full health and not targeted and faded out
		if (!FeedbackConfig.bShowEnemyHealthBars)
		{
			HealthBars.RemoveAtSwap(i);
			continue;
		}

		if (HB.HealthFraction >= 1.0f && !bIsTarget && (CurrentTime - HB.LastDamageTime) > FadeOutTime)
		{
			HealthBars.RemoveAtSwap(i);
			continue;
		}

		// Update screen position
		FVector BarWorldPos = Actor->GetActorLocation() + FVector(0, 0, 120.0f);
		HB.bOnScreen = WorldToScreen(BarWorldPos, HB.ScreenPosition);
	}
}

void UCombatFeedbackSystem::TrackHealthBar(AActor* Actor, float HealthFraction, bool bIsTargeted)
{
	// Find existing
	for (FTrackedHealthBar& HB : HealthBars)
	{
		if (HB.Actor.Get() == Actor)
		{
			HB.HealthFraction = HealthFraction;
			HB.LastDamageTime = GetWorld()->GetTimeSeconds();
			HB.bIsTargeted = bIsTargeted;
			return;
		}
	}

	// Create new
	FTrackedHealthBar NewHB;
	NewHB.Actor = Actor;
	NewHB.HealthFraction = HealthFraction;
	NewHB.LastDamageTime = GetWorld()->GetTimeSeconds();
	NewHB.bIsTargeted = bIsTargeted;
	HealthBars.Add(NewHB);
}

// ============================================================================
// Hit Markers
// ============================================================================

void UCombatFeedbackSystem::UpdateHitMarkers(float DeltaTime)
{
	for (int32 i = HitMarkers.Num() - 1; i >= 0; --i)
	{
		HitMarkers[i].Age += DeltaTime;
		if (HitMarkers[i].IsExpired())
		{
			HitMarkers.RemoveAtSwap(i);
		}
	}
}

void UCombatFeedbackSystem::SpawnHitMarker(FVector WorldLocation, bool bCritical)
{
	FHitMarkerData HM;
	HM.bIsCritical = bCritical;
	HM.Lifetime = FeedbackConfig.HitMarkerDuration;
	WorldToScreen(WorldLocation, HM.ScreenPosition);

	// Cap at reasonable number
	if (HitMarkers.Num() >= 8)
	{
		HitMarkers.RemoveAtSwap(0);
	}

	HitMarkers.Add(HM);
	OnHitMarkerSpawned.Broadcast(bCritical);
}

// ============================================================================
// Utility
// ============================================================================

bool UCombatFeedbackSystem::WorldToScreen(FVector WorldLoc, FVector2D& OutScreen) const
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return false;
	}
	return UGameplayStatics::ProjectWorldToScreen(PC, WorldLoc, OutScreen, false);
}

// ============================================================================
// Component Wiring
// ============================================================================

void UCombatFeedbackSystem::SetTargetingSystem(UTouchTargetingSystem* System)
{
	TargetingSystem = System;
}

void UCombatFeedbackSystem::SetWeaponSystem(UAutoWeaponSystem* System)
{
	// Unbind old
	if (WeaponSystem)
	{
		WeaponSystem->OnWeaponFired.RemoveDynamic(this, &UCombatFeedbackSystem::OnWeaponFiredCallback);
	}

	WeaponSystem = System;

	// Bind new
	if (WeaponSystem)
	{
		WeaponSystem->OnWeaponFired.AddDynamic(this, &UCombatFeedbackSystem::OnWeaponFiredCallback);
	}
}

void UCombatFeedbackSystem::OnWeaponFiredCallback(const FCombatFireResult& Result)
{
	if (Result.bHit)
	{
		NotifyHit(Result.ImpactLocation, Result.DamageDealt, Result.bCritical);

		// Track health bar for the hit actor
		if (AActor* HitActor = Result.HitActor.Get())
		{
			if (UNPCHealthComponent* HC = HitActor->FindComponentByClass<UNPCHealthComponent>())
			{
				NotifyDamage(HitActor, HC->GetHealthPercentage());
			}

			if (Result.bKillingBlow)
			{
				NotifyKill(HitActor);
			}
		}
	}
}
