// NPCShipHealthIntegration.h
// Enhanced NPCShip subclass with full UNPCHealthComponent integration
// Bridges the existing NPCShip combat system to the new health/damage foundation
// Phase 1: Health & Damage Foundation

#pragma once

#include "CoreMinimal.h"
#include "NPCShip.h"
#include "NPCHealthComponent.h"
#include "OdysseyDamageProcessor.h"
#include "NPCShipHealthIntegration.generated.h"

/**
 * Enhanced NPC ship that replaces the legacy per-variable health/shield tracking
 * with the component-based UNPCHealthComponent system.
 *
 * Usage:
 *   - Place ANPCShipEnhanced in a level or spawn via CreateNPCShip()
 *   - Configure ShipConfig as usual; the component auto-syncs
 *   - Damage flows: AttackHit event -> DamageProcessor -> HealthComponent
 *   - Legacy methods (TakeDamage, Heal, GetHealthPercentage) still work
 *   - Health state changes feed back into the NPC behavior component
 */
UCLASS(BlueprintType, Blueprintable)
class ODYSSEY_API ANPCShipEnhanced : public ANPCShip
{
	GENERATED_BODY()

public:
	ANPCShipEnhanced();

protected:
	// === Health System Component ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNPCHealthComponent* AdvancedHealthComponent;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// === Overridden Legacy Interface ===

	/** Route damage through the advanced health component */
	virtual void TakeDamage(float DamageAmount, AActor* DamageSource = nullptr) override;

	// === Enhanced Queries ===

	UFUNCTION(BlueprintCallable, Category = "Enhanced Health")
	EHealthState GetDetailedHealthState() const;

	UFUNCTION(BlueprintPure, Category = "Enhanced Health")
	bool IsHealthRegenerating() const;

	UFUNCTION(BlueprintPure, Category = "Enhanced Health")
	bool IsShieldRegenerating() const;

	UFUNCTION(BlueprintCallable, Category = "Enhanced Health")
	float GetDamageResistance(FName DamageType) const;

	UFUNCTION(BlueprintPure, Category = "Enhanced Health")
	UNPCHealthComponent* GetAdvancedHealthComponent() const { return AdvancedHealthComponent; }

	// === Configuration ===

	/** Apply ship-type-specific resistances to the health component */
	UFUNCTION(BlueprintCallable, Category = "Enhanced Health")
	void ConfigureShipResistances();

	/** Apply ship-type-specific regeneration settings */
	UFUNCTION(BlueprintCallable, Category = "Enhanced Health")
	void SetupHealthRegeneration();

	/** Apply ship-type-specific shield settings */
	UFUNCTION(BlueprintCallable, Category = "Enhanced Health")
	void ConfigureShipShields();

protected:
	// === Health Event Handlers (bound to AdvancedHealthComponent delegates) ===

	UFUNCTION()
	void OnAdvancedHealthChanged(const FHealthEventPayload& HealthData);

	UFUNCTION()
	void OnAdvancedHealthStateChanged(EHealthState NewState);

	UFUNCTION()
	void OnAdvancedActorDied(AActor* DiedActor);

	UFUNCTION()
	void OnAdvancedShieldBroken(AActor* Owner, AActor* DamageSource);

	UFUNCTION()
	void OnAdvancedShieldRestored(AActor* Owner, float ShieldAmount);

	// === Integration Helpers ===

	/** Wire up the advanced health component with ship configuration */
	void InitializeAdvancedHealthSystem();

	/** Copy component state into legacy ANPCShip member variables */
	void SynchronizeLegacyHealthVariables();

	/** Ensure the damage processor singleton is configured */
	void EnsureDamageProcessorConfigured();
};
