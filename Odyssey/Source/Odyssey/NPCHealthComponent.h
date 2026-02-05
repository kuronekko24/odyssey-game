// NPCHealthComponent.h
// Health management component for NPC ships with event integration
// Handles health tracking, regeneration, and visual feedback for ship combat

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OdysseyActionEvent.h"
#include "NPCHealthComponent.generated.h"

class UOdysseyEventBus;

/**
 * Health state for different behaviors and visual feedback
 */
UENUM(BlueprintType)
enum class EHealthState : uint8
{
	Healthy = 0,		// 100% to 75% health
	Damaged = 1,		// 74% to 50% health
	Critical = 2,		// 49% to 25% health
	Dying = 3,			// 24% to 1% health
	Dead = 4			// 0% health
};

/**
 * Health event data for broadcasting health changes
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
	float DamageAmount;

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

	FHealthEventPayload()
		: PreviousHealth(0.0f)
		, CurrentHealth(0.0f)
		, MaxHealth(100.0f)
		, DamageAmount(0.0f)
		, PreviousState(EHealthState::Healthy)
		, CurrentState(EHealthState::Healthy)
		, DamageType(NAME_None)
		, bWasKillingBlow(false)
	{
	}

	float GetHealthPercentage() const
	{
		return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
	}
};

/**
 * Delegate declarations for Blueprint and C++ integration
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHealthChangedDelegate, const FHealthEventPayload&, HealthData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHealthStateChangedDelegate, EHealthState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FActorDiedDelegate, AActor*, DiedActor);

/**
 * Health component for NPC ships in the Odyssey combat system
 * 
 * Features:
 * - Event-driven damage processing with integration to OdysseyEventBus
 * - Health state management for visual feedback
 * - Optional health regeneration with configurable rates
 * - Mobile-optimized with minimal allocations during combat
 * - Blueprint-friendly interface for designers
 * - Support for different damage types and resistances
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
	 * Apply damage to this actor
	 * @param DamageAmount Amount of damage to apply
	 * @param DamageSource Actor that caused the damage
	 * @param DamageType Type identifier for the damage
	 * @return Actual damage applied (after resistances, etc.)
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	float TakeDamage(float DamageAmount, AActor* DamageSource = nullptr, FName DamageType = NAME_None);

	/**
	 * Heal this actor
	 * @param HealAmount Amount to heal
	 * @param HealSource Actor providing the healing (optional)
	 * @return Actual health restored
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	float Heal(float HealAmount, AActor* HealSource = nullptr);

	/**
	 * Set health to a specific value
	 * @param NewHealth New health value (will be clamped to 0-MaxHealth)
	 * @param bBroadcastEvent Whether to broadcast health change event
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetHealth(float NewHealth, bool bBroadcastEvent = true);

	/**
	 * Set maximum health and optionally adjust current health proportionally
	 * @param NewMaxHealth New maximum health value
	 * @param bMaintainHealthPercentage If true, current health % stays the same
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetMaxHealth(float NewMaxHealth, bool bMaintainHealthPercentage = false);

	/**
	 * Kill this actor immediately
	 * @param KillerActor Actor responsible for the kill (optional)
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Kill(AActor* KillerActor = nullptr);

	// ============================================================================
	// Health State and Queries
	// ============================================================================

	/**
	 * Get current health value
	 */
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	/**
	 * Get maximum health value
	 */
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	/**
	 * Get health as percentage (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthPercentage() const;

	/**
	 * Get current health state
	 */
	UFUNCTION(BlueprintPure, Category = "Health")
	EHealthState GetHealthState() const { return CurrentHealthState; }

	/**
	 * Check if actor is dead
	 */
	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return CurrentHealthState == EHealthState::Dead; }

	/**
	 * Check if actor is at full health
	 */
	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsAtFullHealth() const { return FMath::IsNearlyEqual(CurrentHealth, MaxHealth, 0.1f); }

	/**
	 * Check if actor can be healed (not dead and not at full health)
	 */
	UFUNCTION(BlueprintPure, Category = "Health")
	bool CanBeHealed() const { return !IsDead() && !IsAtFullHealth(); }

	// ============================================================================
	// Configuration
	// ============================================================================

	/**
	 * Enable or disable health regeneration
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetHealthRegenEnabled(bool bEnabled);

	/**
	 * Set health regeneration rate (health per second)
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetHealthRegenRate(float RegenPerSecond);

	/**
	 * Set damage resistance for a specific damage type
	 * @param DamageType Type of damage
	 * @param ResistancePercentage 0.0 = no resistance, 1.0 = complete immunity
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetDamageResistance(FName DamageType, float ResistancePercentage);

	/**
	 * Get damage resistance for a specific damage type
	 */
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetDamageResistance(FName DamageType) const;

	// ============================================================================
	// Events and Delegates
	// ============================================================================

	/**
	 * Called when health changes (damage or healing)
	 */
	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FHealthChangedDelegate OnHealthChanged;

	/**
	 * Called when health state changes (e.g., Healthy to Damaged)
	 */
	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FHealthStateChangedDelegate OnHealthStateChanged;

	/**
	 * Called when actor dies
	 */
	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FActorDiedDelegate OnActorDied;

