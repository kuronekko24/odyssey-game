// NPCShipHealthIntegration.h
// Example integration showing how to add NPCHealthComponent to existing NPCShip
// This is a demonstration file - integrate these changes into your actual NPCShip class

#pragma once

#include "CoreMinimal.h"
#include "NPCShip.h"
#include "NPCHealthComponent.h"
#include "OdysseyDamageProcessor.h"
#include "NPCShipHealthIntegration.generated.h"

/**
 * Enhanced NPCShip with integrated health component system
 * This shows how to modify the existing NPCShip to use the new health system
 */
UCLASS(BlueprintType, Blueprintable)
class ODYSSEY_API ANPCShipEnhanced : public ANPCShip
{
	GENERATED_BODY()

public:
	ANPCShipEnhanced();

protected:
	// === Enhanced Health System ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNPCHealthComponent* AdvancedHealthComponent;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// Override the existing damage method to use new health component
	virtual void TakeDamage(float DamageAmount, AActor* DamageSource = nullptr) override;

	// Enhanced health queries
	UFUNCTION(BlueprintCallable, Category = "Enhanced Health")
	EHealthState GetDetailedHealthState() const;

	UFUNCTION(BlueprintCallable, Category = "Enhanced Health")
	bool IsHealthRegenerating() const;

	UFUNCTION(BlueprintCallable, Category = "Enhanced Health")
	float GetDamageResistance(FName DamageType) const;

	// Configuration methods
	UFUNCTION(BlueprintCallable, Category = "Enhanced Health")
	void ConfigureShipResistances();

	UFUNCTION(BlueprintCallable, Category = "Enhanced Health")
	void SetupHealthRegeneration();

protected:
	// === Event Handlers for New Health System ===
	UFUNCTION()
	void OnAdvancedHealthChanged(const FHealthEventPayload& HealthData);

	UFUNCTION()
	void OnAdvancedHealthStateChanged(EHealthState NewState);

	UFUNCTION()
	void OnAdvancedActorDied(AActor* DiedActor);

	// === Integration Helpers ===
	void InitializeAdvancedHealthSystem();
	void SynchronizeHealthSystems();
	void ConfigureDamageProcessor();
};
