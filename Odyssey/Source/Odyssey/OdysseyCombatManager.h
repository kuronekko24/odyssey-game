// OdysseyCombatManager.h
// Master combat system manager that coordinates targeting, weapons, and UI
// Integrates with existing touch interface and action button systems

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OdysseyActionEvent.h"
#include "OdysseyCombatManager.generated.h"

// Forward declarations
class UOdysseyCombatTargetingComponent;
class UOdysseyCombatWeaponComponent;
class UOdysseyCombatUIComponent;
class UOdysseyTouchInterface;
class UOdysseyActionButtonManager;

/**
 * Combat system state
 */
UENUM(BlueprintType)
enum class ECombatSystemState : uint8
{
	Inactive,       // Combat system is off
	Standby,        // Ready for combat but not engaged
	Engaging,       // Actively in combat
	Disabled        // Temporarily disabled
};

/**
 * Combat configuration for different scenarios
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FCombatConfiguration
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Config")
	bool bEnableAutoTargeting;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Config")
	bool bEnableAutoFiring;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Config")
	bool bShowTargetIndicators;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Config")
	bool bShowHealthBars;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Config")
	bool bShowDamageNumbers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Config")
	float TargetingRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Config")
	float WeaponRange;

	FCombatConfiguration()
	{
		bEnableAutoTargeting = true;
		bEnableAutoFiring = true;
		bShowTargetIndicators = true;
		bShowHealthBars = true;
		bShowDamageNumbers = true;
		TargetingRange = 2000.0f;
		WeaponRange = 1500.0f;
	}
};

/**
 * Combat statistics for tracking performance
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FCombatStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat Stats")
	int32 ShotsFired;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Stats")
	int32 ShotsHit;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Stats")
	int32 CriticalHits;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Stats")
	float TotalDamageDealt;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Stats")
	int32 EnemiesDestroyed;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Stats")
	float CombatTime;

	FCombatStats()
	{
		ShotsFired = 0;
		ShotsHit = 0;
		CriticalHits = 0;
		TotalDamageDealt = 0.0f;
		EnemiesDestroyed = 0;
		CombatTime = 0.0f;
	}

	float GetAccuracy() const
	{
		return ShotsFired > 0 ? (float)ShotsHit / (float)ShotsFired : 0.0f;
	}

	float GetCriticalRate() const
	{
		return ShotsHit > 0 ? (float)CriticalHits / (float)ShotsHit : 0.0f;
	}
};

/**
 * Combat Manager Component
 * 
 * Master coordinator for the combat system:
 * - Manages targeting, weapons, and UI components
 * - Handles touch input for combat actions
 * - Integrates with existing action button system
 * - Provides unified combat configuration
 * - Tracks combat statistics
 * - Mobile performance optimization
 * - Event-driven architecture integration
 */
