// CombatFeedbackSystem.h
// Phase 3: Visual feedback system for combat -- targeting reticles, damage numbers,
// health bars, hit markers, and weapon effects
// All rendering is Blueprint/Widget-delegated; this component manages state and data

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatTypes.h"
#include "CombatFeedbackSystem.generated.h"

// Forward declarations
class UTouchTargetingSystem;
class UAutoWeaponSystem;
class UNPCHealthComponent;

/**
 * A single floating damage number tracked by the feedback system.
 * These are lightweight structs -- no UObject overhead.
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FFloatingDamageNumber
{
	GENERATED_BODY()

	/** World location where the damage occurred */
	UPROPERTY(BlueprintReadOnly, Category = "Damage Number")
	FVector WorldOrigin;

	/** Current screen-space position (updated each frame) */
	UPROPERTY(BlueprintReadOnly, Category = "Damage Number")
	FVector2D ScreenPosition;

	/** Damage value to display */
	UPROPERTY(BlueprintReadOnly, Category = "Damage Number")
	float DamageAmount;

	/** Whether this was a critical hit */
	UPROPERTY(BlueprintReadOnly, Category = "Damage Number")
	bool bIsCritical;

	/** Time since creation */
	UPROPERTY(BlueprintReadOnly, Category = "Damage Number")
	float Age;

	/** Configured lifetime */
	UPROPERTY(BlueprintReadOnly, Category = "Damage Number")
	float Lifetime;

	/** Normalized age (0..1) for animation curves */
	UPROPERTY(BlueprintReadOnly, Category = "Damage Number")
	float NormalizedAge;

	/** Random horizontal drift for visual variety */
	float DriftX;

	FFloatingDamageNumber()
		: WorldOrigin(FVector::ZeroVector)
		, ScreenPosition(FVector2D::ZeroVector)
		, DamageAmount(0.0f)
		, bIsCritical(false)
		, Age(0.0f)
		, Lifetime(1.2f)
		, NormalizedAge(0.0f)
		, DriftX(0.0f)
	{
	}

	bool IsExpired() const { return Age >= Lifetime; }
};

/**
 * Per-enemy health bar tracking data.
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FTrackedHealthBar
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Health Bar")
	TWeakObjectPtr<AActor> Actor;

	UPROPERTY(BlueprintReadOnly, Category = "Health Bar")
	FVector2D ScreenPosition;

	UPROPERTY(BlueprintReadOnly, Category = "Health Bar")
	float HealthFraction;

	UPROPERTY(BlueprintReadOnly, Category = "Health Bar")
	float LastDamageTime;

	UPROPERTY(BlueprintReadOnly, Category = "Health Bar")
	bool bIsTargeted;

	UPROPERTY(BlueprintReadOnly, Category = "Health Bar")
	bool bOnScreen;

	FTrackedHealthBar()
		: ScreenPosition(FVector2D::ZeroVector)
		, HealthFraction(1.0f)
		, LastDamageTime(0.0f)
		, bIsTargeted(false)
		, bOnScreen(false)
	{
	}

	bool IsValid() const { return Actor.IsValid(); }
};

/**
 * Reticle display data for the UI layer.
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FReticleDisplayData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Reticle")
	FVector2D ScreenPosition;

	UPROPERTY(BlueprintReadOnly, Category = "Reticle")
	EReticleState State;

	UPROPERTY(BlueprintReadOnly, Category = "Reticle")
	float Size;

	UPROPERTY(BlueprintReadOnly, Category = "Reticle")
	FLinearColor Color;

	UPROPERTY(BlueprintReadOnly, Category = "Reticle")
	float PulsePhase;

	UPROPERTY(BlueprintReadOnly, Category = "Reticle")
	float DistanceToTarget;

	UPROPERTY(BlueprintReadOnly, Category = "Reticle")
	float TargetHealthFraction;

	UPROPERTY(BlueprintReadOnly, Category = "Reticle")
	bool bOnScreen;

	FReticleDisplayData()
		: ScreenPosition(FVector2D::ZeroVector)
		, State(EReticleState::Hidden)
		, Size(72.0f)
		, Color(FLinearColor::Red)
		, PulsePhase(0.0f)
		, DistanceToTarget(0.0f)
		, TargetHealthFraction(1.0f)
		, bOnScreen(false)
	{
	}
};

/**
 * Hit marker display data.
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FHitMarkerData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Hit Marker")
	FVector2D ScreenPosition;

	UPROPERTY(BlueprintReadOnly, Category = "Hit Marker")
	bool bIsCritical;

	UPROPERTY(BlueprintReadOnly, Category = "Hit Marker")
	float Age;

	UPROPERTY(BlueprintReadOnly, Category = "Hit Marker")
	float Lifetime;

	FHitMarkerData()
		: ScreenPosition(FVector2D::ZeroVector)
		, bIsCritical(false)
		, Age(0.0f)
		, Lifetime(0.25f)
	{
	}

	bool IsExpired() const { return Age >= Lifetime; }
};

/**
 * Delegate: new damage number spawned (for sound/haptic triggers)
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnDamageNumberSpawned,
	float, Damage,
	bool, bIsCritical
);

/**
 * Delegate: hit marker spawned
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnHitMarkerSpawned,
	bool, bIsCritical
);

/**
 * Combat Feedback System
 *
 * Manages all visual combat feedback data that the UI layer reads each frame:
 * - Targeting reticle state and position
 * - Floating damage numbers with drift animation
 * - Enemy health bars (only shown when damaged or targeted)
 * - Hit marker flashes
 *
 * This component does NOT create widgets directly. Instead it exposes
 * read-only data arrays that a UMG widget or HUD class polls via
 * Blueprint-accessible getters. This separation keeps the logic
 * testable and widget-toolkit-agnostic.
 *
 * Performance notes:
 * - Ticks at 30 Hz for smooth UI updates
 * - Fixed-size pools with oldest-eviction for damage numbers and hit markers
 * - Minimal allocations per frame
 */
