#include "NPCSpawnManager.h"
#include "OdysseyCharacter.h"
#include "OdysseyMobileOptimizer.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "DrawDebugHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "AIController.h"

ANPCSpawnManager::ANPCSpawnManager()
{
	PrimaryActorTick.bCanEverTick = true;

	// Pool settings
	MaxPoolSize = 20;
	InitialPoolSize = 12;
	ActiveNPCCount = 0;
	bInitialized = false;

	// Update intervals
	DistanceCheckInterval = 2.0f;
	PatrolUpdateInterval = 0.1f;

	// Timers
	DistanceCheckTimer = 0.0f;
	PatrolUpdateTimer = 0.0f;
	CurrentUpdateIndex = 0;
	LastUpdateTime = 0.0f;

	// Performance tier defaults - matching the requirements
	HighPerformanceLimits = FNPCPerformanceLimits(12, 0.05f, 4000.0f, true);
	MediumPerformanceLimits = FNPCPerformanceLimits(8, 0.1f, 3000.0f, true);
	LowPerformanceLimits = FNPCPerformanceLimits(4, 0.2f, 2000.0f, false);

	CurrentPerformanceTier = EPerformanceTier::Medium;
	CurrentLimits = MediumPerformanceLimits;

	MobileOptimizer = nullptr;
}

void ANPCSpawnManager::BeginPlay()
{
	Super::BeginPlay();

	// Find mobile optimizer in the world
	if (UWorld* World = GetWorld())
	{
		if (AActor* GameMode = UGameplayStatics::GetGameMode(World))
		{
			MobileOptimizer = GameMode->FindComponentByClass<UOdysseyMobileOptimizer>();
		}

		// If not found on game mode, check player controller
		if (!MobileOptimizer)
		{
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
			{
				MobileOptimizer = PC->FindComponentByClass<UOdysseyMobileOptimizer>();
			}
		}
	}

	// Initialize the NPC system
	InitializeNPCSystem();

	UE_LOG(LogTemp, Warning, TEXT("NPC Spawn Manager initialized with %d spawn points"), NPCSpawnData.Num());
}

void ANPCSpawnManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bInitialized)
		return;

	// Use staggered updates to reduce performance impact
	StaggeredUpdate(DeltaTime);

	// Update performance settings if needed
	if (MobileOptimizer)
	{
		EPerformanceTier NewTier = MobileOptimizer->GetCurrentPerformanceTier();
		if (NewTier != CurrentPerformanceTier)
		{
			OnPerformanceTierChanged(NewTier);
		}
	}

	LastUpdateTime += DeltaTime;
}

void ANPCSpawnManager::InitializeNPCSystem()
{
	if (bInitialized)
		return;

	// Initialize performance limits
	InitializePerformanceLimits();

	// Sort spawn data by priority
	SortNPCSpawnDataByPriority();

	// Initialize object pool
	InitializeNPCPool();

	// Spawn initial NPCs based on performance tier
	OptimizeNPCCount();

	bInitialized = true;

	UE_LOG(LogTemp, Log, TEXT("NPC System initialized: Pool size=%d, Performance tier=%d"), 
		NPCPool.Num(), (int32)CurrentPerformanceTier);
}

void ANPCSpawnManager::ShutdownNPCSystem()
{
	// Cleanup all NPCs in pool
	for (FNPCPoolEntry& Entry : NPCPool)
	{
		if (Entry.NPCActor && IsValid(Entry.NPCActor))
		{
			Entry.NPCActor->Destroy();
		}
	}

	NPCPool.Empty();
	ActiveNPCCount = 0;
	bInitialized = false;

	UE_LOG(LogTemp, Log, TEXT("NPC System shutdown complete"));
}

void ANPCSpawnManager::InitializeNPCPool()
{
	// Clear existing pool
	NPCPool.Empty();

	// Pre-allocate pool entries
	NPCPool.Reserve(MaxPoolSize);

	// Create initial pool
	for (int32 i = 0; i < InitialPoolSize; i++)
	{
		FNPCPoolEntry NewEntry;
		NPCPool.Add(NewEntry);
	}

	UE_LOG(LogTemp, Verbose, TEXT("NPC Pool initialized with %d entries"), InitialPoolSize);
}

