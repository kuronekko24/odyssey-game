// OdysseyCombatWeaponComponent.h
// Weapon management system for mobile space combat
// Handles automatic firing, weapon effects, and performance optimization

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "OdysseyActionEvent.h"
#include "OdysseyCombatWeaponComponent.generated.h"

// Forward declarations
class UOdysseyEventBus;
class UOdysseyCombatTargetingComponent;
class UNPCHealthComponent;
class UParticleSystemComponent;
class UNiagaraComponent;

/**
 * Weapon firing mode
 */
UENUM(BlueprintType)
enum class EWeaponFireMode : uint8
{
	Manual,         // Player must tap to fire
	Automatic,      // Auto-fire when target is in range and line of sight
	Burst,          // Fire in controlled bursts
	Charged         // Charge up, then release powerful shot
};

/**
 * Weapon state for UI and behavior
 */
UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	Ready,          // Ready to fire
	Firing,         // Currently firing
	Reloading,      // Reloading/cooling down
	Charging,       // Charging up for a shot
	OutOfAmmo,      // No ammo remaining
	Disabled        // Weapon is disabled
};

/**
 * Weapon type enumeration
 */
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	Laser,          // Instant hit energy weapon
	Plasma,         // Energy projectile
	Kinetic,        // Physical projectile
	Missile,        // Guided projectile
	Mining          // For mining operations
};

/**
 * Projectile configuration
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FProjectileConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float Speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float Lifetime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	bool bIsHoming;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float HomingStrength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float HomingRadius;

	FProjectileConfig()
	{
		Speed = 2000.0f;
		Lifetime = 5.0f;
		bIsHoming = false;
		HomingStrength = 1.0f;
		HomingRadius = 500.0f;
	}
};

/**
 * Weapon statistics
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FWeaponStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
	float Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
	float RateOfFire;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
	float Range;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
	float Accuracy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
	int32 EnergyCostPerShot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
	float CriticalChance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
	float CriticalMultiplier;

	FWeaponStats()
	{
		Damage = 25.0f;
		RateOfFire = 2.0f;  // Shots per second
		Range = 1500.0f;
		Accuracy = 0.95f;
		EnergyCostPerShot = 5;
		CriticalChance = 0.1f;
		CriticalMultiplier = 2.0f;
	}
};

/**
 * Visual effects configuration
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FWeaponEffectsConfig
{
	GENERATED_BODY()

	// Muzzle flash effect
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TSoftObjectPtr<UParticleSystem> MuzzleFlashEffect;

	// Projectile trail effect
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TSoftObjectPtr<UParticleSystem> ProjectileTrailEffect;

	// Impact effect
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TSoftObjectPtr<UParticleSystem> ImpactEffect;

	// Charge effect (for charged weapons)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TSoftObjectPtr<UParticleSystem> ChargeEffect;

	// Audio clips
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TSoftObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TSoftObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TSoftObjectPtr<USoundBase> ChargeSound;

	FWeaponEffectsConfig()
	{
	}
};

/**
 * Weapon fire result
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FWeaponFireResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Fire Result")
	bool bFireSuccessful;

	UPROPERTY(BlueprintReadOnly, Category = "Fire Result")
	bool bHitTarget;

	UPROPERTY(BlueprintReadOnly, Category = "Fire Result")
	float DamageDealt;

	UPROPERTY(BlueprintReadOnly, Category = "Fire Result")
	bool bWasCritical;

	UPROPERTY(BlueprintReadOnly, Category = "Fire Result")
	AActor* HitActor;

	UPROPERTY(BlueprintReadOnly, Category = "Fire Result")
	FVector HitLocation;

	UPROPERTY(BlueprintReadOnly, Category = "Fire Result")
	FName FailureReason;

	FWeaponFireResult()
	{
		bFireSuccessful = false;
		bHitTarget = false;
		DamageDealt = 0.0f;
		bWasCritical = false;
		HitActor = nullptr;
		HitLocation = FVector::ZeroVector;
		FailureReason = NAME_None;
	}
};

/**
 * Weapon event payload for combat system integration
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FWeaponEventPayload : public FCombatEventPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Weapon Event")
	EWeaponType WeaponType;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon Event")
	EWeaponState WeaponState;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon Event")
	int32 WeaponId;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon Event")
	float ChargeLevel;

	FWeaponEventPayload()
	{
		WeaponType = EWeaponType::Laser;
		WeaponState = EWeaponState::Ready;
		WeaponId = 0;
		ChargeLevel = 0.0f;
	}
};

/**
 * Combat Weapon Component
 * 
 * Features:
 * - Automatic firing when target is in range (mobile-optimized)
 * - Multiple weapon types with configurable stats
 * - Visual and audio effects with performance optimization
 * - Energy-based ammunition system
 * - Critical hit system
 * - Event-driven architecture for combat integration
 * - Mobile performance optimization with effect pooling
 */
