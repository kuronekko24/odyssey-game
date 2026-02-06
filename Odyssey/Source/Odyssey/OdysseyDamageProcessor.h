// OdysseyDamageProcessor.h
// Central damage processing system that bridges combat events to actual damage application
// Handles damage calculation, validation, and routing to health components
// Phase 1: Health & Damage Foundation

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "OdysseyActionEvent.h"
#include "NPCHealthComponent.h"
#include "OdysseyDamageProcessor.generated.h"

class UOdysseyEventBus;

// ============================================================================
// Damage Calculation Structures
// ============================================================================

/**
 * Input parameters for a damage calculation pass
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FDamageCalculationParams
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Damage")
	float BaseDamage;

	UPROPERTY(BlueprintReadWrite, Category = "Damage")
	FName DamageType;

	UPROPERTY(BlueprintReadWrite, Category = "Damage")
	TWeakObjectPtr<AActor> Attacker;

	UPROPERTY(BlueprintReadWrite, Category = "Damage")
	TWeakObjectPtr<AActor> Target;

	UPROPERTY(BlueprintReadWrite, Category = "Damage")
	FVector HitLocation;

	/** Per-attack critical chance override (-1 = use global) */
	UPROPERTY(BlueprintReadWrite, Category = "Damage")
	float CriticalChance;

	/** Per-attack critical multiplier override (-1 = use global) */
	UPROPERTY(BlueprintReadWrite, Category = "Damage")
	float CriticalMultiplier;

	/** Named multipliers stacked onto the base damage */
	UPROPERTY(BlueprintReadWrite, Category = "Damage")
	TMap<FName, float> DamageModifiers;

	/** Distance between attacker and target (populated by processor if <= 0) */
	UPROPERTY(BlueprintReadWrite, Category = "Damage")
	float Distance;

	FDamageCalculationParams()
		: BaseDamage(0.0f)
		, DamageType(NAME_None)
		, HitLocation(FVector::ZeroVector)
		, CriticalChance(-1.0f)
		, CriticalMultiplier(-1.0f)
		, Distance(-1.0f)
	{
	}
};

/**
 * Output of a damage calculation
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FDamageCalculationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	float FinalDamage;

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	bool bIsCritical;

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	bool bWasBlocked;

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	float DamageMultiplier;

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	float DistanceFalloff;

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	FString CalculationDetails;

	FDamageCalculationResult()
		: FinalDamage(0.0f)
		, bIsCritical(false)
		, bWasBlocked(false)
		, DamageMultiplier(1.0f)
		, DistanceFalloff(1.0f)
	{
	}
};

/**
 * Lifetime statistics for the damage processor
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FDamageProcessorStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int64 TotalDamageEventsProcessed;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int64 TotalDamageDealt;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int64 TotalShieldDamageAbsorbed;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int64 CriticalHits;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int64 BlockedAttacks;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int64 KillsProcessed;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	double AverageProcessingTimeMs;

	FDamageProcessorStats()
		: TotalDamageEventsProcessed(0)
		, TotalDamageDealt(0)
		, TotalShieldDamageAbsorbed(0)
		, CriticalHits(0)
		, BlockedAttacks(0)
		, KillsProcessed(0)
		, AverageProcessingTimeMs(0.0)
	{
	}
};

// ============================================================================
// Delegate Declarations
// ============================================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDamageProcessed, AActor*, Attacker, AActor*, Target, const FDamageCalculationResult&, DamageResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActorKilledByDamage, AActor*, Killer, AActor*, Victim);

// ============================================================================
// UOdysseyDamageProcessor
// ============================================================================

/**
 * Central damage processing singleton for the Odyssey combat system.
 *
 * Responsibilities:
 * - Listens to AttackHit events on the OdysseyEventBus
 * - Calculates final damage (global multipliers, type multipliers, crits, falloff)
 * - Routes calculated damage to the target's UNPCHealthComponent
 * - Publishes DamageDealt events back to the bus for UI/stats
 * - Tracks per-session combat statistics
 *
 * The processor is a UObject singleton (not an actor) to keep it lightweight.
 * It is created on first access via Get() and rooted to prevent GC.
 */
UCLASS(BlueprintType, Blueprintable)
class ODYSSEY_API UOdysseyDamageProcessor : public UObject
{
	GENERATED_BODY()

public:
	UOdysseyDamageProcessor();

	/** Initialize the damage processor and subscribe to combat events */
	UFUNCTION(BlueprintCallable, Category = "Damage Processor")
	void Initialize();

	/** Shutdown and unsubscribe from events */
	UFUNCTION(BlueprintCallable, Category = "Damage Processor")
	void Shutdown();

	// ============================================================================
	// Core Damage Processing
	// ============================================================================

