// CombatSystemController.h
// Phase 3: Master coordinator for the Combat & Targeting System
// Owns and wires the targeting, weapon, and feedback subsystems
// Integrates with existing action button and touch interface systems

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatTypes.h"
#include "CombatSystemController.generated.h"

// Forward declarations
class UTouchTargetingSystem;
class UAutoWeaponSystem;
class UCombatFeedbackSystem;
class UOdysseyTouchInterface;
class UOdysseyActionButtonManager;

/**
 * Delegate broadcast when the overall combat mode changes.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnCombatModeChanged,
	bool, bCombatActive,
	ECombatEngagementState, EngagementState
);

/**
 * Combat System Controller
 *
 * Single point of entry for the Phase 3 combat system.
 * Attach this component to the player ship actor to get:
 *
 *  1. Touch-based enemy targeting (via UTouchTargetingSystem)
 *  2. Automatic weapon firing when a target is locked in range (via UAutoWeaponSystem)
 *  3. Visual feedback -- reticle, damage numbers, health bars (via UCombatFeedbackSystem)
 *  4. Integration with the existing action button Attack/SpecialAttack buttons
 *  5. Integration with the existing touch interface for combat touches
 *
 * Subsystems are created as sibling components on the same actor.
 * They communicate through delegates and direct function calls --
 * no polling between systems.
 *
 * Performance:
 * - Controller itself ticks at 10 Hz for state checks
 * - Each subsystem ticks at its own optimized rate
 * - Total CPU budget target: < 0.5 ms per frame on mobile
 */
UCLASS(ClassGroup=(Odyssey), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UCombatSystemController : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatSystemController();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// System Lifecycle
	// ============================================================================

	/** Initialize and wire all combat subsystems. Safe to call multiple times. */
	UFUNCTION(BlueprintCallable, Category = "Combat|System")
	void InitializeCombat();

	/** Shut down combat, clear targets, disable weapons. */
	UFUNCTION(BlueprintCallable, Category = "Combat|System")
	void ShutdownCombat();

	/** Enable or disable the entire combat system at runtime. */
	UFUNCTION(BlueprintCallable, Category = "Combat|System")
	void SetCombatEnabled(bool bEnabled);

	/** Check if combat is currently enabled and initialized. */
	UFUNCTION(BlueprintPure, Category = "Combat|System")
	bool IsCombatEnabled() const { return bCombatEnabled && bInitialized; }

	// ============================================================================
	// Touch Input Integration
	// ============================================================================

	/**
	 * Handle a touch event for combat.
	 * Call this from UOdysseyTouchInterface when the player taps in the game world area.
	 * Returns true if the combat system consumed the touch.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Input")
	bool HandleCombatTouch(FVector2D ScreenPosition);

	// ============================================================================
	// Action Button Integration
	// ============================================================================

	/**
	 * Handle the Attack action button press.
	 * If no target is selected, auto-selects one. Then fires weapon.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	bool HandleAttackAction();

	/**
	 * Handle the Special Attack action button press.
	 * Reserved for future charged/special weapons.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	bool HandleSpecialAttackAction();

	/**
	 * Register Attack and SpecialAttack buttons with the action button manager.
	 * Called automatically during initialization if an action button manager exists.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	void RegisterCombatActions();

	// ============================================================================
	// Subsystem Access
	// ============================================================================

	UFUNCTION(BlueprintPure, Category = "Combat|System")
	UTouchTargetingSystem* GetTargetingSystem() const { return TargetingSystem; }

	UFUNCTION(BlueprintPure, Category = "Combat|System")
	UAutoWeaponSystem* GetWeaponSystem() const { return WeaponSystem; }

	UFUNCTION(BlueprintPure, Category = "Combat|System")
	UCombatFeedbackSystem* GetFeedbackSystem() const { return FeedbackSystem; }

	// ============================================================================
	// Statistics
	// ============================================================================

	UFUNCTION(BlueprintPure, Category = "Combat|Stats")
	FCombatSessionStats GetSessionStats() const;

	UFUNCTION(BlueprintCallable, Category = "Combat|Stats")
	void ResetSessionStats();

	// ============================================================================
	// Configuration
	// ============================================================================

	/** Targeting configuration applied to the targeting subsystem */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Config")
	FTargetingConfig TargetingConfig;

	/** Weapon configuration applied to the weapon subsystem */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Config")
	FAutoWeaponConfig WeaponConfig;

	/** Feedback configuration applied to the feedback subsystem */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Config")
	FCombatFeedbackConfig FeedbackConfig;

	/** Whether to automatically enable combat on BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Config")
	bool bAutoEnable;

	/** Whether to auto-register combat actions with the action button manager */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Config")
	bool bAutoRegisterActions;

	/** Apply current config values to all subsystems */
	UFUNCTION(BlueprintCallable, Category = "Combat|Config")
	void ApplyConfiguration();

	// ============================================================================
	// Events
	// ============================================================================

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnCombatModeChanged OnCombatModeChanged;

private:
	// ============================================================================
	// State
	// ============================================================================

	bool bInitialized;
	bool bCombatEnabled;
	bool bActionsRegistered;

	// ============================================================================
	// Subsystem References (owned as sibling components)
	// ============================================================================

	UPROPERTY()
	UTouchTargetingSystem* TargetingSystem;

	UPROPERTY()
	UAutoWeaponSystem* WeaponSystem;

	UPROPERTY()
	UCombatFeedbackSystem* FeedbackSystem;

	// ============================================================================
	// External System References
	// ============================================================================

	UPROPERTY()
	UOdysseyTouchInterface* TouchInterface;

	UPROPERTY()
	UOdysseyActionButtonManager* ActionButtonManager;

	// ============================================================================
	// Internal Methods
	// ============================================================================

	/** Find or create all required subsystem components. */
	void EnsureSubsystems();

	/** Wire subsystems together (set references, bind delegates). */
	void WireSubsystems();

	/** Push config values to subsystems. */
	void PushConfiguration();

	/** Find external integration points (touch interface, action buttons). */
	void FindExternalSystems();

	/** Internal callback when engagement state changes. */
	UFUNCTION()
	void OnEngagementStateChanged(ECombatEngagementState OldState, ECombatEngagementState NewState);
};
