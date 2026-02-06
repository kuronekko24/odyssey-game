// NPCShip.h
// NPC Ship class extending AOdysseyCharacter with AI behavior management
// Component-based architecture with event-driven combat and performance tier awareness

#pragma once

#include "CoreMinimal.h"
#include "OdysseyCharacter.h"
#include "NPCBehaviorComponent.h"
#include "OdysseyMobileOptimizer.h"
#include "NPCShip.generated.h"

// Forward declarations
class UNPCBehaviorComponent;
class UNPCHealthComponent;
class UOdysseyEventBus;
class UOdysseyActionDispatcher;
class UOdysseyActionCommand;

/**
 * NPC Ship Type enumeration
 * Defines different types of NPCs with different behaviors and combat profiles
 */
UENUM(BlueprintType)
enum class ENPCShipType : uint8
{
	Civilian,		// Non-hostile, trading/mining ships
	Pirate,			// Hostile, aggressive behavior
	Security,		// Hostile to pirates, neutral to players
	Escort			// Follows and protects specific targets
};

/**
 * NPC Ship Configuration
 * Contains all the settings for configuring NPC behavior and combat stats
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FNPCShipConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Config")
	ENPCShipType ShipType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Config")
	FString ShipName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Config")
	float MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Config")
	float AttackDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Config")
	float MovementSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Config")
	bool bCanRespawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Config")
	float RespawnDelay;

	/** Detection radius override (0 = use component default) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Config")
	float DetectionRadius;

	/** Engagement range override (0 = use component default) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Config")
	float EngagementRange;

	/** Attack cooldown in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Config")
	float AttackCooldown;

	FNPCShipConfig()
		: ShipType(ENPCShipType::Civilian)
		, ShipName(TEXT("Unknown Ship"))
		, MaxHealth(100.0f)
		, AttackDamage(25.0f)
		, MovementSpeed(400.0f)
		, bCanRespawn(false)
		, RespawnDelay(30.0f)
		, DetectionRadius(0.0f)
		, EngagementRange(0.0f)
		, AttackCooldown(2.0f)
	{
	}
};

/**
 * NPC combat statistics for tracking and debugging
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FNPCCombatStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "NPC Stats")
	int32 TotalAttacks;

	UPROPERTY(BlueprintReadOnly, Category = "NPC Stats")
	float TotalDamageDealt;

	UPROPERTY(BlueprintReadOnly, Category = "NPC Stats")
	float TotalDamageTaken;

	UPROPERTY(BlueprintReadOnly, Category = "NPC Stats")
	int32 DeathCount;

	UPROPERTY(BlueprintReadOnly, Category = "NPC Stats")
	int32 RespawnCount;

	UPROPERTY(BlueprintReadOnly, Category = "NPC Stats")
	float TotalTimeAlive;

	UPROPERTY(BlueprintReadOnly, Category = "NPC Stats")
	float TotalTimeInCombat;

	FNPCCombatStats()
		: TotalAttacks(0)
		, TotalDamageDealt(0.0f)
		, TotalDamageTaken(0.0f)
		, DeathCount(0)
		, RespawnCount(0)
		, TotalTimeAlive(0.0f)
		, TotalTimeInCombat(0.0f)
	{
	}

	void Reset()
	{
		TotalAttacks = 0;
		TotalDamageDealt = 0.0f;
		TotalDamageTaken = 0.0f;
		DeathCount = 0;
		RespawnCount = 0;
		TotalTimeAlive = 0.0f;
		TotalTimeInCombat = 0.0f;
	}
};

/**
 * Delegate for NPC lifecycle events
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNPCShipDeathDelegate, ANPCShip*, DeadShip);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNPCShipRespawnDelegate, ANPCShip*, RespawnedShip);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FNPCShipDamageDelegate, ANPCShip*, DamagedShip, float, DamageAmount, AActor*, DamageSource);

/**
 * ANPCShip
 * NPC ship class extending AOdysseyCharacter with AI behavior management
 *
 * Architecture:
 * - Extends AOdysseyCharacter to reuse component system (mesh, interaction, inventory)
 * - UNPCBehaviorComponent handles all AI state machine logic
 * - Combat integrates with OdysseyEventBus and action command patterns
 * - Performance tier awareness for mobile optimization
 * - Shield regeneration with configurable delay
 * - Respawn system with timer-based resurrection
 *
 * Component Composition:
 * - ShipMesh (from AOdysseyCharacter)
 * - InteractionSphere (from AOdysseyCharacter)
 * - InventoryComponent (from AOdysseyCharacter) - for loot drops
 * - BehaviorComponent (NPC-specific AI state machine)
 *
 * Event Integration:
 * - Publishes: DamageDealt, DamageReceived, Death, Respawn events
 * - Subscribes: Performance tier changes, combat events targeting this ship
 */