	/**
	 * Process an AttackHit combat event payload end-to-end
	 * @return True if damage was successfully applied
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	bool ProcessAttackHit(const FCombatEventPayload& AttackEvent);

	/**
	 * Calculate damage from parameters without applying it
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	FDamageCalculationResult CalculateDamage(const FDamageCalculationParams& Params);

	/**
	 * Apply a pre-calculated damage result to the target's health component
	 * @return Actual hull damage dealt
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	float ApplyDamageToTarget(AActor* Target, const FDamageCalculationResult& DamageResult, AActor* Attacker, FName DamageType = NAME_None);

	/**
	 * Convenience: calculate and apply in one call (bypasses event bus)
	 * @return Actual hull damage dealt
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	float DealDamage(AActor* Target, float DamageAmount, FName DamageType, AActor* Attacker = nullptr);

	// ============================================================================
	// Configuration
	// ============================================================================

	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	void SetGlobalDamageMultiplier(float Multiplier);

	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	void SetDamageTypeMultiplier(FName DamageType, float Multiplier);

	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	void SetCriticalHitsEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	void SetGlobalCriticalChance(float CriticalChance);

	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	void SetGlobalCriticalMultiplier(float CriticalMultiplier);

	/** Enable/disable distance-based damage falloff */
	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	void SetDistanceFalloffEnabled(bool bEnabled);

	/**
	 * Configure distance falloff curve
	 * @param MinRange Full-damage range (no falloff)
	 * @param MaxRange Zero-damage range (beyond this, damage = 0)
	 * @param Exponent Falloff curve exponent (1.0 = linear, 2.0 = quadratic)
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	void SetDistanceFalloffParams(float MinRange, float MaxRange, float Exponent = 1.0f);

	/** Set minimum damage floor (damage can never go below this after all reductions) */
	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	void SetMinimumDamage(float MinDamage);

	// ============================================================================
	// Queries & Statistics
	// ============================================================================

	UFUNCTION(BlueprintPure, Category = "Damage Processing")
	FDamageProcessorStats GetStatistics() const { return ProcessorStats; }

	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	void ResetStatistics();

	UFUNCTION(BlueprintPure, Category = "Damage Processing")
	bool IsInitialized() const { return bIsInitialized; }

	// ============================================================================
	// Events
	// ============================================================================

	/** Broadcast after damage is processed and applied */
	UPROPERTY(BlueprintAssignable, Category = "Damage Processing|Events")
	FOnDamageProcessed OnDamageProcessed;

	/** Broadcast when an actor is killed by damage routed through this processor */
	UPROPERTY(BlueprintAssignable, Category = "Damage Processing|Events")
	FOnActorKilledByDamage OnActorKilled;

	// ============================================================================
	// Singleton Access
	// ============================================================================

	UFUNCTION(BlueprintPure, Category = "Damage Processing", meta = (CallInEditor = "true"))
	static UOdysseyDamageProcessor* Get();

protected:
	// ============================================================================
	// Configuration Properties
	// ============================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Processing", meta = (ClampMin = "0.0"))
	float GlobalDamageMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Processing|Critical Hits")
	bool bCriticalHitsEnabled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Processing|Critical Hits", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GlobalCriticalChance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Processing|Critical Hits", meta = (ClampMin = "1.0"))
	float GlobalCriticalMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Processing")
	TMap<FName, float> DamageTypeMultipliers;

	/** Whether distance-based falloff is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Processing|Falloff")
	bool bDistanceFalloffEnabled;

	/** Range within which full damage is dealt */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Processing|Falloff", meta = (ClampMin = "0.0"))
	float FalloffMinRange;

	/** Range beyond which damage is zero */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Processing|Falloff", meta = (ClampMin = "0.0"))
	float FalloffMaxRange;

	/** Falloff curve exponent (1 = linear, 2 = quadratic) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Processing|Falloff", meta = (ClampMin = "0.1"))
	float FalloffExponent;

	/** Minimum damage floor after all reductions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Processing", meta = (ClampMin = "0.0"))
	float MinimumDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Processing|Debug")
	bool bVerboseLogging;

	// ============================================================================
	// Runtime State
	// ============================================================================

	bool bIsInitialized;

	UPROPERTY()
	UOdysseyEventBus* EventBus;

	TArray<FOdysseyEventHandle> EventSubscriptionHandles;

	UPROPERTY(BlueprintReadOnly, Category = "Damage Processing")
	FDamageProcessorStats ProcessorStats;

	// Performance tracking accumulators
	mutable double TotalProcessingTime;
	mutable int64 ProcessingTimeSamples;

	// ============================================================================
	// Internal Methods
	// ============================================================================

	void InitializeEventSubscriptions();
	void CleanupEventSubscriptions();

	/** EventBus callback for AttackHit events */
	void OnAttackHitEvent(const FOdysseyEventPayload& Payload);

	/** Determine if the attack scores a critical hit */
	bool RollCriticalHit(const FDamageCalculationParams& Params) const;

	/** Calculate distance-based damage falloff multiplier */
	float CalculateDistanceFalloff(float Distance) const;

	/** Locate the health component on the target */
	UNPCHealthComponent* FindHealthComponent(AActor* Target) const;

	/** Update running statistics */
	void UpdateStatistics(const FDamageCalculationResult& Result, float ShieldAbsorbed, double ProcessingTimeMs);

	/** Publish DamageDealt event to the event bus */
	void PublishDamageDealtEvent(AActor* Attacker, AActor* Target, const FDamageCalculationResult& Result);

	/** Handle post-kill logic and event broadcast */
	void HandleActorKilled(AActor* Killer, AActor* Victim);

	// ============================================================================
	// Singleton
	// ============================================================================

	static UOdysseyDamageProcessor* GlobalInstance;
};
