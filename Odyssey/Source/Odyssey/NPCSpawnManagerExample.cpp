#include "NPCSpawnManagerExample.h"
#include "OdysseyCharacter.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ANPCSpawnManagerExample::ANPCSpawnManagerExample()
{
	PrimaryActorTick.bCanEverTick = false;
	
	NPCSpawnManager = nullptr;
	ExampleNPCClass = AOdysseyCharacter::StaticClass();
	bDebugDisplayEnabled = false;
}

void ANPCSpawnManagerExample::BeginPlay()
{
	Super::BeginPlay();

	// Find or create NPC Spawn Manager
	if (UWorld* World = GetWorld())
	{
		NPCSpawnManager = World->SpawnActor<ANPCSpawnManager>();
		if (NPCSpawnManager)
		{
			UE_LOG(LogTemp, Log, TEXT("NPC Spawn Manager created successfully"));
			
			// Setup example configuration after a short delay to ensure everything is initialized
			FTimerHandle TimerHandle;
			World->GetTimerManager().SetTimer(TimerHandle, this, &ANPCSpawnManagerExample::SetupExampleNPCs, 1.0f, false);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create NPC Spawn Manager"));
		}
	}
}

void ANPCSpawnManagerExample::SetupExampleNPCs()
{
	if (!NPCSpawnManager || !ExampleNPCClass)
	{
		UE_LOG(LogTemp, Error, TEXT("NPCSpawnManager or ExampleNPCClass is null"));
		return;
	}

	// Example spawn configuration
	FVector BaseLocation = GetActorLocation();
	
	// Create a simple patrol route around the spawn manager
	TArray<FWaypoint> PatrolWaypoints;
	PatrolWaypoints.Add(FWaypoint(BaseLocation + FVector(500, 0, 0), 2.0f, true));
	PatrolWaypoints.Add(FWaypoint(BaseLocation + FVector(500, 500, 0), 1.0f, false));
	PatrolWaypoints.Add(FWaypoint(BaseLocation + FVector(0, 500, 0), 2.0f, true));
	PatrolWaypoints.Add(FWaypoint(BaseLocation + FVector(-500, 500, 0), 1.0f, false));
	PatrolWaypoints.Add(FWaypoint(BaseLocation + FVector(-500, 0, 0), 2.0f, false));
	PatrolWaypoints.Add(FWaypoint(BaseLocation + FVector(-500, -500, 0), 1.0f, false));
	PatrolWaypoints.Add(FWaypoint(BaseLocation + FVector(0, -500, 0), 2.0f, true));
	PatrolWaypoints.Add(FWaypoint(BaseLocation + FVector(500, -500, 0), 1.0f, false));

	FPatrolRoute ExampleRoute;
	ExampleRoute.RouteName = TEXT("Example Patrol Route");
	ExampleRoute.Waypoints = PatrolWaypoints;
	ExampleRoute.bLooping = true;
	ExampleRoute.MovementSpeed = 200.0f;
	ExampleRoute.ActivationDistance = 2000.0f;

	// Create NPC spawn data with different priorities
	TArray<FNPCSpawnData> SpawnData;

	// Essential NPC (always spawns regardless of performance)
	FNPCSpawnData EssentialNPC;
	EssentialNPC.NPCClass = ExampleNPCClass;
	EssentialNPC.SpawnLocation = BaseLocation;
	EssentialNPC.SpawnRotation = FRotator::ZeroRotator;
	EssentialNPC.PatrolRoute = ExampleRoute;
	EssentialNPC.Priority = 100;
	EssentialNPC.bEssential = true;
	SpawnData.Add(EssentialNPC);

	// High priority NPCs
	for (int32 i = 0; i < 3; i++)
	{
		FNPCSpawnData HighPriorityNPC;
		HighPriorityNPC.NPCClass = ExampleNPCClass;
		HighPriorityNPC.SpawnLocation = BaseLocation + FVector(i * 200, 0, 0);
		HighPriorityNPC.SpawnRotation = FRotator(0, i * 90, 0);
		
		// Create smaller patrol routes for these NPCs
		TArray<FWaypoint> SmallRoute;
		SmallRoute.Add(FWaypoint(HighPriorityNPC.SpawnLocation + FVector(100, 0, 0), 1.0f));
		SmallRoute.Add(FWaypoint(HighPriorityNPC.SpawnLocation + FVector(100, 100, 0), 1.0f));
		SmallRoute.Add(FWaypoint(HighPriorityNPC.SpawnLocation + FVector(0, 100, 0), 1.0f));
		SmallRoute.Add(FWaypoint(HighPriorityNPC.SpawnLocation, 1.0f));
		
		FPatrolRoute SmallPatrolRoute;
		SmallPatrolRoute.RouteName = FString::Printf(TEXT("Small Route %d"), i);
		SmallPatrolRoute.Waypoints = SmallRoute;
		SmallPatrolRoute.bLooping = true;
		SmallPatrolRoute.MovementSpeed = 150.0f;
		SmallPatrolRoute.ActivationDistance = 1500.0f;
		
		HighPriorityNPC.PatrolRoute = SmallPatrolRoute;
		HighPriorityNPC.Priority = 50 - i; // Decreasing priority
		HighPriorityNPC.bEssential = false;
		SpawnData.Add(HighPriorityNPC);
	}

	// Medium priority NPCs
	for (int32 i = 0; i < 4; i++)
	{
		FNPCSpawnData MediumPriorityNPC;
		MediumPriorityNPC.NPCClass = ExampleNPCClass;
		MediumPriorityNPC.SpawnLocation = BaseLocation + FVector(0, i * 200, 0);
		MediumPriorityNPC.SpawnRotation = FRotator(0, (i + 1) * 90, 0);
		
		// Static NPCs (no patrol)
		MediumPriorityNPC.PatrolRoute = FPatrolRoute(); // Empty route
		MediumPriorityNPC.Priority = 25 - i;
		MediumPriorityNPC.bEssential = false;
		SpawnData.Add(MediumPriorityNPC);
	}

	// Low priority NPCs
	for (int32 i = 0; i < 6; i++)
	{
		FNPCSpawnData LowPriorityNPC;
		LowPriorityNPC.NPCClass = ExampleNPCClass;
		LowPriorityNPC.SpawnLocation = BaseLocation + FVector(FMath::RandRange(-1000, 1000), FMath::RandRange(-1000, 1000), 0);
		LowPriorityNPC.SpawnRotation = FRotator(0, FMath::RandRange(0, 360), 0);
		LowPriorityNPC.PatrolRoute = FPatrolRoute(); // No patrol
		LowPriorityNPC.Priority = 10 - i;
		LowPriorityNPC.bEssential = false;
		SpawnData.Add(LowPriorityNPC);
	}

	// Set the spawn data on the manager
	NPCSpawnManager->NPCSpawnData = SpawnData;

	// Initialize the system
	NPCSpawnManager->InitializeNPCSystem();

	UE_LOG(LogTemp, Warning, TEXT("Example NPC system setup complete with %d spawn points"), SpawnData.Num());
}