FNPCPoolEntry* ANPCSpawnManager::GetPooledNPC()
{
	// Find an available pool entry
	for (int32 i = 0; i < NPCPool.Num(); i++)
	{
		if (!NPCPool[i].bInUse)
		{
			NPCPool[i].bInUse = true;
			return &NPCPool[i];
		}
	}

	// Expand pool if needed and possible
	if (NPCPool.Num() < MaxPoolSize)
	{
		ExpandPool(1);
		int32 NewIndex = NPCPool.Num() - 1;
		NPCPool[NewIndex].bInUse = true;
		return &NPCPool[NewIndex];
	}

	// Pool is full
	UE_LOG(LogTemp, Warning, TEXT("NPC Pool is full! Cannot allocate new NPC."));
	return nullptr;
}

void ANPCSpawnManager::ReturnNPCToPool(int32 PoolIndex)
{
	if (!NPCPool.IsValidIndex(PoolIndex))
		return;

	FNPCPoolEntry& Entry = NPCPool[PoolIndex];

	// Deactivate and hide the NPC
	if (Entry.NPCActor && IsValid(Entry.NPCActor))
	{
		Entry.NPCActor->SetActorHiddenInGame(true);
		Entry.NPCActor->SetActorEnableCollision(false);
		Entry.NPCActor->SetActorTickEnabled(false);
	}

	// Reset pool entry
	ResetNPCPoolEntry(PoolIndex);

	if (Entry.bActive)
	{
		ActiveNPCCount--;
	}

	Entry.bInUse = false;
	Entry.bActive = false;

	UE_LOG(LogTemp, Verbose, TEXT("Returned NPC to pool at index %d"), PoolIndex);
}

void ANPCSpawnManager::ExpandPool(int32 AdditionalSize)
{
	int32 NewSize = FMath::Min(NPCPool.Num() + AdditionalSize, MaxPoolSize);
	int32 ActualIncrease = NewSize - NPCPool.Num();

	for (int32 i = 0; i < ActualIncrease; i++)
	{
		FNPCPoolEntry NewEntry;
		NPCPool.Add(NewEntry);
	}

	UE_LOG(LogTemp, Verbose, TEXT("Expanded NPC pool by %d entries (total: %d)"), 
		ActualIncrease, NPCPool.Num());
}

bool ANPCSpawnManager::SpawnNPC(int32 SpawnDataIndex)
{
	if (!NPCSpawnData.IsValidIndex(SpawnDataIndex))
		return false;

	if (!CanSpawnMoreNPCs())
		return false;

	const FNPCSpawnData& SpawnData = NPCSpawnData[SpawnDataIndex];
	if (!SpawnData.NPCClass)
		return false;

	// Get pooled NPC
	FNPCPoolEntry* PoolEntry = GetPooledNPC();
	if (!PoolEntry)
		return false;

	// Create or reuse NPC actor
	if (!PoolEntry->NPCActor || !IsValid(PoolEntry->NPCActor))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		
		PoolEntry->NPCActor = GetWorld()->SpawnActor<AOdysseyCharacter>(
			SpawnData.NPCClass,
			SpawnData.SpawnLocation,
			SpawnData.SpawnRotation,
			SpawnParams
		);

		if (!PoolEntry->NPCActor)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to spawn NPC actor for spawn data index %d"), SpawnDataIndex);
			PoolEntry->bInUse = false;
			return false;
		}
	}
	else
	{
		// Reuse existing actor
		PoolEntry->NPCActor->SetActorLocation(SpawnData.SpawnLocation);
		PoolEntry->NPCActor->SetActorRotation(SpawnData.SpawnRotation);
	}

	// Setup pool entry
	PoolEntry->SpawnDataIndex = SpawnDataIndex;
	PoolEntry->CurrentWaypointIndex = 0;
	PoolEntry->bMovingToWaypoint = false;
	PoolEntry->bWaitingAtWaypoint = false;
	PoolEntry->MoveStartTime = 0.0f;
	PoolEntry->WaitStartTime = 0.0f;

	// Activate the NPC
	int32 PoolIndex = NPCPool.IndexOfByPredicate([PoolEntry](const FNPCPoolEntry& Entry) {
		return &Entry == PoolEntry;
	});

	ActivateNPC(PoolIndex);

	// Start patrol if available
	if (SpawnData.PatrolRoute.Waypoints.Num() > 0 && CurrentLimits.bEnablePatrolling)
	{
		PoolEntry->bMovingToWaypoint = true;
		PoolEntry->MoveStartTime = GetWorld()->GetTimeSeconds();
	}

	OnNPCSpawned(PoolEntry->NPCActor, SpawnDataIndex);

	UE_LOG(LogTemp, Verbose, TEXT("Spawned NPC at index %d from spawn data %d"), 
		PoolIndex, SpawnDataIndex);

	return true;
}

