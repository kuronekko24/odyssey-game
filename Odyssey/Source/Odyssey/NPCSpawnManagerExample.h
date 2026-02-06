#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPCSpawnManager.h"
#include "NPCSpawnManagerExample.generated.h"

/**
 * Example class demonstrating setup and runtime control of the NPC Spawn Manager.
 * Place in a level alongside an ANPCSpawnManager.
 */
UCLASS(BlueprintType, Blueprintable)
class ODYSSEY_API ANPCSpawnManagerExample : public AActor
{
	GENERATED_BODY()

public:
	ANPCSpawnManagerExample();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Management")
	ANPCSpawnManager* NPCSpawnManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Setup")
	TSubclassOf<AOdysseyCharacter> ExampleNPCClass;

protected:
	virtual void BeginPlay() override;

public:
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
