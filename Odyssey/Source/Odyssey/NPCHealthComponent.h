// NPCHealthComponent.h
// Component-based health tracking with event integration for NPC ships
// Supports shields, damage resistances, regeneration, and visual health feedback
// Phase 1: Health & Damage Foundation

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OdysseyActionEvent.h"
#include "NPCHealthComponent.generated.h"

class UOdysseyEventBus;
class UWidgetComponent;

// ============================================================================
// Enumerations
// ============================================================================

/**
 * Health state tiers for behavior and visual feedback
 * Thresholds are based on combined health+shield percentage
 */
UENUM(BlueprintType)
enum class EHealthState : uint8
{
	Healthy   = 0,  // 75-100%
	Damaged   = 1,  // 50-74%
	Critical  = 2,  // 25-49%
	Dying     = 3,  // 1-24%
	Dead      = 4   // 0%
};

/**
 * Damage type categories for resistance calculations
 */
UENUM(BlueprintType)
enum class EDamageCategory : uint8
{
	Kinetic   = 0,  // Physical projectile damage
	Energy    = 1,  // Laser / energy weapon damage
	Plasma    = 2,  // Plasma weapon damage
	Explosive = 3,  // Area-of-effect explosive damage
	Collision = 4,  // Environmental / ram damage
	True      = 5   // Bypasses all resistances
};

// ============================================================================
// Event Payload Structures
// ============================================================================

/**
 * Payload broadcast when health or shields change
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FHealthEventPayload : public FOdysseyEventPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Health Event")
	float PreviousHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Health Event")
	float CurrentHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Health Event")
	float MaxHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Health Event")
	float PreviousShields;

	UPROPERTY(BlueprintReadOnly, Category = "Health Event")
	float CurrentShields;

	UPROPERTY(BlueprintReadOnly, Category = "Health Event")
	float MaxShields;

	UPROPERTY(BlueprintReadOnly, Category = "Health Event")
	float DamageAmount;

	UPROPERTY(BlueprintReadOnly, Category = "Health Event")
	float ShieldDamageAbsorbed;

	UPROPERTY(BlueprintReadOnly, Category = "Health Event")
	EHealthState PreviousState;

	UPROPERTY(BlueprintReadOnly, Category = "Health Event")
	EHealthState CurrentState;

	UPROPERTY(BlueprintReadOnly, Category = "Health Event")
	TWeakObjectPtr<AActor> DamageSource;

	UPROPERTY(BlueprintReadOnly, Category = "Health Event")
	FName DamageType;

	UPROPERTY(BlueprintReadOnly, Category = "Health Event")
	bool bWasKillingBlow;

	UPROPERTY(BlueprintReadOnly, Category = "Health Event")
	bool bWasCritical;

	FHealthEventPayload()
		: PreviousHealth(0.0f)
		, CurrentHealth(0.0f)
		, MaxHealth(100.0f)
		, PreviousShields(0.0f)
		, CurrentShields(0.0f)
		, MaxShields(0.0f)
		, DamageAmount(0.0f)
		, ShieldDamageAbsorbed(0.0f)
		, PreviousState(EHealthState::Healthy)
		, CurrentState(EHealthState::Healthy)
		, DamageType(NAME_None)
		, bWasKillingBlow(false)
		, bWasCritical(false)
	{
	}

	/** Get combined health + shield percentage (0.0 to 1.0) */
	float GetEffectiveHealthPercentage() const
	{
		float MaxEffective = MaxHealth + MaxShields;
		return MaxEffective > 0.0f ? (CurrentHealth + CurrentShields) / MaxEffective : 0.0f;
	}

	/** Get hull health percentage only (0.0 to 1.0) */
	float GetHealthPercentage() const
	{
		return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
	}

	/** Get shield percentage only (0.0 to 1.0) */
	float GetShieldPercentage() const
	{
		return MaxShields > 0.0f ? CurrentShields / MaxShields : 0.0f;
	}
};

