// OdysseyDamageProcessor.h
// Central damage processing system that bridges combat events to actual damage application
// Handles damage calculation, validation, and routing to health components

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "OdysseyActionEvent.h"
#include "OdysseyDamageProcessor.generated.h"

class UOdysseyEventBus;
class UNPCHealthComponent;

/**
 * Damage calculation parameters
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

	UPROPERTY(BlueprintReadWrite, Category = "Damage")
	float CriticalChance;

	UPROPERTY(BlueprintReadWrite, Category = "Damage")
	float CriticalMultiplier;

	UPROPERTY(BlueprintReadWrite, Category = "Damage")
	TMap<FName, float> DamageModifiers;

	FDamageCalculationParams()
		: BaseDamage(0.0f)
		, DamageType(NAME_None)
		, HitLocation(FVector::ZeroVector)
		, CriticalChance(0.05f)
		, CriticalMultiplier(2.0f)
	{
	}
};

/**
 * Result of damage calculation
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
	FString CalculationDetails;

	FDamageCalculationResult()
		: FinalDamage(0.0f)
		, bIsCritical(false)
		, bWasBlocked(false)
		, DamageMultiplier(1.0f)
	{
	}
};

/**
 * Damage processing statistics
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
		, CriticalHits(0)
		, BlockedAttacks(0)
		, KillsProcessed(0)
		, AverageProcessingTimeMs(0.0)
	{
	}
};

/**
 * Delegate for damage processing events
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDamageProcessedDelegate, AActor*, Attacker, AActor*, Target, const FDamageCalculationResult&, DamageResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FActorKilledDelegate, AActor*, Killer, AActor*, Victim);

/**
 * Central damage processing system for the Odyssey combat system
 * 
 * Features:
 * - Event-driven damage processing with OdysseyEventBus integration
 * - Advanced damage calculation with modifiers, criticals, and blocking
 * - Mobile-optimized with object pooling and minimal allocations
 * - Comprehensive damage statistics and debugging
 * - Blueprint-friendly interface for designers
 * - Support for damage types, resistances, and custom calculations
 * - Integration with health components for actual damage application
 */
UCLASS(BlueprintType, Blueprintable)
class ODYSSEY_API UOdysseyDamageProcessor : public UObject
{
	GENERATED_BODY()

public:
	UOdysseyDamageProcessor();

	/**
	 * Initialize the damage processor and set up event subscriptions
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Processor")
	void Initialize();

	/**
	 * Shutdown the damage processor and clean up
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Processor")
	void Shutdown();

	// ============================================================================
	// Core Damage Processing
	// ============================================================================

	/**
	 * Process damage from an attack hit event
	 * @param AttackEvent The attack hit event to process
	 * @return True if damage was successfully processed
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	bool ProcessAttackHit(const FCombatEventPayload& AttackEvent);

	/**
	 * Calculate damage based on parameters
	 * @param Params Damage calculation parameters
	 * @return Calculated damage result
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	FDamageCalculationResult CalculateDamage(const FDamageCalculationParams& Params);

	/**
	 * Apply calculated damage to target actor
	 * @param Target Actor to receive damage
	 * @param DamageResult Calculated damage to apply
	 * @param Attacker Actor dealing the damage
	 * @return Actual damage applied
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	float ApplyDamageToTarget(AActor* Target, const FDamageCalculationResult& DamageResult, AActor* Attacker);

	/**
	 * Direct damage application (bypasses event system)
	 * @param Target Actor to damage
	 * @param DamageAmount Amount of damage
	 * @param DamageType Type of damage
	 * @param Attacker Source of damage
	 * @return Actual damage applied
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	float DealDamage(AActor* Target, float DamageAmount, FName DamageType, AActor* Attacker = nullptr);

	// ============================================================================
	// Configuration
	// ============================================================================

	/**
	 * Set global damage multiplier (affects all damage)
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	void SetGlobalDamageMultiplier(float Multiplier);

	/**
	 * Set damage multiplier for specific damage type
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	void SetDamageTypeMultiplier(FName DamageType, float Multiplier);

	/**
	 * Enable or disable critical hits globally
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	void SetCriticalHitsEnabled(bool bEnabled);

	/**
	 * Set global critical hit chance modifier
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	void SetGlobalCriticalChance(float CriticalChance);

	/**
	 * Set global critical damage multiplier
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	void SetGlobalCriticalMultiplier(float CriticalMultiplier);

	// ============================================================================
	// Queries and Statistics
	// ============================================================================

	/**
	 * Get damage processing statistics
	 */
	UFUNCTION(BlueprintPure, Category = "Damage Processing")
	FDamageProcessorStats GetStatistics() const { return ProcessorStats; }