void ANPCSpawnManager::DespawnNPC(int32 PoolIndex)
{
	if (!NPCPool.IsValidIndex(PoolIndex))
		return;

	FNPCPoolEntry& Entry = NPCPool[PoolIndex];
	if (!Entry.bInUse)
		return;

	OnNPCDespawned(Entry.NPCActor, Entry.SpawnDataIndex);

	ReturnNPCToPool(PoolIndex);

	UE_LOG(LogTemp, Verbose, TEXT("Despawned NPC at pool index %d"), PoolIndex);
}

void ANPCSpawnManager::ActivateNPC(int32 PoolIndex)
{
	if (!NPCPool.IsValidIndex(PoolIndex))
		return;

	FNPCPoolEntry& Entry = NPCPool[PoolIndex];
	if (!Entry.bInUse || Entry.bActive)
		return;

	if (Entry.NPCActor && IsValid(Entry.NPCActor))
	{
		Entry.NPCActor->SetActorHiddenInGame(false);
		Entry.NPCActor->SetActorEnableCollision(true);
		Entry.NPCActor->SetActorTickEnabled(true);

		Entry.bActive = true;
		ActiveNPCCount++;

		OnNPCActivated(Entry.NPCActor);

		UE_LOG(LogTemp, Verbose, TEXT("Activated NPC at pool index %d (Active count: %d)"), 
			PoolIndex, ActiveNPCCount);
	}
}

void ANPCSpawnManager::DeactivateNPC(int32 PoolIndex)
{
	if (!NPCPool.IsValidIndex(PoolIndex))
		return;

	FNPCPoolEntry& Entry = NPCPool[PoolIndex];
	if (!Entry.bActive)
		return;

	if (Entry.NPCActor && IsValid(Entry.NPCActor))
	{
		Entry.NPCActor->SetActorHiddenInGame(true);
		Entry.NPCActor->SetActorEnableCollision(false);
		Entry.NPCActor->SetActorTickEnabled(false);

		Entry.bActive = false;
		ActiveNPCCount--;

		OnNPCDeactivated(Entry.NPCActor);

		UE_LOG(LogTemp, Verbose, TEXT("Deactivated NPC at pool index %d (Active count: %d)"), 
			PoolIndex, ActiveNPCCount);
	}
}

void ANPCSpawnManager::UpdatePerformanceSettings()
{
	if (!MobileOptimizer)
		return;

	EPerformanceTier NewTier = MobileOptimizer->GetCurrentPerformanceTier();
	if (NewTier != CurrentPerformanceTier)
	{
		OnPerformanceTierChanged(NewTier);
	}
}

void ANPCSpawnManager::OnPerformanceTierChanged(EPerformanceTier NewTier)
{
	CurrentPerformanceTier = NewTier;
	CurrentLimits = GetLimitsForTier(NewTier);

	UE_LOG(LogTemp, Warning, TEXT("Performance tier changed to %d, Max NPCs: %d"), 
		(int32)NewTier, CurrentLimits.MaxNPCs);

	// Optimize NPC count for new tier
	OptimizeNPCCount();

	OnPerformanceOptimized(NewTier, ActiveNPCCount);
}

