#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPCSpawnManager.h"
#include "NPCSpawnManagerExample.generated.h"

/**
 * Example class showing how to set up and use the NPC Spawn Manager
 * This demonstrates the mobile-optimized NPC management system
 */
UCLASS(BlueprintType, Blueprintable)
class ODYSSEY_API ANPCSpawnManagerExample : public AActor
{
	GENERATED_BODY()

public:
	ANPCSpawnManagerExample();

protected:
	// Reference to the NPC Spawn Manager
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Management")
	ANPCSpawnManager* NPCSpawnManager;

	// Example NPC class to spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Setup")
	TSubclassOf<AOdysseyCharacter> ExampleNPCClass;

protected:
	virtual void BeginPlay() override;

public:
	// Setup functions for demonstration
	UFUNCTION(BlueprintCallable, Category = "NPC Setup")
	void SetupExampleNPCs();

	UFUNCTION(BlueprintCallable, Category = "NPC Setup")
	void CreatePatrolRoute(const FString& RouteName, const TArray<FVector>& WaypointLocations);

	UFUNCTION(BlueprintCallable, Category = "NPC Management")
	void SpawnTestNPCs();

	UFUNCTION(BlueprintCallable, Category = "NPC Management")
	void ClearAllNPCs();

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void ToggleDebugDisplay();

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void PrintNPCSystemStats();

private:
	bool bDebugDisplayEnabled;
};