UCLASS(BlueprintType, Blueprintable)
class ODYSSEY_API ANPCShip : public AOdysseyCharacter
{
	GENERATED_BODY()

public:
	ANPCShip();

protected:
	// === Core Components ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNPCBehaviorComponent* BehaviorComponent;

	// === NPC Configuration ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Configuration")
	FNPCShipConfig ShipConfig;

	// === Combat Stats ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float CurrentHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MaxShields;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float CurrentShields;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float ShieldRegenRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float ShieldRegenDelay;

	// === Respawn System ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Respawn")
	bool bIsDead;

	UPROPERTY()
	FVector RespawnLocation;

	UPROPERTY()
	FRotator RespawnRotation;

	UPROPERTY()
	FTimerHandle RespawnTimerHandle;

	// === Shield Regeneration ===
	UPROPERTY()
	FTimerHandle ShieldRegenTimerHandle;

	UPROPERTY()
	float LastDamageTime;

	// === Combat Statistics ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statistics")
	FNPCCombatStats CombatStats;

	// === Performance Tier ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Performance")
	EPerformanceTier CurrentPerformanceTier;

	// === Event Integration ===
	UPROPERTY()
	UOdysseyEventBus* EventBus;

	/** Event subscription handles for cleanup */
	TArray<FOdysseyEventHandle> EventSubscriptionHandles;

	/** Time when this ship spawned or last respawned */
	float SpawnTime;

	/** Accumulated time in engaging state for stats */
	float CombatStateEnterTime;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;

	// === Combat System ===
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void TakeDamage(float DamageAmount, AActor* DamageSource = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Heal(float HealAmount);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void RestoreShields(float ShieldAmount);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool IsAlive() const { return !bIsDead && CurrentHealth > 0.0f; }

	UFUNCTION(BlueprintCallable, Category = "Combat")
	float GetHealthPercentage() const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	float GetShieldPercentage() const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintCallable, Category = "Combat")
	float GetMaxHealth() const { return ShipConfig.MaxHealth; }

	UFUNCTION(BlueprintCallable, Category = "Combat")
	float GetCurrentShields() const { return CurrentShields; }

	UFUNCTION(BlueprintCallable, Category = "Combat")
	float GetMaxShields() const { return MaxShields; }

	// === Death and Respawn ===
	UFUNCTION(BlueprintCallable, Category = "Death")
	void Die();

	UFUNCTION(BlueprintCallable, Category = "Respawn")
	void Respawn();

	UFUNCTION(BlueprintCallable, Category = "Respawn")
	void SetRespawnLocation(const FVector& Location, const FRotator& Rotation);

	UFUNCTION(BlueprintCallable, Category = "Respawn")
	bool CanRespawn() const { return ShipConfig.bCanRespawn; }

	// === Configuration ===
	UFUNCTION(BlueprintCallable, Category = "Configuration")
	void SetShipConfig(const FNPCShipConfig& NewConfig);

	UFUNCTION(BlueprintCallable, Category = "Configuration")
	FNPCShipConfig GetShipConfig() const { return ShipConfig; }

	UFUNCTION(BlueprintCallable, Category = "Configuration")
	ENPCShipType GetShipType() const { return ShipConfig.ShipType; }

	// === Behavior Component Access ===
	UFUNCTION(BlueprintCallable, Category = "AI")
	UNPCBehaviorComponent* GetBehaviorComponent() const { return BehaviorComponent; }