void ANPCSpawnManager::OptimizeNPCCount()
{
	int32 TargetNPCs = GetMaxNPCsForCurrentTier();

	// If we have too many active NPCs, deactivate some
	while (ActiveNPCCount > TargetNPCs)
	{
		bool bFound = false;
		// Deactivate non-essential NPCs first
		for (int32 i = NPCPool.Num() - 1; i >= 0; i--)
		{
			FNPCPoolEntry& Entry = NPCPool[i];
			if (Entry.bActive && Entry.bInUse && NPCSpawnData.IsValidIndex(Entry.SpawnDataIndex))
			{
				const FNPCSpawnData& SpawnData = NPCSpawnData[Entry.SpawnDataIndex];
				if (!SpawnData.bEssential)
				{
					DeactivateNPC(i);
					bFound = true;
					break;
				}
			}
		}

		if (!bFound)
			break; // All remaining NPCs are essential
	}

	// If we can spawn more NPCs, do so
	while (ActiveNPCCount < TargetNPCs)
	{
		bool bSpawned = false;
		// Try to spawn NPCs in priority order
		for (int32 i = 0; i < NPCSpawnData.Num(); i++)
		{
			// Check if this spawn point is already in use
			bool bAlreadySpawned = false;
			for (const FNPCPoolEntry& Entry : NPCPool)
			{
				if (Entry.bInUse && Entry.SpawnDataIndex == i)
				{
					if (!Entry.bActive)
					{
						// Reactivate existing NPC
						int32 PoolIndex = NPCPool.IndexOfByPredicate([&Entry](const FNPCPoolEntry& E) {
							return &E == &Entry;
						});
						ActivateNPC(PoolIndex);
						bSpawned = true;
					}
					bAlreadySpawned = true;
					break;
				}
			}

			if (!bAlreadySpawned)
			{
				if (SpawnNPC(i))
				{
					bSpawned = true;
					break;
				}
			}

			if (bSpawned)
				break;
		}

		if (!bSpawned)
			break; // No more NPCs can be spawned
	}

	UE_LOG(LogTemp, Log, TEXT("NPC count optimized: %d/%d active NPCs"), 
		ActiveNPCCount, TargetNPCs);
}

int32 ANPCSpawnManager::GetMaxNPCsForCurrentTier() const
{
	return CurrentLimits.MaxNPCs;
}

void ANPCSpawnManager::UpdateNPCDistances()
{
	APawn* PlayerPawn = GetPlayerPawn();
	if (!PlayerPawn)
		return;

	FVector PlayerLocation = PlayerPawn->GetActorLocation();

	// Update activation status based on distance
	for (int32 i = 0; i < NPCPool.Num(); i++)
	{
		FNPCPoolEntry& Entry = NPCPool[i];
		if (!Entry.bInUse || !Entry.NPCActor || !IsValid(Entry.NPCActor))
			continue;

		float Distance = FVector::Dist(PlayerLocation, Entry.NPCActor->GetActorLocation());

		// Check if NPC should be activated or deactivated based on distance
		bool bShouldBeActive = Distance <= CurrentLimits.CullingDistance;

		// For essential NPCs or if we're under the limit, activate regardless of distance
		if (NPCSpawnData.IsValidIndex(Entry.SpawnDataIndex))
		{
			const FNPCSpawnData& SpawnData = NPCSpawnData[Entry.SpawnDataIndex];
			if (SpawnData.bEssential || ActiveNPCCount < CurrentLimits.MaxNPCs)
			{
				bShouldBeActive = true;
			}
		}

		if (bShouldBeActive && !Entry.bActive)
		{
			if (ActiveNPCCount < CurrentLimits.MaxNPCs)
			{
				ActivateNPC(i);
			}
		}
		else if (!bShouldBeActive && Entry.bActive)
		{
			// Don't deactivate essential NPCs
			if (NPCSpawnData.IsValidIndex(Entry.SpawnDataIndex))
			{
				const FNPCSpawnData& SpawnData = NPCSpawnData[Entry.SpawnDataIndex];
				if (!SpawnData.bEssential)
				{
					DeactivateNPC(i);
				}
			}
			else
			{
				DeactivateNPC(i);
			}
		}
	}
}

void ANPCSpawnManager::CullDistantNPCs()
{
	APawn* PlayerPawn = GetPlayerPawn();
	if (!PlayerPawn)
		return;

	FVector PlayerLocation = PlayerPawn->GetActorLocation();

	for (int32 i = 0; i < NPCPool.Num(); i++)
	{
		FNPCPoolEntry& Entry = NPCPool[i];
		if (!Entry.bActive || !Entry.NPCActor || !IsValid(Entry.NPCActor))
			continue;

		float Distance = FVector::Dist(PlayerLocation, Entry.NPCActor->GetActorLocation());
		
		if (Distance > CurrentLimits.CullingDistance)
		{
			// Don't cull essential NPCs
			if (NPCSpawnData.IsValidIndex(Entry.SpawnDataIndex))
			{
				const FNPCSpawnData& SpawnData = NPCSpawnData[Entry.SpawnDataIndex];
				if (!SpawnData.bEssential)
				{
					DeactivateNPC(i);
				}
			}
			else
			{
				DeactivateNPC(i);
			}
		}
	}
}

