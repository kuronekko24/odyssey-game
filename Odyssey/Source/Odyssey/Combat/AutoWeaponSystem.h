// AutoWeaponSystem.h
// Phase 3: Automatic weapon engagement system
// Fires at the locked target when in range, requires minimal player input

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatTypes.h"
#include "AutoWeaponSystem.generated.h"

// Forward declarations
class UTouchTargetingSystem;
class UNPCHealthComponent;
class UOdysseyActionButtonManager;

/**
 * Delegate broadcast on every successful weapon discharge.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnWeaponFired,
	const FCombatFireResult&, Result
);

/**
 * Delegate broadcast when weapon engagement state changes.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnEngagementStateChanged,
	ECombatEngagementState, OldState,
	ECombatEngagementState, NewState
);

/**
 * Automatic Weapon System
 *
 * Core behaviour loop:
 *   1. Every tick, check if a locked target exists (from TouchTargetingSystem)
 *   2. If target is within engagement range and has line of sight, enter Firing state
 *   3. Fire at the configured rate until target dies, moves out of range, or is deselected
 *   4. Apply damage through NPC health components; broadcast results
 *
 * Design goals:
 * - One-touch-to-engage: player taps target, weapons do the rest
 * - Lead-target prediction for moving enemies (when ProjectileSpeed > 0)
 * - Critical hits with visual/audio distinction
 * - Energy consumption integrated with the action button energy pool
 * - Performance: tick at 20 Hz, no per-frame allocations
 */
UCLASS(ClassGroup=(Odyssey), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UAutoWeaponSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UAutoWeaponSystem();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// Weapon Control
	// ============================================================================

	/** Attempt a single manual shot at the current target. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
	FCombatFireResult FireOnce();

	/** Enable or disable automatic firing. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
	void SetAutoFireEnabled(bool bEnabled);

	/** Check if automatic firing is currently enabled. */
	UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
	bool IsAutoFireEnabled() const { return bAutoFireEnabled; }

	/** Enable or disable the entire weapon system. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
	void SetWeaponEnabled(bool bEnabled);

	/** Check if the weapon system is enabled. */
	UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
	bool IsWeaponEnabled() const { return bWeaponEnabled; }

	// ============================================================================
	// State Queries
	// ============================================================================

	/** Current engagement state. */
	UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
	ECombatEngagementState GetEngagementState() const { return EngagementState; }

	/** True if the weapon can fire right now (cooldown, energy, target checks). */
	UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
	bool CanFire() const;

	/** Fraction of cooldown elapsed (0 = just fired, 1 = ready). */
	UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
	float GetCooldownProgress() const;

	/** Session combat statistics. */
	UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
	FCombatSessionStats GetSessionStats() const { return SessionStats; }

	/** Reset session statistics. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
	void ResetSessionStats() { SessionStats.Reset(); }

	// ============================================================================
	// Configuration
	// ============================================================================

	/** Runtime-tunable weapon configuration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
	FAutoWeaponConfig Config;

	/** Link to the targeting system (auto-resolved if on same actor). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
	void SetTargetingSystem(UTouchTargetingSystem* System);

	// ============================================================================
	// Events
	// ============================================================================

	UPROPERTY(BlueprintAssignable, Category = "Combat|Weapon|Events")
	FOnWeaponFired OnWeaponFired;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Weapon|Events")
	FOnEngagementStateChanged OnEngagementStateChanged;

private:
	// ============================================================================
	// State
	// ============================================================================

	ECombatEngagementState EngagementState;
	bool bAutoFireEnabled;
	bool bWeaponEnabled;
	float TimeSinceLastShot;
	FCombatSessionStats SessionStats;

	UPROPERTY()
	UTouchTargetingSystem* TargetingSystem;

	UPROPERTY()
	UOdysseyActionButtonManager* EnergySource;

	// ============================================================================
	// Internal Methods
	// ============================================================================

	/** Core firing logic -- resolves target, performs hit check, applies damage. */
	FCombatFireResult FireInternal();

	/** Update engagement state based on target availability. */
	void UpdateEngagementState();

	/** Transition to a new engagement state. */
	void TransitionState(ECombatEngagementState NewState);

	/** Get the world-space muzzle origin. */
	FVector GetMuzzleLocation() const;

	/**
	 * Calculate the aim direction with optional lead-target prediction.
	 * If ProjectileSpeed == 0, simply aims directly at the target.
	 */
	FVector CalculateAimDirection(const FCombatTargetSnapshot& Target) const;

	/** Apply accuracy spread to a direction vector. */
	FVector ApplySpread(FVector Direction) const;

	/** Roll for critical hit. */
	bool RollCritical() const;

	/** Check if energy is available and consume it for one shot. */
	bool TryConsumeEnergy();

	/** Hitscan trace from muzzle along direction. */
	bool PerformHitscan(FVector Origin, FVector Direction, FHitResult& OutHit) const;

	/** Apply damage to the hit actor through its health component. */
	float ApplyDamage(AActor* Target, float Damage, bool bCritical);
};