UCLASS(ClassGroup=(Odyssey), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UOdysseyCombatManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdysseyCombatManager();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// Combat System Control
	// ============================================================================

	/**
	 * Initialize the entire combat system
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat System")
	void InitializeCombatSystem();

	/**
	 * Shutdown the combat system
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat System")
	void ShutdownCombatSystem();

	/**
	 * Enable or disable the combat system
	 * @param bEnabled Whether to enable combat
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat System")
	void SetCombatEnabled(bool bEnabled);

	/**
	 * Set combat system state
	 * @param NewState New combat state
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat System")
	void SetCombatState(ECombatSystemState NewState);

	/**
	 * Get current combat system state
	 */
	UFUNCTION(BlueprintPure, Category = "Combat System")
	ECombatSystemState GetCombatState() const { return CurrentState; }

	/**
	 * Check if combat system is active
	 */
	UFUNCTION(BlueprintPure, Category = "Combat System")
	bool IsCombatActive() const;

	// ============================================================================
	// Touch Input Integration
	// ============================================================================

	/**
	 * Handle touch input for targeting
	 * @param TouchLocation Screen coordinates of touch
	 * @return True if touch was handled by combat system
	 */
	UFUNCTION(BlueprintCallable, Category = "Touch Input")
	bool HandleCombatTouch(FVector2D TouchLocation);

	/**
	 * Handle touch input for manual firing
	 * @param TouchLocation Screen coordinates of touch
	 * @return True if weapon was fired
	 */
	UFUNCTION(BlueprintCallable, Category = "Touch Input")
	bool HandleFireTouch(FVector2D TouchLocation);

	/**
	 * Register with touch interface for combat input
	 */
	UFUNCTION(BlueprintCallable, Category = "Touch Input")
	void RegisterTouchInput();

	/**
	 * Unregister from touch interface
	 */
	UFUNCTION(BlueprintCallable, Category = "Touch Input")
	void UnregisterTouchInput();

	// ============================================================================
	// Action Button Integration
	// ============================================================================

	/**
	 * Handle attack action button press
	 * @return True if attack was executed
	 */
	UFUNCTION(BlueprintCallable, Category = "Action Buttons")
	bool HandleAttackAction();

	/**
	 * Handle special attack action button press
	 * @return True if special attack was executed
	 */
	UFUNCTION(BlueprintCallable, Category = "Action Buttons")
	bool HandleSpecialAttackAction();

	/**
	 * Register combat actions with action button manager
	 */
	UFUNCTION(BlueprintCallable, Category = "Action Buttons")
	void RegisterCombatActions();

	/**
	 * Unregister combat actions
	 */
	UFUNCTION(BlueprintCallable, Category = "Action Buttons")
	void UnregisterCombatActions();

	// ============================================================================
	// Targeting Control
	// ============================================================================

	/**
	 * Set targeting mode
	 * @param Mode New targeting mode
	 */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	void SetTargetingMode(ETargetingMode Mode);

	/**
	 * Get current target
	 */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	AActor* GetCurrentTarget() const;

	/**
	 * Manually select target
	 * @param Target Actor to target
	 * @return True if target was selected
	 */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	bool SelectTarget(AActor* Target);

	/**
	 * Clear current target
	 */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	void ClearTarget();

	/**
	 * Find and auto-select best target
	 */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	bool AutoSelectTarget();

	// ============================================================================
	// Weapon Control
	// ============================================================================

	/**
	 * Fire weapon at current target
	 * @return Fire result
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	FWeaponFireResult FireWeapon();

	/**
	 * Set weapon auto-fire enabled
	 * @param bEnabled Whether to enable auto-firing
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void SetAutoFireEnabled(bool bEnabled);

	/**
	 * Check if weapon can fire
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	bool CanFireWeapon() const;

	/**
	 * Get weapon charge level
	 */
	UFUNCTION(BlueprintPure, Category = "Weapons")
	float GetWeaponChargeLevel() const;

	// ============================================================================
	// Configuration
	// ============================================================================

	/**
	 * Set combat configuration
	 * @param Config New combat configuration
	 */
	UFUNCTION(BlueprintCallable, Category = "Configuration")
	void SetCombatConfiguration(const FCombatConfiguration& Config);

	/**
	 * Get current combat configuration
	 */
	UFUNCTION(BlueprintPure, Category = "Configuration")
	FCombatConfiguration GetCombatConfiguration() const { return CombatConfig; }

	/**
	 * Apply configuration to combat components
	 */
	UFUNCTION(BlueprintCallable, Category = "Configuration")
	void ApplyConfiguration();

	// ============================================================================
	// Statistics and Metrics
	// ============================================================================

	/**
	 * Get combat statistics
	 */
	UFUNCTION(BlueprintPure, Category = "Statistics")
	FCombatStats GetCombatStats() const { return CombatStats; }

	/**
	 * Reset combat statistics
	 */
	UFUNCTION(BlueprintCallable, Category = "Statistics")
	void ResetCombatStats();

	/**
	 * Update combat statistics
	 */
	void UpdateCombatStats(const FWeaponFireResult& FireResult);

	// ============================================================================
	// Component Access
	// ============================================================================

	/**
	 * Get targeting component
	 */
	UFUNCTION(BlueprintPure, Category = "Components")
	UOdysseyCombatTargetingComponent* GetTargetingComponent() const { return TargetingComponent; }

	/**
	 * Get weapon component
	 */
	UFUNCTION(BlueprintPure, Category = "Components")
	UOdysseyCombatWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }

	/**
	 * Get UI component
	 */
	UFUNCTION(BlueprintPure, Category = "Components")
	UOdysseyCombatUIComponent* GetUIComponent() const { return UIComponent; }

	// ============================================================================
	// Blueprint Events
	// ============================================================================

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Events")
	void OnCombatStateChanged(ECombatSystemState OldState, ECombatSystemState NewState);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Events")
	void OnTargetEngaged(AActor* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Events")
	void OnTargetDestroyed(AActor* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Events")
	void OnCombatStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Events")
	void OnCombatEnded();

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Events")
	void OnWeaponFired(AActor* Target, bool bHit);

protected:
	// ============================================================================
	// Configuration Properties
	// ============================================================================

	/**
	 * Combat system configuration
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Configuration")
	FCombatConfiguration CombatConfig;

	/**
	 * Whether to automatically initialize combat system on BeginPlay
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Configuration")
	bool bAutoInitialize;

	/**
	 * Whether to automatically register with touch interface
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Configuration")
	bool bAutoRegisterTouch;

	/**
	 * Whether to automatically register combat actions
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Configuration")
	bool bAutoRegisterActions;

	/**
	 * Combat update frequency for performance
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance", meta = (ClampMin = "0.016"))
	float CombatUpdateFrequency;

	// ============================================================================
	// Runtime State
	// ============================================================================

	/**
	 * Current combat system state
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Combat State")
	ECombatSystemState CurrentState;

	/**
	 * Time when combat started
	 */
	float CombatStartTime;

	/**
	 * Last combat update time
	 */
	float LastUpdateTime;

	/**
	 * Combat statistics
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Statistics")
	FCombatStats CombatStats;

	// ============================================================================
	// Component References
	// ============================================================================

	/**
	 * Targeting component
	 */
	UPROPERTY()
	UOdysseyCombatTargetingComponent* TargetingComponent;

	/**
	 * Weapon component
	 */
	UPROPERTY()
	UOdysseyCombatWeaponComponent* WeaponComponent;

	/**
	 * UI component
	 */
	UPROPERTY()
	UOdysseyCombatUIComponent* UIComponent;

	/**
	 * Touch interface reference
	 */
	UPROPERTY()
	UOdysseyTouchInterface* TouchInterface;

	/**
	 * Action button manager reference
	 */
	UPROPERTY()
	UOdysseyActionButtonManager* ActionButtonManager;

	// ============================================================================
	// Internal Methods
	// ============================================================================

	/**
	 * Find and initialize combat components
	 */
	void InitializeComponents();

	/**
	 * Update combat system logic
	 */
	void UpdateCombatSystem(float DeltaTime);

	/**
	 * Check for combat state transitions
	 */
	void UpdateCombatState();

	/**
	 * Handle component events
	 */
	void OnTargetChanged();
	void OnWeaponFiredInternal(const FWeaponFireResult& Result);
	void OnTargetDied(AActor* Target);

	/**
	 * Validate system integrity
	 */
	bool ValidateCombatSystem() const;

private:
	/**
	 * Internal state tracking
	 */
	bool bSystemInitialized;
	bool bTouchRegistered;
	bool bActionsRegistered;
	AActor* LastTarget;
};