void ANPCSpawnManager::ActivateNearbyNPCs()
{
	if (ActiveNPCCount >= CurrentLimits.MaxNPCs)
		return;

	APawn* PlayerPawn = GetPlayerPawn();
	if (!PlayerPawn)
		return;

	FVector PlayerLocation = PlayerPawn->GetActorLocation();

	// Find inactive NPCs within activation distance
	TArray<TPair<float, int32>> NearbyNPCs;

	for (int32 i = 0; i < NPCPool.Num(); i++)
	{
		FNPCPoolEntry& Entry = NPCPool[i];
		if (Entry.bInUse && !Entry.bActive && Entry.NPCActor && IsValid(Entry.NPCActor))
		{
			float Distance = FVector::Dist(PlayerLocation, Entry.NPCActor->GetActorLocation());
			if (Distance <= CurrentLimits.CullingDistance)
			{
				NearbyNPCs.Add(TPair<float, int32>(Distance, i));
			}
		}
	}

	// Sort by distance (closest first)
	NearbyNPCs.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& B) {
		return A.Key < B.Key;
	});

	// Activate closest NPCs up to the limit
	for (const auto& NPCPair : NearbyNPCs)
	{
		if (ActiveNPCCount >= CurrentLimits.MaxNPCs)
			break;

		ActivateNPC(NPCPair.Value);
	}
}

float ANPCSpawnManager::GetDistanceToPlayer(const FVector& NPCLocation) const
{
	APawn* PlayerPawn = GetPlayerPawn();
	if (!PlayerPawn)
		return FLT_MAX;

	return FVector::Dist(PlayerPawn->GetActorLocation(), NPCLocation);
}

void ANPCSpawnManager::UpdateNPCPatrols(float DeltaTime)
{
	if (!CurrentLimits.bEnablePatrolling)
		return;

	// Stagger patrol updates to reduce performance impact
	int32 NPCsPerUpdate = FMath::Max(1, NPCPool.Num() / 5); // Update 20% of NPCs per frame
	int32 EndIndex = FMath::Min(CurrentUpdateIndex + NPCsPerUpdate, NPCPool.Num());

	for (int32 i = CurrentUpdateIndex; i < EndIndex; i++)
	{
		if (NPCPool[i].bActive && NPCPool[i].bInUse)
		{
			UpdateNPCPatrol(i, DeltaTime);
		}
	}

	CurrentUpdateIndex = (EndIndex >= NPCPool.Num()) ? 0 : EndIndex;
}

