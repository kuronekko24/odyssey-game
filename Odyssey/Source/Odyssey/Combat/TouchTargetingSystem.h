// TouchTargetingSystem.h
// Phase 3: Touch-based targeting system for mobile combat
// Handles touch-to-world raycasting, target selection, auto-targeting, and validation

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatTypes.h"
#include "TouchTargetingSystem.generated.h"

// Forward declarations
class UNPCHealthComponent;
class UNPCBehaviorComponent;
class UCameraComponent;

/**
 * Delegate fired when the current target changes (or is cleared).
 * Param 1: Previous target snapshot (may be invalid if none)
 * Param 2: New target snapshot (may be invalid if cleared)
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnTargetChanged,
	const FCombatTargetSnapshot&, PreviousTarget,
	const FCombatTargetSnapshot&, NewTarget
);

/**
 * Delegate fired when a touch hits a valid targetable actor.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnTouchTargetHit,
	AActor*, HitActor,
	FVector2D, ScreenLocation
);

/**
 * Touch-Based Targeting System
 *
 * Mobile-first targeting for arcade space combat:
 * - Touch an enemy ship to select it as the current target
 * - Auto-targeting finds the best nearby hostile when no manual selection
 * - Priority scoring weights distance, health, and hostility
 * - Expanded touch hit zones for comfortable mobile play
 * - Line-of-sight validation to prevent shooting through obstacles
 * - Continuous target tracking with automatic invalidation
 *
 * Performance considerations:
 * - Ticks at 10 Hz (not every frame) for auto-targeting scans
 * - Uses sphere overlap instead of per-actor iteration
 * - Caches camera component reference
 * - Minimizes per-frame allocations
 */
UCLASS(ClassGroup=(Odyssey), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UTouchTargetingSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UTouchTargetingSystem();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// Touch Input API
	// ============================================================================

	/**
	 * Process a touch event for target selection.
	 * Call from the touch interface when a tap occurs in the game world area.
	 *
	 * @param ScreenPosition  Touch coordinates in screen space
	 * @return true if a valid target was acquired from this touch
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Targeting")
	bool HandleTouch(FVector2D ScreenPosition);

	// ============================================================================
	// Target Management
	// ============================================================================

	/**
	 * Programmatically select a specific actor as the target.
	 * Runs full validation unless bSkipValidation is true.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Targeting")
	bool SelectTarget(AActor* TargetActor, bool bSkipValidation = false);

	/** Clear the current target and broadcast the change event. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Targeting")
	void ClearTarget();

	/** Let the system automatically choose the best available target. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Targeting")
	bool AutoSelectBestTarget();

	// ============================================================================
	// Queries
	// ============================================================================

	/** Get the currently locked target actor (may be nullptr). */
	UFUNCTION(BlueprintPure, Category = "Combat|Targeting")
	AActor* GetCurrentTarget() const;

	/** Get the full snapshot of the current target. */
	UFUNCTION(BlueprintPure, Category = "Combat|Targeting")
	FCombatTargetSnapshot GetCurrentTargetSnapshot() const { return CurrentTarget; }

	/** True if a valid, living target is currently selected. */
	UFUNCTION(BlueprintPure, Category = "Combat|Targeting")
	bool HasValidTarget() const;

	/** Distance to the current target, or MAX_FLT if none. */
	UFUNCTION(BlueprintPure, Category = "Combat|Targeting")
	float GetDistanceToTarget() const;

	/** Project current target's world position to screen coords. Returns false if off-screen. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Targeting")
	bool GetTargetScreenPosition(FVector2D& OutScreenPos) const;

	/** Get the reticle visual state for UI rendering. */
	UFUNCTION(BlueprintPure, Category = "Combat|Targeting")
	EReticleState GetReticleState() const { return CurrentReticleState; }

	// ============================================================================
	// Configuration
	// ============================================================================

	/** Runtime-tunable targeting configuration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Targeting")
	FTargetingConfig Config;

	// ============================================================================
	// Events
	// ============================================================================

	/** Broadcast when target changes (including clear). */
	UPROPERTY(BlueprintAssignable, Category = "Combat|Targeting|Events")
	FOnTargetChanged OnTargetChanged;

	/** Broadcast when a touch successfully hits a targetable actor. */
	UPROPERTY(BlueprintAssignable, Category = "Combat|Targeting|Events")
	FOnTouchTargetHit OnTouchTargetHit;

private:
	// ============================================================================
	// Internal State
	// ============================================================================

	/** Current locked target data */
	FCombatTargetSnapshot CurrentTarget;

	/** Visual state of the targeting reticle */
	EReticleState CurrentReticleState;

	/** Accumulator for auto-target scan interval */
	float AutoTargetTimer;

	/** Cached camera component */
	UPROPERTY()
	UCameraComponent* CachedCamera;

	// ============================================================================
	// Internal Methods
	// ============================================================================

	/**
	 * Perform a screen-to-world raycast to find what actor the player touched.
	 * Uses an expanded sphere trace around the ray for mobile-friendly hit detection.
	 */
	AActor* RaycastFromScreen(FVector2D ScreenPosition, FVector& OutWorldHit) const;

	/**
	 * Collect all actors with valid target tags within Config.MaxRange.
	 * Returns them sorted by proximity (ascending).
	 */
	TArray<AActor*> GatherCandidates() const;

	/**
	 * Build a full snapshot for the given actor.
	 * Reads health, behavior, position, and calculates priority score.
	 */
	FCombatTargetSnapshot BuildSnapshot(AActor* Actor) const;

	/**
	 * Check whether the given actor passes all validation checks:
	 * - Not null / not pending kill
	 * - Has at least one valid target tag
	 * - Not the owning player
	 * - Within range
	 * - Not dead
	 */
	bool ValidateTarget(AActor* Actor) const;

	/**
	 * Calculate priority score for auto-targeting.
	 * Higher is better. Factors: distance, hostility, health.
	 */
	float ScoreTarget(AActor* Actor) const;

	/**
	 * Perform a line trace from the owner to the target
	 * to check for occluding geometry.
	 */
	bool CheckLineOfSight(AActor* Target) const;

	/**
	 * Refresh the current target snapshot (distance, health, LOS).
	 * Clears the target if it becomes invalid.
	 */
	void RefreshCurrentTarget();

	/**
	 * Run auto-targeting logic when interval timer expires.
	 */
	void TickAutoTargeting(float DeltaTime);

	/**
	 * Update the reticle visual state based on current combat context.
	 */
	void UpdateReticleState();

	/**
	 * Resolve and cache the camera component used for screen-to-world projection.
	 */
	UCameraComponent* ResolveCamera() const;

	/**
	 * Internal helper to set target and broadcast events.
	 */
	void SetTargetInternal(const FCombatTargetSnapshot& NewSnapshot);
};