/**
 * Pending damage-over-time effect
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FDamageOverTimeEffect
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "DOT")
	float DamagePerTick;

	UPROPERTY(BlueprintReadOnly, Category = "DOT")
	float TickInterval;

	UPROPERTY(BlueprintReadOnly, Category = "DOT")
	float RemainingDuration;

	UPROPERTY(BlueprintReadOnly, Category = "DOT")
	FName DamageType;

	UPROPERTY(BlueprintReadOnly, Category = "DOT")
	TWeakObjectPtr<AActor> Source;

	/** Internal: time accumulator for tick scheduling */
	float TickAccumulator;

	FDamageOverTimeEffect()
		: DamagePerTick(0.0f)
		, TickInterval(1.0f)
		, RemainingDuration(0.0f)
		, DamageType(NAME_None)
		, TickAccumulator(0.0f)
	{
	}
};

// ============================================================================
// Delegate Declarations
// ============================================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, const FHealthEventPayload&, HealthData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthStateChanged, EHealthState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorDied, AActor*, DiedActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShieldBroken, AActor*, Owner, AActor*, DamageSource);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShieldRestored, AActor*, Owner, float, ShieldAmount);

// ============================================================================
// UNPCHealthComponent
// ============================================================================

/**
 * Health component for NPC ships in the Odyssey combat system.
 *
 * Features:
 * - Dual-layer defense: shields absorb damage before hull health
 * - Per-type damage resistances (Kinetic, Energy, Plasma, etc.)
 * - Configurable health and shield regeneration with combat-awareness
 * - Damage-over-time effect tracking
 * - Event-driven via OdysseyEventBus and local delegates
 * - Mobile-optimized: reduced tick rate, minimal allocations
 * - Visual health bar support via optional WidgetComponent
 */