void ANPCSpawnManager::UpdateNPCPatrol(int32 PoolIndex, float DeltaTime)
{
	if (!NPCPool.IsValidIndex(PoolIndex))
		return;

	FNPCPoolEntry& Entry = NPCPool[PoolIndex];
	if (!Entry.bActive || !Entry.bInUse || !NPCSpawnData.IsValidIndex(Entry.SpawnDataIndex))
		return;

	const FNPCSpawnData& SpawnData = NPCSpawnData[Entry.SpawnDataIndex];
	const FPatrolRoute& Route = SpawnData.PatrolRoute;

	if (Route.Waypoints.Num() == 0)
		return;

	float CurrentTime = GetWorld()->GetTimeSeconds();
	FWaypoint CurrentWaypoint = Route.Waypoints[Entry.CurrentWaypointIndex];

	// Handle waiting at waypoint
	if (Entry.bWaitingAtWaypoint)
	{
		if (CurrentTime - Entry.WaitStartTime >= CurrentWaypoint.WaitTime)
		{
			Entry.bWaitingAtWaypoint = false;
			Entry.bMovingToWaypoint = true;
			Entry.MoveStartTime = CurrentTime;
			
			// Move to next waypoint
			Entry.CurrentWaypointIndex = (Entry.CurrentWaypointIndex + 1) % Route.Waypoints.Num();
			if (!Route.bLooping && Entry.CurrentWaypointIndex == 0)
			{
				// Reached end of non-looping route
				Entry.bMovingToWaypoint = false;
				return;
			}
		}
		return;
	}

	// Handle movement to waypoint
	if (Entry.bMovingToWaypoint)
	{
		AOdysseyCharacter* NPC = Entry.NPCActor;
		if (!NPC || !IsValid(NPC))
			return;

		FVector CurrentLocation = NPC->GetActorLocation();
		FVector TargetLocation = CurrentWaypoint.Location;

		// Check if we've reached the waypoint
		if (IsNPCAtWaypoint(PoolIndex, CurrentWaypoint))
		{
			Entry.bMovingToWaypoint = false;
			
			if (CurrentWaypoint.WaitTime > 0.0f)
			{
				Entry.bWaitingAtWaypoint = true;
				Entry.WaitStartTime = CurrentTime;
			}
			else
			{
				// Move to next waypoint immediately
				Entry.CurrentWaypointIndex = (Entry.CurrentWaypointIndex + 1) % Route.Waypoints.Num();
				if (!Route.bLooping && Entry.CurrentWaypointIndex == 0)
				{
					Entry.bMovingToWaypoint = false;
					return;
				}
				Entry.MoveStartTime = CurrentTime;
			}
			return;
		}

		// Move towards waypoint
		FVector Direction = (TargetLocation - CurrentLocation).GetSafeNormal();
		float MovementDistance = Route.MovementSpeed * DeltaTime;
		FVector NewLocation = CurrentLocation + Direction * MovementDistance;

		// Make sure we don't overshoot
		float DistanceToTarget = FVector::Dist(CurrentLocation, TargetLocation);
		if (MovementDistance >= DistanceToTarget)
		{
			NewLocation = TargetLocation;
		}

		NPC->SetActorLocation(NewLocation);

		// Face movement direction
		if (!Direction.IsNearlyZero())
		{
			FRotator NewRotation = Direction.Rotation();
			NPC->SetActorRotation(NewRotation);
		}
	}
}

void ANPCSpawnManager::MoveNPCToWaypoint(int32 PoolIndex, const FWaypoint& Waypoint)
{
	if (!NPCPool.IsValidIndex(PoolIndex))
		return;

	FNPCPoolEntry& Entry = NPCPool[PoolIndex];
	if (!Entry.bActive || !Entry.NPCActor || !IsValid(Entry.NPCActor))
		return;

	Entry.bMovingToWaypoint = true;
	Entry.bWaitingAtWaypoint = false;
	Entry.MoveStartTime = GetWorld()->GetTimeSeconds();
}

bool ANPCSpawnManager::IsNPCAtWaypoint(int32 PoolIndex, const FWaypoint& Waypoint) const
{
	if (!NPCPool.IsValidIndex(PoolIndex))
		return false;

	const FNPCPoolEntry& Entry = NPCPool[PoolIndex];
	if (!Entry.NPCActor || !IsValid(Entry.NPCActor))
		return false;

	float Distance = FVector::Dist(Entry.NPCActor->GetActorLocation(), Waypoint.Location);
	return Distance <= 50.0f; // 50 unit tolerance
}

FWaypoint ANPCSpawnManager::GetNextWaypoint(int32 PoolIndex) const
{
	if (!NPCPool.IsValidIndex(PoolIndex))
		return FWaypoint();

	const FNPCPoolEntry& Entry = NPCPool[PoolIndex];
	if (!NPCSpawnData.IsValidIndex(Entry.SpawnDataIndex))
		return FWaypoint();

	const FPatrolRoute& Route = NPCSpawnData[Entry.SpawnDataIndex].PatrolRoute;
	if (Route.Waypoints.Num() == 0)
		return FWaypoint();

	int32 NextIndex = (Entry.CurrentWaypointIndex + 1) % Route.Waypoints.Num();
	return Route.Waypoints[NextIndex];
}

bool ANPCSpawnManager::IsNPCActive(int32 PoolIndex) const
{
	if (!NPCPool.IsValidIndex(PoolIndex))
		return false;

	return NPCPool[PoolIndex].bActive;
}