	/**
	 * Reset damage processing statistics
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Processing")
	void ResetStatistics();

	/**
	 * Check if damage processor is initialized
	 */
	UFUNCTION(BlueprintPure, Category = "Damage Processing")
	bool IsInitialized() const { return bIsInitialized; }

	// ============================================================================
	// Events and Delegates
	// ============================================================================

	/**
	 * Called when damage is processed and applied
	 */
	UPROPERTY(BlueprintAssignable, Category = "Damage Processing|Events")
	FDamageProcessedDelegate OnDamageProcessed;

	/**
	 * Called when an actor is killed by damage
	 */
	UPROPERTY(BlueprintAssignable, Category = "Damage Processing|Events")
	FActorKilledDelegate OnActorKilled;

	// ============================================================================
	// Singleton Access
	// ============================================================================

	/**
	 * Get the global damage processor instance
	 */
	UFUNCTION(BlueprintPure, Category = "Damage Processing", meta = (CallInEditor = "true"))
	static UOdysseyDamageProcessor* Get();

protected:
	// ============================================================================
	// Configuration Properties
	// ============================================================================

	/**
	 * Global damage multiplier applied to all damage
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Processing", meta = (ClampMin = "0.0"))
	float GlobalDamageMultiplier;

	/**
	 * Whether critical hits are enabled globally
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Processing|Critical Hits")
	bool bCriticalHitsEnabled;

	/**
	 * Global critical hit chance modifier
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Processing|Critical Hits", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GlobalCriticalChance;

	/**
	 * Global critical damage multiplier
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Processing|Critical Hits", meta = (ClampMin = "1.0"))
	float GlobalCriticalMultiplier;

	/**
	 * Damage type multipliers
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Processing")
	TMap<FName, float> DamageTypeMultipliers;

	/**
	 * Whether to log detailed damage calculations
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Processing|Debug")
	bool bVerboseLogging;

	// ============================================================================
	// Runtime State
	// ============================================================================

	/**
	 * Whether the damage processor is initialized
	 */
	bool bIsInitialized;

	/**
	 * Reference to the global event bus
	 */
	UPROPERTY()
	UOdysseyEventBus* EventBus;

	/**
	 * Event subscription handles for cleanup
	 */
	TArray<FOdysseyEventHandle> EventSubscriptionHandles;

	/**
	 * Damage processing statistics
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Damage Processing")
	FDamageProcessorStats ProcessorStats;

	/**
	 * Performance tracking
	 */
	mutable double TotalProcessingTime;
	mutable int64 ProcessingTimeSamples;

	// ============================================================================
	// Internal Methods
	// ============================================================================

	/**
	 * Initialize event bus subscriptions
	 */
	void InitializeEventSubscriptions();

	/**
	 * Clean up event bus subscriptions
	 */
	void CleanupEventSubscriptions();

	/**
	 * Handle incoming attack hit events
	 */
	void OnAttackHitEvent(const FOdysseyEventPayload& Payload);

	/**
	 * Calculate if an attack is a critical hit
	 */
	bool CalculateCriticalHit(const FDamageCalculationParams& Params) const;

	/**
	 * Calculate damage blocking
	 */
	bool CalculateBlocking(const FDamageCalculationParams& Params) const;

	/**
	 * Apply damage modifiers to base damage
	 */
	float ApplyDamageModifiers(float BaseDamage, const FDamageCalculationParams& Params) const;

	/**
	 * Find health component on target actor
	 */
	UNPCHealthComponent* FindHealthComponent(AActor* Target) const;

	/**
	 * Update processing statistics
	 */
	void UpdateStatistics(const FDamageCalculationResult& Result, double ProcessingTimeMs);

	/**
	 * Broadcast damage processed event
	 */
	void BroadcastDamageEvent(AActor* Attacker, AActor* Target, const FDamageCalculationResult& Result);

	/**
	 * Handle actor death
	 */
	void HandleActorKilled(AActor* Killer, AActor* Victim);

	// ============================================================================
	// Singleton
	// ============================================================================

	/**
	 * Global instance of the damage processor
	 */
	static UOdysseyDamageProcessor* GlobalInstance;
};