void ANPCSpawnManagerExample::CreatePatrolRoute(const FString& RouteName, const TArray<FVector>& WaypointLocations)
{
	if (!NPCSpawnManager)
		return;

	TArray<FWaypoint> Waypoints;
	for (const FVector& Location : WaypointLocations)
	{
		Waypoints.Add(FWaypoint(Location, 1.0f, true));
	}

	FPatrolRoute NewRoute;
	NewRoute.RouteName = RouteName;
	NewRoute.Waypoints = Waypoints;
	NewRoute.bLooping = true;
	NewRoute.MovementSpeed = 200.0f;
	NewRoute.ActivationDistance = 2000.0f;

	UE_LOG(LogTemp, Log, TEXT("Created patrol route '%s' with %d waypoints"), *RouteName, Waypoints.Num());
}

void ANPCSpawnManagerExample::SpawnTestNPCs()
{
	if (!NPCSpawnManager)
	{
		UE_LOG(LogTemp, Error, TEXT("NPCSpawnManager is null"));
		return;
	}

	// Trigger optimization to spawn NPCs based on current performance tier
	NPCSpawnManager->OptimizeNPCCount();
	
	UE_LOG(LogTemp, Warning, TEXT("Test NPCs spawned. Active NPCs: %d"), 
		NPCSpawnManager->GetActiveNPCCount());
}

void ANPCSpawnManagerExample::ClearAllNPCs()
{
	if (!NPCSpawnManager)
		return;

	NPCSpawnManager->ShutdownNPCSystem();
	UE_LOG(LogTemp, Warning, TEXT("All NPCs cleared"));
}

void ANPCSpawnManagerExample::ToggleDebugDisplay()
{
	bDebugDisplayEnabled = !bDebugDisplayEnabled;
	
	if (NPCSpawnManager)
	{
		if (bDebugDisplayEnabled)
		{
			NPCSpawnManager->DebugDrawPatrolRoutes();
			NPCSpawnManager->DebugDrawNPCStates();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Debug display %s"), bDebugDisplayEnabled ? TEXT("enabled") : TEXT("disabled"));
}

void ANPCSpawnManagerExample::PrintNPCSystemStats()
{
	if (!NPCSpawnManager)
	{
		UE_LOG(LogTemp, Error, TEXT("NPCSpawnManager is null"));
		return;
	}

	NPCSpawnManager->LogNPCSystemState();
	
	// Additional stats
	TArray<AOdysseyCharacter*> ActiveNPCs = NPCSpawnManager->GetActiveNPCs();
	UE_LOG(LogTemp, Log, TEXT("Active NPC Actors: %d"), ActiveNPCs.Num());

	for (int32 i = 0; i < ActiveNPCs.Num(); i++)
	{
		if (ActiveNPCs[i] && IsValid(ActiveNPCs[i]))
		{
			FVector NPCLocation = ActiveNPCs[i]->GetActorLocation();
			UE_LOG(LogTemp, Log, TEXT("NPC %d: Location = %s"), i, *NPCLocation.ToString());
		}
	}
}