	// === Patrol Configuration ===
	UFUNCTION(BlueprintCallable, Category = "Patrol")
	void SetPatrolRoute(const TArray<FVector>& PatrolPoints);

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	void StartPatrol();

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	void StopPatrol();

	// === Combat Actions ===
	UFUNCTION(BlueprintCallable, Category = "Combat Actions")
	void AttackTarget(AOdysseyCharacter* Target);

	UFUNCTION(BlueprintCallable, Category = "Combat Actions")
	bool CanAttackTarget(AOdysseyCharacter* Target) const;

	// === Performance Tier ===
	UFUNCTION(BlueprintCallable, Category = "Performance")
	void SetPerformanceTier(EPerformanceTier NewTier);

	UFUNCTION(BlueprintCallable, Category = "Performance")
	EPerformanceTier GetPerformanceTier() const { return CurrentPerformanceTier; }

	// === Statistics ===
	UFUNCTION(BlueprintCallable, Category = "Statistics")
	FNPCCombatStats GetCombatStats() const { return CombatStats; }

	UFUNCTION(BlueprintCallable, Category = "Statistics")
	void ResetCombatStats() { CombatStats.Reset(); }

	// === Delegates ===
	UPROPERTY(BlueprintAssignable, Category = "NPC Events")
	FNPCShipDeathDelegate OnNPCDeath;

	UPROPERTY(BlueprintAssignable, Category = "NPC Events")
	FNPCShipRespawnDelegate OnNPCRespawn;

	UPROPERTY(BlueprintAssignable, Category = "NPC Events")
	FNPCShipDamageDelegate OnNPCDamaged;

	// === Blueprint Events ===
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Events")
	void OnHealthChanged(float OldHealth, float NewHealth);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Events")
	void OnShieldChanged(float OldShield, float NewShield);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Events")
	void OnDamageTaken(float DamageAmount, AActor* DamageSource);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Events")
	void OnDeath();

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Events")
	void OnRespawned();

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Events")
	void OnAttackPerformed(AOdysseyCharacter* Target, float Damage);

	UFUNCTION(BlueprintImplementableEvent, Category = "AI Events")
	void OnBehaviorStateChanged(ENPCState OldState, ENPCState NewState);

	// === Static Factory Methods ===
	UFUNCTION(BlueprintCallable, Category = "Factory", meta = (WorldContext = "WorldContext"))
	static ANPCShip* CreateNPCShip(UObject* WorldContext, ENPCShipType ShipType, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator);

	/** Create an NPC ship with a full configuration struct */
	UFUNCTION(BlueprintCallable, Category = "Factory", meta = (WorldContext = "WorldContext"))
	static ANPCShip* CreateConfiguredNPCShip(UObject* WorldContext, const FNPCShipConfig& Config, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator);

	// === Utility ===
	UFUNCTION(BlueprintCallable, Category = "Utility")
	FString GetShipDisplayName() const;

	UFUNCTION(BlueprintCallable, Category = "Utility")
	FText GetShipStatusText() const;

protected:
	// === Internal Systems ===
	void InitializeNPCShip();
	void SetupComponentReferences();
	void ConfigureMovementForNPC();
	void ApplyShipConfigToBehavior();
	void StartShieldRegeneration();
	void StopShieldRegeneration();

	// === Timer Callbacks ===
	UFUNCTION()
	void OnRespawnTimerExpired();

	UFUNCTION()
	void OnShieldRegenTick();

	// === Combat Helpers ===
	float CalculateDamageAfterShields(float IncomingDamage);
	void ApplyDamageToShields(float DamageAmount);
	void ApplyDamageToHealth(float DamageAmount);

	// === Event System Integration ===
	void RegisterWithEventBus();
	void UnregisterFromEventBus();
	void PublishDamageEvent(float DamageAmount, AActor* DamageSource);
	void PublishDeathEvent();
	void PublishRespawnEvent();
	void PublishAttackEvent(AOdysseyCharacter* Target, float Damage);

	// === Behavior State Change Handler ===
	UFUNCTION()
	void HandleBehaviorStateChanged(ENPCState OldState, ENPCState NewState);

	// === Statistics Tracking ===
	void UpdateAliveTimeStats();
};