AOdysseyCharacter* ANPCSpawnManager::GetNPCFromPool(int32 PoolIndex) const
{
	if (!NPCPool.IsValidIndex(PoolIndex))
		return nullptr;

	return NPCPool[PoolIndex].NPCActor;
}

TArray<AOdysseyCharacter*> ANPCSpawnManager::GetActiveNPCs() const
{
	TArray<AOdysseyCharacter*> ActiveNPCs;

	for (const FNPCPoolEntry& Entry : NPCPool)
	{
		if (Entry.bActive && Entry.NPCActor && IsValid(Entry.NPCActor))
		{
			ActiveNPCs.Add(Entry.NPCActor);
		}
	}

	return ActiveNPCs;
}

void ANPCSpawnManager::DebugDrawPatrolRoutes()
{
	if (!GetWorld())
		return;

	for (int32 i = 0; i < NPCSpawnData.Num(); i++)
	{
		const FPatrolRoute& Route = NPCSpawnData[i].PatrolRoute;
		
		for (int32 j = 0; j < Route.Waypoints.Num(); j++)
		{
			const FWaypoint& Waypoint = Route.Waypoints[j];
			
			// Draw waypoint sphere
			DrawDebugSphere(GetWorld(), Waypoint.Location, 25.0f, 8, FColor::Yellow, false, -1.0f, 0, 2.0f);
			
			// Draw line to next waypoint
			if (j < Route.Waypoints.Num() - 1 || Route.bLooping)
			{
				int32 NextIndex = (j + 1) % Route.Waypoints.Num();
				DrawDebugLine(GetWorld(), Waypoint.Location, Route.Waypoints[NextIndex].Location, 
					FColor::Green, false, -1.0f, 0, 1.0f);
			}
		}
	}
}

void ANPCSpawnManager::DebugDrawNPCStates()
{
	if (!GetWorld())
		return;

	for (int32 i = 0; i < NPCPool.Num(); i++)
	{
		const FNPCPoolEntry& Entry = NPCPool[i];
		if (!Entry.bInUse || !Entry.NPCActor || !IsValid(Entry.NPCActor))
			continue;

		FVector NPCLocation = Entry.NPCActor->GetActorLocation();
		FColor StateColor = Entry.bActive ? FColor::Green : FColor::Red;

		// Draw state indicator
		DrawDebugSphere(GetWorld(), NPCLocation + FVector(0, 0, 100), 15.0f, 8, StateColor, false, -1.0f, 0, 2.0f);

		// Draw distance to player
		float Distance = GetDistanceToPlayer(NPCLocation);
		FString DebugText = FString::Printf(TEXT("Pool:%d Dist:%.0f %s"), i, Distance, 
			Entry.bActive ? TEXT("ACTIVE") : TEXT("INACTIVE"));
		
		DrawDebugString(GetWorld(), NPCLocation + FVector(0, 0, 120), DebugText, nullptr, StateColor, 0.0f);
	}
}

void ANPCSpawnManager::LogNPCSystemState()
{
	UE_LOG(LogTemp, Log, TEXT("=== NPC System State ==="));
	UE_LOG(LogTemp, Log, TEXT("Performance Tier: %d"), (int32)CurrentPerformanceTier);
	UE_LOG(LogTemp, Log, TEXT("Active NPCs: %d/%d"), ActiveNPCCount, CurrentLimits.MaxNPCs);
	UE_LOG(LogTemp, Log, TEXT("Pool Size: %d/%d"), NPCPool.Num(), MaxPoolSize);
	UE_LOG(LogTemp, Log, TEXT("Culling Distance: %.0f"), CurrentLimits.CullingDistance);
	UE_LOG(LogTemp, Log, TEXT("Patrolling Enabled: %s"), CurrentLimits.bEnablePatrolling ? TEXT("Yes") : TEXT("No"));
	
	int32 InUseCount = 0;
	for (const FNPCPoolEntry& Entry : NPCPool)
	{
		if (Entry.bInUse)
			InUseCount++;
	}
	
	UE_LOG(LogTemp, Log, TEXT("Pool Entries In Use: %d"), InUseCount);
	UE_LOG(LogTemp, Log, TEXT("========================"));
}

// Private helper functions