UCLASS(ClassGroup=(Odyssey), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UNPCHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPCHealthComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// Core Health Management
	// ============================================================================

	/**
	 * Apply damage to this actor. Shields absorb damage first.
	 * @param DamageAmount Raw damage before resistances
	 * @param DamageSource Actor that caused the damage
	 * @param DamageType Damage type name for resistance lookup
	 * @return Actual damage applied to hull (after shields and resistances)
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	float TakeDamage(float DamageAmount, AActor* DamageSource = nullptr, FName DamageType = NAME_None);

	/**
	 * Apply damage with critical hit flag
	 * @param DamageAmount Raw damage before resistances
	 * @param DamageSource Actor that caused the damage
	 * @param DamageType Damage type name for resistance lookup
	 * @param bIsCritical Whether this was a critical hit
	 * @return Actual damage applied to hull
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	float TakeDamageEx(float DamageAmount, AActor* DamageSource, FName DamageType, bool bIsCritical);

	/**
	 * Heal this actor's hull health
	 * @param HealAmount Amount to heal
	 * @param HealSource Actor providing the healing (optional)
	 * @return Actual health restored
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	float Heal(float HealAmount, AActor* HealSource = nullptr);

	/**
	 * Restore shield points
	 * @param ShieldAmount Amount of shields to restore
	 * @param Source Actor providing the restore (optional)
	 * @return Actual shields restored
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	float RestoreShields(float ShieldAmount, AActor* Source = nullptr);

	/**
	 * Set health to a specific value (clamped to 0..MaxHealth)
	 * @param NewHealth New health value
	 * @param bBroadcastEvent Whether to broadcast health change event
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetHealth(float NewHealth, bool bBroadcastEvent = true);

	/**
	 * Set shields to a specific value (clamped to 0..MaxShields)
	 * @param NewShields New shield value
	 * @param bBroadcastEvent Whether to broadcast change event
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetShields(float NewShields, bool bBroadcastEvent = true);

	/**
	 * Set maximum health, optionally preserving health percentage
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetMaxHealth(float NewMaxHealth, bool bMaintainHealthPercentage = false);

	/**
	 * Set maximum shields, optionally preserving shield percentage
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetMaxShields(float NewMaxShields, bool bMaintainShieldPercentage = false);

	/**
	 * Kill this actor immediately
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Kill(AActor* KillerActor = nullptr);

	// ============================================================================
	// Damage-Over-Time
	// ============================================================================

	/**
	 * Apply a damage-over-time effect
	 * @param DamagePerTick Damage each tick
	 * @param TickInterval Seconds between ticks
	 * @param Duration Total effect duration
	 * @param DamageType Type name for resistance lookup
	 * @param Source Actor responsible
	 */
	UFUNCTION(BlueprintCallable, Category = "Health|DOT")
	void ApplyDamageOverTime(float DamagePerTick, float TickInterval, float Duration, FName DamageType = NAME_None, AActor* Source = nullptr);

	/**
	 * Remove all damage-over-time effects
	 */
	UFUNCTION(BlueprintCallable, Category = "Health|DOT")
	void ClearAllDamageOverTime();

	/**
	 * Get number of active DOT effects
	 */
	UFUNCTION(BlueprintPure, Category = "Health|DOT")
	int32 GetActiveDOTCount() const { return ActiveDOTEffects.Num(); }

	// ============================================================================
	// Health Queries
	// ============================================================================

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthPercentage() const;

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetCurrentShields() const { return CurrentShields; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxShields() const { return MaxShields; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetShieldPercentage() const;

	/** Combined health+shield percentage for overall survivability */
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetEffectiveHealthPercentage() const;

	UFUNCTION(BlueprintPure, Category = "Health")
	EHealthState GetHealthState() const { return CurrentHealthState; }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return CurrentHealthState == EHealthState::Dead; }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsAtFullHealth() const;

	UFUNCTION(BlueprintPure, Category = "Health")
	bool HasShields() const { return MaxShields > 0.0f && CurrentShields > 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool CanBeHealed() const { return !IsDead() && !IsAtFullHealth(); }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsInCombat() const;

	// ============================================================================
	// Resistance Configuration
	// ============================================================================

	/**
	 * Set damage resistance for a specific damage type
	 * @param DamageType Type of damage (use FName like "Kinetic", "Energy", etc.)
	 * @param ResistancePercentage 0.0 = no resistance, 1.0 = complete immunity
	 */
	UFUNCTION(BlueprintCallable, Category = "Health|Resistances")
	void SetDamageResistance(FName DamageType, float ResistancePercentage);

	UFUNCTION(BlueprintPure, Category = "Health|Resistances")
	float GetDamageResistance(FName DamageType) const;

	/** Set a flat damage reduction amount (applied after resistance percentage) */
	UFUNCTION(BlueprintCallable, Category = "Health|Resistances")
	void SetFlatDamageReduction(float ReductionAmount);

	UFUNCTION(BlueprintPure, Category = "Health|Resistances")
	float GetFlatDamageReduction() const { return FlatDamageReduction; }

	// ============================================================================
	// Regeneration Configuration
	// ============================================================================

	UFUNCTION(BlueprintCallable, Category = "Health|Regeneration")
	void SetHealthRegenEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Health|Regeneration")
	void SetHealthRegenRate(float RegenPerSecond);

	UFUNCTION(BlueprintCallable, Category = "Health|Regeneration")
	void SetShieldRegenEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Health|Regeneration")
	void SetShieldRegenRate(float RegenPerSecond);

	// ============================================================================
	// Visual Health Bar
	// ============================================================================

	/**
	 * Get the health bar color based on current state
	 * @return Interpolated color from green (healthy) to red (dying)
	 */
	UFUNCTION(BlueprintPure, Category = "Health|Visual")
	FLinearColor GetHealthBarColor() const;

	/**
	 * Get the shield bar color
	 */
	UFUNCTION(BlueprintPure, Category = "Health|Visual")
	FLinearColor GetShieldBarColor() const;

	/**
	 * Whether the health bar should be visible (damaged or recently damaged)
	 */
	UFUNCTION(BlueprintPure, Category = "Health|Visual")
	bool ShouldShowHealthBar() const;

	// ============================================================================
	// Events and Delegates
	// ============================================================================

	/** Called when health or shields change */
	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnHealthChanged OnHealthChanged;

	/** Called when health state tier changes */
	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnHealthStateChanged OnHealthStateChanged;

	/** Called when actor dies */
	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnActorDied OnActorDied;

	/** Called when shields are fully depleted by damage */
	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnShieldBroken OnShieldBroken;

	/** Called when shields finish regenerating to full */
	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnShieldRestored OnShieldRestored;

protected:
	// ============================================================================
	// Configuration Properties (Editable in Details Panel)
	// ============================================================================

	/** Maximum hull health */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Hull", meta = (ClampMin = "1.0"))
	float MaxHealth;

	/** Starting health as percentage of MaxHealth */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Hull", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StartingHealthPercentage;

	/** Maximum shield capacity (0 = no shields) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Shields", meta = (ClampMin = "0.0"))
	float MaxShields;

	/** Starting shields as percentage of MaxShields */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Shields", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StartingShieldPercentage;

	/** Percentage of excess shield damage that bleeds through to hull (0.0 = none, 1.0 = all) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Shields", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ShieldBleedThroughRatio;

	// --- Health Regeneration ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Regeneration")
	bool bHealthRegenEnabled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Regeneration", meta = (ClampMin = "0.0"))
	float HealthRegenRate;

	/** Delay in seconds after taking damage before health regen starts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Regeneration", meta = (ClampMin = "0.0"))
	float HealthRegenDelay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Regeneration")
	bool bOnlyRegenOutOfCombat;

	/** Seconds since last damage to be considered "out of combat" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Regeneration", meta = (ClampMin = "0.0"))
	float OutOfCombatTime;

	// --- Shield Regeneration ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Shields")
	bool bShieldRegenEnabled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Shields", meta = (ClampMin = "0.0"))
	float ShieldRegenRate;

	/** Delay after shield damage before regen starts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Shields", meta = (ClampMin = "0.0"))
	float ShieldRegenDelay;

	// --- Resistances ---

	/** Damage resistances by type name: 0.0 = no resistance, 1.0 = immunity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Resistances")
	TMap<FName, float> DamageResistances;

	/** Flat damage reduction applied after percentage resistance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Resistances", meta = (ClampMin = "0.0"))
	float FlatDamageReduction;

	// --- Death & Events ---

	/** Whether this actor can actually die (if false, health clamps to 1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	bool bCanDie;

	/** Whether to publish health events to the global OdysseyEventBus */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Events")
	bool bBroadcastToEventBus;

	// --- Visual ---

	/** How long the health bar stays visible after last damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Visual", meta = (ClampMin = "0.0"))
	float HealthBarVisibilityDuration;

	/** Whether to only show the health bar when damaged */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Visual")
	bool bOnlyShowHealthBarWhenDamaged;

	// ============================================================================
	// Runtime State
	// ============================================================================

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	float CurrentHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	float CurrentShields;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	EHealthState CurrentHealthState;

	/** Seconds since hull was last damaged */
	float TimeSinceLastDamage;

	/** Seconds since shields were last damaged */
	float TimeSinceLastShieldDamage;

	/** Whether health is currently regenerating */
	bool bIsHealthRegenerating;

	/** Whether shields are currently regenerating */
	bool bIsShieldRegenerating;

	/** Whether shields were full before last damage */
	bool bShieldsWereFull;

	/** Active damage-over-time effects */
	UPROPERTY()
	TArray<FDamageOverTimeEffect> ActiveDOTEffects;

	/** Reference to the global event bus */
	UPROPERTY()
	UOdysseyEventBus* EventBus;

	/** Event subscription handle for incoming damage events */
	FOdysseyEventHandle DamageSubscriptionHandle;

	// ============================================================================
	// Internal Methods
	// ============================================================================

	/** Calculate post-resistance, post-reduction damage */
	float CalculateActualDamage(float BaseDamage, FName DamageType) const;

	/** Route damage through shields first, then hull. Returns hull damage dealt. */
	float ApplyDamageToShieldsAndHealth(float ProcessedDamage, AActor* Source, FName DamageType, bool bIsCritical);

	/** Update health state tier based on effective health percentage */
	void UpdateHealthState();

	/** Broadcast FHealthEventPayload to delegates and event bus */
	void BroadcastHealthChangeEvent(float PrevHealth, float PrevShields, float DamageAmount, float ShieldAbsorbed, AActor* Source, FName DamageType, EHealthState PrevState, bool bIsCritical);

	/** Handle death logic */
	void HandleDeath(AActor* KillerActor);

	/** Process health regeneration per tick */
	void ProcessHealthRegeneration(float DeltaTime);

	/** Process shield regeneration per tick */
	void ProcessShieldRegeneration(float DeltaTime);

	/** Process active DOT effects per tick */
	void ProcessDamageOverTime(float DeltaTime);

	// --- Event Bus Integration ---

	void InitializeEventBusSubscriptions();
	void CleanupEventBusSubscriptions();
	void OnDamageEventReceived(const FOdysseyEventPayload& Payload);
};
