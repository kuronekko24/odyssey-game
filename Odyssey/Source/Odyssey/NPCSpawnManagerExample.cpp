#include "NPCSpawnManagerExample.h"
#include "OdysseyCharacter.h"
#include "NPCShip.h"
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

	// Create NPC Spawn Manager
	if (UWorld* World = GetWorld())
	{
		NPCSpawnManager = World->SpawnActor<ANPCSpawnManager>();
		if (NPCSpawnManager)
		{
			UE_LOG(LogTemp, Log, TEXT("NPCSpawnManagerExample: Manager created"));

			FTimerHandle TimerHandle;
			World->GetTimerManager().SetTimer(TimerHandle, this,
				&ANPCSpawnManagerExample::SetupExampleNPCs, 1.0f, false);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("NPCSpawnManagerExample: Failed to create manager"));
		}
	}
}

void ANPCSpawnManagerExample::SetupExampleNPCs()
{
	if (!NPCSpawnManager || !ExampleNPCClass)
	{
		UE_LOG(LogTemp, Error, TEXT("NPCSpawnManagerExample: Manager or class is null"));
		return;
	}

	FVector BaseLocation = GetActorLocation();

	// Register a shared patrol route
	FPatrolRoute SharedRoute;
	SharedRoute.RouteId = FName("SharedPerimeterRoute");
	SharedRoute.RouteName = TEXT("Perimeter Patrol");
	SharedRoute.bLooping = true;
	SharedRoute.MovementSpeed = 200.0f;
	SharedRoute.ActivationDistance = 2000.0f;
	SharedRoute.Waypoints.Add(FWaypoint(BaseLocation + FVector(500, 0, 0), 2.0f, true));
	SharedRoute.Waypoints.Add(FWaypoint(BaseLocation + FVector(500, 500, 0), 1.0f, false));
	SharedRoute.Waypoints.Add(FWaypoint(BaseLocation + FVector(0, 500, 0), 2.0f, true));
	SharedRoute.Waypoints.Add(FWaypoint(BaseLocation + FVector(-500, 500, 0), 1.0f, false));
	SharedRoute.Waypoints.Add(FWaypoint(BaseLocation + FVector(-500, 0, 0), 2.0f, false));
	SharedRoute.Waypoints.Add(FWaypoint(BaseLocation + FVector(-500, -500, 0), 1.0f, false));
	SharedRoute.Waypoints.Add(FWaypoint(BaseLocation + FVector(0, -500, 0), 2.0f, true));
	SharedRoute.Waypoints.Add(FWaypoint(BaseLocation + FVector(500, -500, 0), 1.0f, false));
	NPCSpawnManager->RegisterPatrolRoute(SharedRoute);

	TArray<FNPCSpawnData> SpawnData;

	// Essential NPC (always active)
	{
		FNPCSpawnData EssentialNPC;
		EssentialNPC.NPCClass = ExampleNPCClass;
		EssentialNPC.SpawnLocation = BaseLocation;
		EssentialNPC.SpawnRotation = FRotator::ZeroRotator;
		EssentialNPC.PatrolRoute = SharedRoute;
		EssentialNPC.Priority = 100;
		EssentialNPC.bEssential = true;
		SpawnData.Add(EssentialNPC);
	}

	// High priority NPCs with small patrol routes
	for (int32 i = 0; i < 3; i++)
	{
		FNPCSpawnData NPC;
		NPC.NPCClass = ExampleNPCClass;
		NPC.SpawnLocation = BaseLocation + FVector(i * 200, 0, 0);
		NPC.SpawnRotation = FRotator(0, i * 90, 0);

		FPatrolRoute SmallRoute;
		SmallRoute.RouteId = FName(*FString::Printf(TEXT("SmallRoute_%d"), i));
		SmallRoute.RouteName = FString::Printf(TEXT("Small Route %d"), i);
		SmallRoute.bLooping = true;
		SmallRoute.MovementSpeed = 150.0f;
		SmallRoute.ActivationDistance = 1500.0f;
		SmallRoute.Waypoints.Add(FWaypoint(NPC.SpawnLocation + FVector(100, 0, 0), 1.0f));
		SmallRoute.Waypoints.Add(FWaypoint(NPC.SpawnLocation + FVector(100, 100, 0), 1.0f));
		SmallRoute.Waypoints.Add(FWaypoint(NPC.SpawnLocation + FVector(0, 100, 0), 1.0f));
		SmallRoute.Waypoints.Add(FWaypoint(NPC.SpawnLocation, 1.0f));

		NPC.PatrolRoute = SmallRoute;
		NPC.Priority = 50 - i;
		NPC.bEssential = false;
		SpawnData.Add(NPC);
	}

	// Medium priority static NPCs
	for (int32 i = 0; i < 4; i++)
	{
		FNPCSpawnData NPC;
		NPC.NPCClass = ExampleNPCClass;
		NPC.SpawnLocation = BaseLocation + FVector(0, i * 200, 0);
		NPC.SpawnRotation = FRotator(0, (i + 1) * 90, 0);
		NPC.PatrolRoute = FPatrolRoute();
		NPC.Priority = 25 - i;
		NPC.bEssential = false;
		SpawnData.Add(NPC);
	}

	// Low priority scattered NPCs
	for (int32 i = 0; i < 6; i++)
	{
		FNPCSpawnData NPC;
		NPC.NPCClass = ExampleNPCClass;
		NPC.SpawnLocation = BaseLocation + FVector(
			FMath::RandRange(-1000, 1000), FMath::RandRange(-1000, 1000), 0);
		NPC.SpawnRotation = FRotator(0, FMath::RandRange(0, 360), 0);
		NPC.PatrolRoute = FPatrolRoute();
		NPC.Priority = 10 - i;
		NPC.bEssential = false;
		SpawnData.Add(NPC);
	}

	NPCSpawnManager->NPCSpawnData = SpawnData;
	NPCSpawnManager->InitializeNPCSystem();

	UE_LOG(LogTemp, Warning, TEXT("NPCSpawnManagerExample: Setup complete with %d spawn points"), SpawnData.Num());
}