UCLASS(ClassGroup=(Odyssey), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UCombatFeedbackSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatFeedbackSystem();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// Event Receivers (call from weapon/targeting systems)
	// ============================================================================

	/** Notify that a shot hit a target. Creates damage number + hit marker. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Feedback")
	void NotifyHit(FVector WorldLocation, float Damage, bool bCritical);

	/** Notify that an enemy took damage (for health bar tracking). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Feedback")
	void NotifyDamage(AActor* DamagedActor, float NewHealthFraction);

	/** Notify that an enemy was destroyed. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Feedback")
	void NotifyKill(AActor* KilledActor);

	// ============================================================================
	// Data Getters (polled by UI widgets)
	// ============================================================================

	/** Get the current targeting reticle display data. */
	UFUNCTION(BlueprintPure, Category = "Combat|Feedback")
	FReticleDisplayData GetReticleData() const { return ReticleData; }

	/** Get all active floating damage numbers. */
	UFUNCTION(BlueprintPure, Category = "Combat|Feedback")
	const TArray<FFloatingDamageNumber>& GetActiveDamageNumbers() const { return DamageNumbers; }

	/** Get all tracked enemy health bars. */
	UFUNCTION(BlueprintPure, Category = "Combat|Feedback")
	const TArray<FTrackedHealthBar>& GetTrackedHealthBars() const { return HealthBars; }

	/** Get all active hit markers. */
	UFUNCTION(BlueprintPure, Category = "Combat|Feedback")
	const TArray<FHitMarkerData>& GetActiveHitMarkers() const { return HitMarkers; }

	// ============================================================================
	// Configuration
	// ============================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Feedback")
	FCombatFeedbackConfig FeedbackConfig;

	/** Set the targeting system to read reticle state from. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Feedback")
	void SetTargetingSystem(UTouchTargetingSystem* System);

	/** Set the weapon system to auto-subscribe to fire events. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Feedback")
	void SetWeaponSystem(UAutoWeaponSystem* System);

	// ============================================================================
	// Events
	// ============================================================================

	UPROPERTY(BlueprintAssignable, Category = "Combat|Feedback|Events")
	FOnDamageNumberSpawned OnDamageNumberSpawned;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Feedback|Events")
	FOnHitMarkerSpawned OnHitMarkerSpawned;

private:
	// ============================================================================
	// State
	// ============================================================================

	FReticleDisplayData ReticleData;
	TArray<FFloatingDamageNumber> DamageNumbers;
	TArray<FTrackedHealthBar> HealthBars;
	TArray<FHitMarkerData> HitMarkers;
	float PulseAccumulator;

	UPROPERTY()
	UTouchTargetingSystem* TargetingSystem;

	UPROPERTY()
	UAutoWeaponSystem* WeaponSystem;

	// ============================================================================
	// Internal Methods
	// ============================================================================

	/** Update the targeting reticle display data from the targeting system. */
	void UpdateReticle(float DeltaTime);

	/** Tick all active damage numbers (position, age, eviction). */
	void UpdateDamageNumbers(float DeltaTime);

	/** Update health bar screen positions and cull off-screen or full-health bars. */
	void UpdateHealthBars(float DeltaTime);

	/** Tick hit markers and remove expired. */
	void UpdateHitMarkers(float DeltaTime);

	/** Add a new damage number, evicting the oldest if at capacity. */
	void SpawnDamageNumber(FVector WorldLocation, float Damage, bool bCritical);

	/** Add or update a tracked health bar for an actor. */
	void TrackHealthBar(AActor* Actor, float HealthFraction, bool bIsTargeted);

	/** Add a new hit marker. */
	void SpawnHitMarker(FVector WorldLocation, bool bCritical);

	/** Project a world location to screen space. Returns false if behind camera. */
	bool WorldToScreen(FVector WorldLoc, FVector2D& OutScreen) const;

	/** Auto-subscribe to weapon fire events when the weapon system is set. */
	UFUNCTION()
	void OnWeaponFiredCallback(const FCombatFireResult& Result);
};