protected:
	// ============================================================================
	// Configuration Properties
	// ============================================================================

	/**
	 * Maximum health value
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = "1.0"))
	float MaxHealth;

	/**
	 * Starting health as percentage of MaxHealth (0.0 to 1.0)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StartingHealthPercentage;

	/**
	 * Whether health can regenerate over time
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Regeneration")
	bool bHealthRegenEnabled;

	/**
	 * Health regeneration rate (health points per second)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Regeneration", meta = (ClampMin = "0.0"))
	float HealthRegenRate;

	/**
	 * Delay before health regeneration starts after taking damage (seconds)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Regeneration", meta = (ClampMin = "0.0"))
	float HealthRegenDelay;

	/**
	 * Whether regeneration only works when not in combat
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Regeneration")
	bool bOnlyRegenOutOfCombat;

	/**
	 * Time to consider actor "out of combat" after last damage (seconds)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Regeneration", meta = (ClampMin = "0.0"))
	float OutOfCombatTime;

	/**
	 * Damage resistances by type (0.0 = no resistance, 1.0 = immunity)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Resistances")
	TMap<FName, float> DamageResistances;

	/**
	 * Whether to broadcast health events to the global event bus
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health|Events")
	bool bBroadcastHealthEvents;

	/**
	 * Whether this actor can die (if false, health will be clamped to 1)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	bool bCanDie;

	// ============================================================================
	// Runtime State
	// ============================================================================

	/**
	 * Current health value
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Health")
	float CurrentHealth;

	/**
	 * Current health state
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Health")
	EHealthState CurrentHealthState;

	/**
	 * Time since last damage received (for regen delay and out-of-combat logic)
	 */
	float TimeSinceLastDamage;

	/**
	 * Whether health regeneration is currently active
	 */
	bool bIsRegenerating;

	/**
	 * Reference to the global event bus for broadcasting events
	 */
	UPROPERTY()
	UOdysseyEventBus* EventBus;

	// ============================================================================
	// Internal Methods
	// ============================================================================

	/**
	 * Update health state based on current health percentage
	 */
	void UpdateHealthState();

	/**
	 * Calculate actual damage after applying resistances
	 */
	float CalculateActualDamage(float BaseDamage, FName DamageType) const;

	/**
	 * Broadcast health change event
	 */
	void BroadcastHealthChangeEvent(float PreviouHealth, float DamageAmount, AActor* Source, FName DamageType, EHealthState PreviousState);

	/**
	 * Handle actor death
	 */
	void HandleDeath(AActor* KillerActor);

	/**
	 * Process health regeneration
	 */
	void ProcessHealthRegeneration(float DeltaTime);

	/**
	 * Check if actor is currently in combat
	 */
	bool IsInCombat() const;

	// ============================================================================
	// Event Bus Integration
	// ============================================================================

	/**
	 * Initialize event bus subscriptions
	 */
	void InitializeEventBusSubscriptions();

	/**
	 * Clean up event bus subscriptions
	 */
	void CleanupEventBusSubscriptions();

	/**
	 * Handle incoming damage events from the event bus
	 */
	void OnDamageReceived(const FOdysseyEventPayload& Payload);

	/**
	 * Event subscription handle for cleanup
	 */
	FOdysseyEventHandle DamageSubscriptionHandle;
};