void ANPCSpawnManagerExample::CreatePatrolRoute(const FString& RouteName, const TArray<FVector>& WaypointLocations)
{
	if (!NPCSpawnManager)
	{
		return;
	}

	FPatrolRoute NewRoute;
	NewRoute.RouteId = FName(*RouteName);
	NewRoute.RouteName = RouteName;
	NewRoute.bLooping = true;
	NewRoute.MovementSpeed = 200.0f;
	NewRoute.ActivationDistance = 2000.0f;

	for (const FVector& Location : WaypointLocations)
	{
		NewRoute.Waypoints.Add(FWaypoint(Location, 1.0f, true));
	}

	NPCSpawnManager->RegisterPatrolRoute(NewRoute);

	UE_LOG(LogTemp, Log, TEXT("Created patrol route '%s' with %d waypoints"),
		*RouteName, NewRoute.Waypoints.Num());
}

void ANPCSpawnManagerExample::SpawnTestNPCs()
{
	if (!NPCSpawnManager)
	{
		UE_LOG(LogTemp, Error, TEXT("NPCSpawnManager is null"));
		return;
	}

	NPCSpawnManager->OptimizeNPCCount();
	UE_LOG(LogTemp, Warning, TEXT("Test NPCs spawned. Active: %d"), NPCSpawnManager->GetActiveNPCCount());
}

void ANPCSpawnManagerExample::ClearAllNPCs()
{
	if (!NPCSpawnManager)
	{
		return;
	}

	NPCSpawnManager->ShutdownNPCSystem();
	UE_LOG(LogTemp, Warning, TEXT("All NPCs cleared"));
}

void ANPCSpawnManagerExample::ToggleDebugDisplay()
{
	bDebugDisplayEnabled = !bDebugDisplayEnabled;

	if (NPCSpawnManager && bDebugDisplayEnabled)
	{
		NPCSpawnManager->DebugDrawPatrolRoutes();
		NPCSpawnManager->DebugDrawNPCStates();
		NPCSpawnManager->DebugDrawSpatialGrid();
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

	TArray<AOdysseyCharacter*> ActiveNPCs = NPCSpawnManager->GetActiveNPCs();
	UE_LOG(LogTemp, Log, TEXT("Active NPC Actors: %d"), ActiveNPCs.Num());

	for (int32 i = 0; i < ActiveNPCs.Num(); i++)
	{
		if (ActiveNPCs[i] && IsValid(ActiveNPCs[i]))
		{
			int32 PoolIdx = -1;
			for (int32 j = 0; j < NPCSpawnManager->GetPoolSize(); j++)
			{
				if (NPCSpawnManager->GetNPCFromPool(j) == ActiveNPCs[i])
				{
					PoolIdx = j;
					break;
				}
			}

			ENPCBehaviorLOD LOD = (PoolIdx >= 0)
				? NPCSpawnManager->GetNPCBehaviorLOD(PoolIdx)
				: ENPCBehaviorLOD::Dormant;

			FString LODName;
			switch (LOD)
			{
			case ENPCBehaviorLOD::Full:    LODName = TEXT("Full");    break;
			case ENPCBehaviorLOD::Reduced: LODName = TEXT("Reduced"); break;
			case ENPCBehaviorLOD::Minimal: LODName = TEXT("Minimal"); break;
			case ENPCBehaviorLOD::Dormant: LODName = TEXT("Dormant"); break;
			}

			UE_LOG(LogTemp, Log, TEXT("  NPC %d: %s LOD=%s"),
				i, *ActiveNPCs[i]->GetActorLocation().ToString(), *LODName);
		}
	}
}
