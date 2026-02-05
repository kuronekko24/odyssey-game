// OdysseyCombatDemo.h
// Demo component showing how to set up and use the combat system
// Provides example configuration and usage patterns

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OdysseyCombatDemo.generated.h"

// Forward declarations
class UOdysseyCombatManager;
class UOdysseyCombatIntegration;
class ANPCSpawnManager;

/**
 * Combat Demo Component
 * 
 * Demonstrates the combat system setup and usage:
 * - Sets up combat system with appropriate configuration
 * - Spawns demo enemies for testing
 * - Provides example combat scenarios
 * - Shows integration with existing Odyssey systems
 */
UCLASS(ClassGroup=(Odyssey), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UOdysseyCombatDemo : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdysseyCombatDemo();

protected:
	virtual void BeginPlay() override;

	// ============================================================================
	// Demo Setup
	// ============================================================================

	/**
	 * Initialize the combat demo
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat Demo")
	void InitializeCombatDemo();

	/**
	 * Setup combat system with demo configuration
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat Demo")
	void SetupCombatSystem();

	/**
	 * Spawn demo enemies for combat testing
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat Demo")
	void SpawnDemoEnemies();

	/**
	 * Start a demo combat scenario
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat Demo")
	void StartDemoScenario();

	/**
	 * Stop the demo and clean up
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat Demo")
	void StopDemo();

protected:
	/**
	 * Combat system manager
	 */
	UPROPERTY()
	UOdysseyCombatManager* CombatManager;

	/**
	 * Combat integration component
	 */
	UPROPERTY()
	UOdysseyCombatIntegration* CombatIntegration;

	/**
	 * NPC spawn manager for demo enemies
	 */
	UPROPERTY()
	ANPCSpawnManager* NPCSpawnManager;

	/**
	 * Whether demo is active
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Demo State")
	bool bDemoActive;

	/**
	 * Demo enemy locations
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo Configuration")
	TArray<FVector> DemoEnemyLocations;

	/**
	 * Demo configuration flags
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo Configuration")
	bool bAutoStartDemo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo Configuration")
	bool bShowDemoInstructions;

	// ============================================================================
	// Blueprint Events
	// ============================================================================

	UFUNCTION(BlueprintImplementableEvent, Category = "Demo Events")
	void OnDemoStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Demo Events")
	void OnDemoEnded();

	UFUNCTION(BlueprintImplementableEvent, Category = "Demo Events")
	void OnEnemySpawned(AActor* Enemy);

	UFUNCTION(BlueprintImplementableEvent, Category = "Demo Events")
	void OnCombatEngaged();

private:
	/**
	 * Initialize component references
	 */
	void InitializeReferences();

	/**
	 * Create combat components if they don't exist
	 */
	void EnsureCombatComponents();

	/**
	 * Configure combat system for demo
	 */
	void ConfigureCombatForDemo();

	/**
	 * Get demo spawn locations around player
	 */
	TArray<FVector> GetDemoSpawnLocations();
};