UCLASS(ClassGroup=(Odyssey), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UOdysseyCombatWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdysseyCombatWeaponComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// Weapon Firing Interface
	// ============================================================================

	/**
	 * Attempt to fire the weapon at current target
	 * @return Fire result with hit information
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Firing")
	FWeaponFireResult FireWeapon();

	/**
	 * Fire at a specific target
	 * @param Target Target actor to fire at
	 * @return Fire result with hit information
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Firing")
	FWeaponFireResult FireAtTarget(AActor* Target);

	/**
	 * Fire at a specific location
	 * @param TargetLocation World location to fire at
	 * @return Fire result with hit information
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Firing")
	FWeaponFireResult FireAtLocation(FVector TargetLocation);

	/**
	 * Start charging a charged weapon
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Firing")
	bool StartCharging();

	/**
	 * Stop charging and fire charged shot
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Firing")
	FWeaponFireResult ReleaseChargedShot();

	/**
	 * Cancel charging
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Firing")
	void CancelCharging();

	// ============================================================================
	// Automatic Firing System
	// ============================================================================

	/**
	 * Enable or disable automatic firing
	 */
	UFUNCTION(BlueprintCallable, Category = "Auto Firing")
	void SetAutoFireEnabled(bool bEnabled);

	/**
	 * Check if auto-firing is enabled
	 */
	UFUNCTION(BlueprintPure, Category = "Auto Firing")
	bool IsAutoFireEnabled() const { return bAutoFireEnabled; }

	/**
	 * Update automatic firing logic (called internally)
	 */
	void UpdateAutoFiring(float DeltaTime);

	/**
	 * Check if conditions are met for automatic firing
	 */
	UFUNCTION(BlueprintCallable, Category = "Auto Firing")
	bool CanAutoFire() const;

	// ============================================================================
	// Weapon State Management
	// ============================================================================

	/**
	 * Get current weapon state
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon State")
	EWeaponState GetWeaponState() const { return CurrentState; }

	/**
	 * Check if weapon can fire
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon State")
	bool CanFire() const;

	/**
	 * Get reload progress (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon State")
	float GetReloadProgress() const;

	/**
	 * Get charge level (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon State")
	float GetChargeLevel() const { return CurrentChargeLevel; }

	/**
	 * Get time until next shot can be fired
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon State")
	float GetTimeUntilNextShot() const;

	// ============================================================================
	// Weapon Configuration
	// ============================================================================

	/**
	 * Set weapon type and update configuration
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Config")
	void SetWeaponType(EWeaponType NewType);

	/**
	 * Set firing mode
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Config")
	void SetFireMode(EWeaponFireMode NewMode);

	/**
	 * Set weapon statistics
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Config")
	void SetWeaponStats(const FWeaponStats& NewStats);

	/**
	 * Get current weapon statistics
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon Config")
	FWeaponStats GetWeaponStats() const { return WeaponStats; }

	/**
	 * Set weapon enabled/disabled
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Config")
	void SetWeaponEnabled(bool bEnabled);

	// ============================================================================
	// Targeting Integration
	// ============================================================================

	/**
	 * Set the targeting component to use
	 */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	void SetTargetingComponent(UOdysseyCombatTargetingComponent* TargetingComp);

	/**
	 * Get current target from targeting component
	 */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	AActor* GetCurrentTarget() const;

	/**
	 * Check if target is in weapon range
	 */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	bool IsTargetInRange(AActor* Target) const;

	// ============================================================================
	// Effects and Feedback
	// ============================================================================

	/**
	 * Play muzzle flash effect
	 */
	UFUNCTION(BlueprintCallable, Category = "Effects")
	void PlayMuzzleFlash();

	/**
	 * Spawn projectile trail effect
	 */
	UFUNCTION(BlueprintCallable, Category = "Effects")
	void SpawnProjectileTrail(FVector Start, FVector End);

	/**
	 * Play impact effect at location
	 */
	UFUNCTION(BlueprintCallable, Category = "Effects")
	void PlayImpactEffect(FVector Location, AActor* HitActor = nullptr);

	/**
	 * Play weapon firing sound
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlayFireSound();

	// ============================================================================
	// Energy Integration
	// ============================================================================

	/**
	 * Check if enough energy for a shot
	 */
	UFUNCTION(BlueprintCallable, Category = "Energy")
	bool HasEnoughEnergy() const;

	/**
	 * Consume energy for a shot
	 */
	UFUNCTION(BlueprintCallable, Category = "Energy")
	bool ConsumeEnergyForShot();

	// ============================================================================
	// Event System Integration
	// ============================================================================

	/**
	 * Get the event bus
	 */
	UFUNCTION(BlueprintCallable, Category = "Event System")
	UOdysseyEventBus* GetEventBus();

	// ============================================================================
	// Blueprint Events
	// ============================================================================

	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon Events")
	void OnWeaponFired(AActor* Target, bool bHit, float Damage);

	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon Events")
	void OnWeaponStateChanged(EWeaponState OldState, EWeaponState NewState);

	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon Events")
	void OnWeaponChargeStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon Events")
	void OnWeaponChargeCompleted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon Events")
	void OnTargetHit(AActor* Target, float Damage, bool bWasCritical);

	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon Events")
	void OnAutoFireStateChanged(bool bEnabled);

protected:
	// ============================================================================
	// Configuration Properties
	// ============================================================================

	/**
	 * Weapon type
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Config")
	EWeaponType WeaponType;

	/**
	 * Firing mode
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Config")
	EWeaponFireMode FireMode;

	/**
	 * Weapon statistics
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Config")
	FWeaponStats WeaponStats;

	/**
	 * Projectile configuration (for projectile weapons)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Config")
	FProjectileConfig ProjectileConfig;

	/**
	 * Visual and audio effects
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Config")
	FWeaponEffectsConfig EffectsConfig;

	/**
	 * Whether weapon is enabled
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Config")
	bool bWeaponEnabled;

	/**
	 * Whether automatic firing is enabled
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Firing")
	bool bAutoFireEnabled;

	/**
	 * Auto-fire update frequency (for performance)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Firing", meta = (ClampMin = "0.1"))
	float AutoFireUpdateFrequency;

	/**
	 * Unique weapon ID (for multi-weapon systems)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Config")
	int32 WeaponId;

	/**
	 * Weapon mount point (relative to owner)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Config")
	FVector MountOffset;

	/**
	 * Whether to broadcast weapon events
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event System")
	bool bBroadcastWeaponEvents;

	// ============================================================================
	// Runtime State
	// ============================================================================

	/**
	 * Current weapon state
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon State")
	EWeaponState CurrentState;

	/**
	 * Time of last shot
	 */
	float LastFireTime;

	/**
	 * Current charge level (0.0 to 1.0)
	 */
	float CurrentChargeLevel;

	/**
	 * Time when charging started
	 */
	float ChargeStartTime;

	/**
	 * Last auto-fire update time
	 */
	float LastAutoFireUpdateTime;

	/**
	 * Cached targeting component
	 */
	UPROPERTY()
	UOdysseyCombatTargetingComponent* TargetingComponent;

	/**
	 * Event bus reference
	 */
	UPROPERTY()
	UOdysseyEventBus* EventBus;

	/**
	 * Event subscription handles
	 */
	TArray<FOdysseyEventHandle> EventHandles;

	/**
	 * Cached audio component for weapon sounds
	 */
	UPROPERTY()
	UAudioComponent* AudioComponent;

	// ============================================================================
	// Internal Methods
	// ============================================================================

	/**
	 * Initialize weapon system
	 */
	void InitializeWeapon();

	/**
	 * Shutdown weapon system
	 */
	void ShutdownWeapon();

	/**
	 * Update weapon state (reloading, charging, etc.)
	 */
	void UpdateWeaponState(float DeltaTime);

	/**
	 * Calculate fire direction to target
	 */
	FVector CalculateFireDirection(AActor* Target) const;

	/**
	 * Calculate fire direction to location
	 */
	FVector CalculateFireDirection(FVector TargetLocation) const;

	/**
	 * Perform hit scan for instant-hit weapons
	 */
	bool PerformHitScan(FVector StartLocation, FVector Direction, float MaxRange, FHitResult& HitResult);

	/**
	 * Calculate damage including critical hits
	 */
	float CalculateDamage(bool& bIsCritical);

	/**
	 * Apply damage to target
	 */
	bool ApplyDamageToTarget(AActor* Target, float Damage, bool bIsCritical);

	/**
	 * Change weapon state and broadcast events
	 */
	void ChangeWeaponState(EWeaponState NewState);

	/**
	 * Get weapon mount world location
	 */
	FVector GetMountWorldLocation() const;

	/**
	 * Get weapon mount world rotation
	 */
	FRotator GetMountWorldRotation() const;

	/**
	 * Broadcast weapon event
	 */
	void BroadcastWeaponEvent(EOdysseyEventType EventType, AActor* Target = nullptr);

	/**
	 * Subscribe to relevant events
	 */
	void InitializeEventSubscriptions();

	/**
	 * Clean up event subscriptions
	 */
	void CleanupEventSubscriptions();

	/**
	 * Handle target change event
	 */
	void OnTargetChangedEvent(const FOdysseyEventPayload& Payload);

	/**
	 * Handle energy change event
	 */
	void OnEnergyChangedEvent(const FOdysseyEventPayload& Payload);

private:
	/**
	 * Internal fire implementation
	 */
	FWeaponFireResult FireInternal(AActor* Target, FVector TargetLocation);

	/**
	 * Validate fire conditions
	 */
	bool ValidateFireConditions(FName& FailureReason);
};