void ANPCSpawnManager::InitializePerformanceLimits()
{
	// Performance limits are already initialized in constructor
	// This function could be used to load settings from config files if needed
	CurrentLimits = GetLimitsForTier(CurrentPerformanceTier);
}

FNPCPerformanceLimits ANPCSpawnManager::GetLimitsForTier(EPerformanceTier Tier) const
{
	switch (Tier)
	{
		case EPerformanceTier::High:
			return HighPerformanceLimits;
		case EPerformanceTier::Medium:
			return MediumPerformanceLimits;
		case EPerformanceTier::Low:
		default:
			return LowPerformanceLimits;
	}
}

bool ANPCSpawnManager::CanSpawnMoreNPCs() const
{
	return ActiveNPCCount < CurrentLimits.MaxNPCs;
}

void ANPCSpawnManager::SortNPCSpawnDataByPriority()
{
	NPCSpawnData.Sort([](const FNPCSpawnData& A, const FNPCSpawnData& B) {
		// Essential NPCs first, then by priority (higher priority first)
		if (A.bEssential != B.bEssential)
			return A.bEssential > B.bEssential;
		return A.Priority > B.Priority;
	});
}

int32 ANPCSpawnManager::FindNextAvailablePoolEntry() const
{
	for (int32 i = 0; i < NPCPool.Num(); i++)
	{
		if (!NPCPool[i].bInUse)
			return i;
	}
	return -1;
}

void ANPCSpawnManager::ResetNPCPoolEntry(int32 PoolIndex)
{
	if (!NPCPool.IsValidIndex(PoolIndex))
		return;

	FNPCPoolEntry& Entry = NPCPool[PoolIndex];
	Entry.SpawnDataIndex = -1;
	Entry.CurrentWaypointIndex = 0;
	Entry.MoveStartTime = 0.0f;
	Entry.WaitStartTime = 0.0f;
	Entry.bMovingToWaypoint = false;
	Entry.bWaitingAtWaypoint = false;
}

APawn* ANPCSpawnManager::GetPlayerPawn() const
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
		{
			return PC->GetPawn();
		}
	}
	return nullptr;
}

void ANPCSpawnManager::StaggeredUpdate(float DeltaTime)
{
	// Distance checks (every 2 seconds)
	DistanceCheckTimer += DeltaTime;
	if (DistanceCheckTimer >= DistanceCheckInterval)
	{
		UpdateNPCDistances();
		DistanceCheckTimer = 0.0f;
	}

	// Patrol updates (every update interval based on performance)
	PatrolUpdateTimer += DeltaTime;
	if (PatrolUpdateTimer >= CurrentLimits.UpdateFrequency)
	{
		UpdateNPCPatrols(DeltaTime);
		PatrolUpdateTimer = 0.0f;
	}

	// Validate pool occasionally (every 5 seconds)
	static float ValidationTimer = 0.0f;
	ValidationTimer += DeltaTime;
	if (ValidationTimer >= 5.0f)
	{
		ValidateNPCPool();
		ValidationTimer = 0.0f;
	}
}

bool ANPCSpawnManager::IsWithinActivationDistance(const FVector& NPCLocation) const
{
	return GetDistanceToPlayer(NPCLocation) <= CurrentLimits.CullingDistance;
}

void ANPCSpawnManager::ValidateNPCPool()
{
	// Remove invalid entries and fix inconsistencies
	int32 ActualActiveCount = 0;

	for (int32 i = 0; i < NPCPool.Num(); i++)
	{
		FNPCPoolEntry& Entry = NPCPool[i];
		
		// Check if actor is still valid
		if (Entry.bInUse && (!Entry.NPCActor || !IsValid(Entry.NPCActor)))
		{
			UE_LOG(LogTemp, Warning, TEXT("Found invalid NPC actor in pool at index %d, cleaning up"), i);
			ResetNPCPoolEntry(i);
			Entry.bInUse = false;
			Entry.bActive = false;
			continue;
		}

		// Count actual active NPCs
		if (Entry.bActive)
		{
			ActualActiveCount++;
		}
	}

	// Fix active count if it's wrong
	if (ActualActiveCount != ActiveNPCCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("Active NPC count mismatch! Expected: %d, Actual: %d. Fixing..."), 
			ActiveNPCCount, ActualActiveCount);
		ActiveNPCCount = ActualActiveCount;
	}
}
