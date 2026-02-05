// OdysseyCombatIntegration.h
// Integration component that connects combat system to existing Odyssey systems
// Extends action button manager with combat actions

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OdysseyActionEvent.h"
#include "OdysseyCombatIntegration.generated.h"

// Forward declarations
class UOdysseyCombatManager;
class UOdysseyActionButtonManager;
class UOdysseyTouchInterface;

/**
 * Combat Integration Component
 * 
 * Bridges the combat system with the existing Odyssey action system:
 * - Registers combat actions with action button manager
 * - Handles combat touch input through touch interface
 * - Integrates combat events with the event system
 * - Provides backwards compatibility with existing systems
 */
UCLASS(ClassGroup=(Odyssey), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UOdysseyCombatIntegration : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdysseyCombatIntegration();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ============================================================================
	// Action Button Integration
	// ============================================================================

	/**
	 * Setup combat actions in the action button manager
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat Integration")
	void SetupCombatActions();

	/**
	 * Handle attack button activation
	 */
	UFUNCTION()
	void OnAttackButtonPressed();

	/**
	 * Handle special attack button activation
	 */
	UFUNCTION()
	void OnSpecialAttackButtonPressed();

	// ============================================================================
	// Touch Interface Integration
	// ============================================================================

	/**
	 * Setup combat touch handlers
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat Integration")
	void SetupTouchIntegration();

	/**
	 * Handle touch input for combat targeting
	 */
	UFUNCTION()
	void OnCombatTouch(FVector2D TouchLocation, int32 FingerIndex);

protected:
	/**
	 * Reference to combat manager
	 */
	UPROPERTY()
	UOdysseyCombatManager* CombatManager;

	/**
	 * Reference to action button manager
	 */
	UPROPERTY()
	UOdysseyActionButtonManager* ActionButtonManager;

	/**
	 * Reference to touch interface
	 */
	UPROPERTY()
	UOdysseyTouchInterface* TouchInterface;

	/**
	 * Whether combat actions are registered
	 */
	bool bActionsRegistered;

	/**
	 * Whether touch integration is active
	 */
	bool bTouchIntegrationActive;

private:
	/**
	 * Initialize component references
	 */
	void InitializeReferences();

	/**
	 * Clean up integrations
	 */
	void CleanupIntegrations();
};
