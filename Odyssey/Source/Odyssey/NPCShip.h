#pragma once

#include "CoreMinimal.h"
#include "OdysseyCharacter.h"
#include "NPCBehaviorComponent.h"
#include "NPCShip.generated.h"

// Forward declarations
class UNPCBehaviorComponent;
class UOdysseyEventBus;

/**
 * NPC Ship Type enumeration
 * Defines different types of NPCs with different behaviors
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
 * Contains all the settings for configuring NPC behavior
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

	FNPCShipConfig()
	{
		ShipType = ENPCShipType::Civilian;
		ShipName = TEXT("Unknown Ship");
		MaxHealth = 100.0f;
		AttackDamage = 25.0f;
		MovementSpeed = 400.0f;
		bCanRespawn = false;
		RespawnDelay = 30.0f;
	}
};

/**
 * ANPCShip
 * NPC ship class extending AOdysseyCharacter with AI behavior
 * Designed for mobile-friendly performance with component-based architecture
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

	// === Event Integration ===
	UPROPERTY()
	UOdysseyEventBus* EventBus;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;

	// === Combat System ===
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void TakeDamage(float DamageAmount, AActor* DamageSource = nullptr);

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

	// === Death and Respawn ===
	UFUNCTION(BlueprintCallable, Category = "Death")
	void Die();

	UFUNCTION(BlueprintCallable, Category = "Respawn")
	void Respawn();

	UFUNCTION(BlueprintCallable, Category = "Respawn")
	void SetRespawnLocation(const FVector& Location, const FRotator& Rotation);

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

	// === Events ===
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
	UFUNCTION(BlueprintCallable, Category = "Factory", CallInEditor = true, meta = (Category = "NPC Factory"))
	static ANPCShip* CreateNPCShip(UWorld* World, ENPCShipType ShipType, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator);

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
	void BroadcastNPCEvent(const FString& EventType, const TMap<FString, FString>& EventData);
};
