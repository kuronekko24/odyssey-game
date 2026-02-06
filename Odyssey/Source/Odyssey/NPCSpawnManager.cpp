// NPCSpawnManager.cpp
// Mobile-optimized NPC lifecycle manager implementation.
// Phase 4: NPC Spawning & Performance (Task #12)

#include "NPCSpawnManager.h"
#include "NPCShip.h"
#include "NPCBehaviorComponent.h"
#include "OdysseyCharacter.h"
#include "OdysseyMobileOptimizer.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "DrawDebugHelpers.h"
#include "Components/StaticMeshComponent.h"

// ============================================================================
// FNPCSpatialGrid
// ============================================================================

void FNPCSpatialGrid::Initialize(float InCellSize)
{
	CellSize = FMath::Max(InCellSize, 100.0f);
	Cells.Empty();
	// Pre-allocate bucket space to avoid rehashing at runtime
	Cells.Reserve(64);
}

void FNPCSpatialGrid::Clear()
{
	for (auto& Pair : Cells)
	{
		Pair.Value.Empty();
	}
	Cells.Empty();
}

void FNPCSpatialGrid::Insert(int32 PoolIndex, const FVector& WorldLocation)
{
	int32 CX, CY;
	WorldToCell(WorldLocation, CX, CY);
	uint64 Key = CellKey(CX, CY);
	TArray<int32>& Cell = Cells.FindOrAdd(Key);
	Cell.AddUnique(PoolIndex);
}

void FNPCSpatialGrid::Remove(int32 PoolIndex, int32 CellX, int32 CellY)
{
	uint64 Key = CellKey(CellX, CellY);
	if (TArray<int32>* Cell = Cells.Find(Key))
	{
		Cell->Remove(PoolIndex);
		if (Cell->Num() == 0)
		{
			Cells.Remove(Key);
		}
	}
}

void FNPCSpatialGrid::Move(int32 PoolIndex, int32 OldCellX, int32 OldCellY,
	const FVector& NewWorldLocation, int32& OutNewCellX, int32& OutNewCellY)
{
	WorldToCell(NewWorldLocation, OutNewCellX, OutNewCellY);

	// Only update grid if the cell changed
	if (OutNewCellX != OldCellX || OutNewCellY != OldCellY)
	{
		Remove(PoolIndex, OldCellX, OldCellY);
		uint64 NewKey = CellKey(OutNewCellX, OutNewCellY);
		TArray<int32>& Cell = Cells.FindOrAdd(NewKey);
		Cell.AddUnique(PoolIndex);
	}
}

void FNPCSpatialGrid::QueryRadius(const FVector& Center, float Radius, TArray<int32>& OutIndices) const
{
	OutIndices.Reset();

	int32 MinCX, MinCY, MaxCX, MaxCY;
	WorldToCell(Center - FVector(Radius, Radius, 0.0f), MinCX, MinCY);
	WorldToCell(Center + FVector(Radius, Radius, 0.0f), MaxCX, MaxCY);

	for (int32 CX = MinCX; CX <= MaxCX; CX++)
	{
		for (int32 CY = MinCY; CY <= MaxCY; CY++)
		{
			uint64 Key = CellKey(CX, CY);
			if (const TArray<int32>* Cell = Cells.Find(Key))
			{
				for (int32 Idx : *Cell)
				{
					OutIndices.AddUnique(Idx);
				}
			}
		}
	}
}

void FNPCSpatialGrid::WorldToCell(const FVector& WorldLocation, int32& OutCellX, int32& OutCellY) const
{
	OutCellX = FMath::FloorToInt(WorldLocation.X / CellSize);
	OutCellY = FMath::FloorToInt(WorldLocation.Y / CellSize);
}

uint64 FNPCSpatialGrid::CellKey(int32 CellX, int32 CellY)
{
	// Pack two int32s into a uint64 for hash map key
	return (static_cast<uint64>(static_cast<uint32>(CellX)) << 32) | static_cast<uint64>(static_cast<uint32>(CellY));
}

// ============================================================================
// ANPCSpawnManager - Constructor
// ============================================================================

ANPCSpawnManager::ANPCSpawnManager()
{
	PrimaryActorTick.bCanEverTick = true;

	// Pool sizing
	MaxPoolSize = 20;
	InitialPoolSize = 12;
	bPreSpawnPoolActors = true;
	ActiveNPCCount = 0;
	bInitialized = false;

	// Spatial grid
	SpatialGridCellSize = 1000.0f;

	// Update intervals
	DistanceCheckInterval = 1.0f;
	PatrolUpdateInterval = 0.1f;
	SpatialGridRebuildInterval = 5.0f;

	// Timers
	DistanceCheckTimer = 0.0f;
	PatrolUpdateTimer = 0.0f;
	SpatialRebuildTimer = 0.0f;
	PatrolStaggerCursor = 0;
	DistanceStaggerCursor = 0;
	LastUpdateTime = 0.0f;

	// Player location cache
	CachedPlayerLocation = FVector::ZeroVector;
	bPlayerLocationValid = false;

	// Default NPC ship class
	DefaultNPCShipClass = nullptr;

	// Performance tier defaults
	// High: 12 NPCs, fast updates, generous LOD distances, large batch
	HighPerformanceLimits = FNPCPerformanceLimits(
		12,       // MaxNPCs
		0.05f,    // UpdateFrequency
		5000.0f,  // CullingDistance
		true,     // bEnablePatrolling
		1500.0f,  // FullLODDistance
		3000.0f,  // ReducedLODDistance
		5000.0f,  // MinimalLODDistance
		6         // PatrolBatchSize
	);

	// Medium: 8 NPCs, moderate updates
	MediumPerformanceLimits = FNPCPerformanceLimits(
		8,        // MaxNPCs
		0.1f,     // UpdateFrequency
		3500.0f,  // CullingDistance
		true,     // bEnablePatrolling
		1000.0f,  // FullLODDistance
		2000.0f,  // ReducedLODDistance
		3500.0f,  // MinimalLODDistance
		4         // PatrolBatchSize
	);

	// Low: 4 NPCs, infrequent updates, no patrolling, tight LOD
	LowPerformanceLimits = FNPCPerformanceLimits(
		4,        // MaxNPCs
		0.2f,     // UpdateFrequency
		2500.0f,  // CullingDistance
		false,    // bEnablePatrolling
		600.0f,   // FullLODDistance
		1200.0f,  // ReducedLODDistance
		2500.0f,  // MinimalLODDistance
		2         // PatrolBatchSize
	);

	CurrentPerformanceTier = EPerformanceTier::Medium;
	CurrentLimits = MediumPerformanceLimits;

	MobileOptimizer = nullptr;
}

// ============================================================================
// Lifecycle
// ============================================================================

void ANPCSpawnManager::BeginPlay()
{
	Super::BeginPlay();

	// Locate the mobile optimizer component in the world
	if (UWorld* World = GetWorld())
	{
		if (AActor* GameMode = UGameplayStatics::GetGameMode(World))
		{
			MobileOptimizer = GameMode->FindComponentByClass<UOdysseyMobileOptimizer>();
		}

		if (!MobileOptimizer)
		{
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
			{
				MobileOptimizer = PC->FindComponentByClass<UOdysseyMobileOptimizer>();
			}
		}

		// Also search any actor that owns the optimizer
		if (!MobileOptimizer)
		{
			TArray<AActor*> AllActors;
			UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
			for (AActor* Actor : AllActors)
			{
				MobileOptimizer = Actor->FindComponentByClass<UOdysseyMobileOptimizer>();
				if (MobileOptimizer)
				{
					break;
				}
			}
		}
	}

	// Detect current tier from optimizer
	if (MobileOptimizer)
	{
		CurrentPerformanceTier = MobileOptimizer->GetCurrentPerformanceTier();
	}

	InitializeNPCSystem();

	UE_LOG(LogTemp, Warning, TEXT("ANPCSpawnManager::BeginPlay - Initialized with %d spawn definitions, pool size %d, tier %d"),
		NPCSpawnData.Num(), NPCPool.Num(), static_cast<int32>(CurrentPerformanceTier));
}

void ANPCSpawnManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShutdownNPCSystem();
	Super::EndPlay(EndPlayReason);
}

void ANPCSpawnManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bInitialized)
	{
		return;
	}

	// Check for performance tier changes
	if (MobileOptimizer)
	{
		EPerformanceTier NewTier = MobileOptimizer->GetCurrentPerformanceTier();
		if (NewTier != CurrentPerformanceTier)
		{
			OnPerformanceTierChanged(NewTier);
		}
	}

	StaggeredUpdate(DeltaTime);
	LastUpdateTime += DeltaTime;
}

// ============================================================================
// Initialization / Shutdown
// ============================================================================

void ANPCSpawnManager::InitializeNPCSystem()
{
	if (bInitialized)
	{
		return;
	}

	InitializePerformanceLimits();
	SortNPCSpawnDataByPriority();

	// Register any patrol routes embedded in spawn data
	for (const FNPCSpawnData& Data : NPCSpawnData)
	{
		if (Data.PatrolRoute.RouteId != NAME_None && Data.PatrolRoute.Waypoints.Num() > 0)
		{
			RegisterPatrolRoute(Data.PatrolRoute);
		}
	}

	// Initialize spatial grid
	SpatialGrid.Initialize(SpatialGridCellSize);

	// Initialize object pool
	InitializeNPCPool();

	// Spawn initial NPCs based on current tier
	OptimizeNPCCount();

	bInitialized = true;

	UE_LOG(LogTemp, Log, TEXT("NPC System initialized: Pool=%d, Tier=%d, MaxNPCs=%d"),
		NPCPool.Num(), static_cast<int32>(CurrentPerformanceTier), CurrentLimits.MaxNPCs);
}

void ANPCSpawnManager::ShutdownNPCSystem()
{
	if (!bInitialized)
	{
		return;
	}

	// Destroy all pooled actors
	for (FNPCPoolEntry& Entry : NPCPool)
	{
		if (Entry.NPCActor && IsValid(Entry.NPCActor))
		{
			Entry.NPCActor->Destroy();
			Entry.NPCActor = nullptr;
		}
	}

	NPCPool.Empty();
	SpatialGrid.Clear();
	PatrolRouteRegistry.Empty();
	ActiveNPCCount = 0;
	bInitialized = false;

	UE_LOG(LogTemp, Log, TEXT("NPC System shutdown complete"));
}

// ============================================================================
// Object Pool
// ============================================================================

void ANPCSpawnManager::InitializeNPCPool()
{
	NPCPool.Empty();
	NPCPool.Reserve(MaxPoolSize);

	for (int32 i = 0; i < InitialPoolSize; i++)
	{
		FNPCPoolEntry NewEntry;
		NPCPool.Add(NewEntry);
	}

	// Pre-spawn actors if requested (zero-allocation mode)
	if (bPreSpawnPoolActors)
	{
		PreSpawnPoolActors();
	}

	UE_LOG(LogTemp, Verbose, TEXT("NPC Pool initialized: %d entries, PreSpawn=%s"),
		InitialPoolSize, bPreSpawnPoolActors ? TEXT("true") : TEXT("false"));
}

void ANPCSpawnManager::PreSpawnPoolActors()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Determine the class to pre-spawn.
	// Use DefaultNPCShipClass if set; otherwise fall back to the first spawn data's class.
	TSubclassOf<AOdysseyCharacter> PreSpawnClass = nullptr;

	if (DefaultNPCShipClass)
	{
		PreSpawnClass = DefaultNPCShipClass;
	}
	else if (NPCSpawnData.Num() > 0 && NPCSpawnData[0].NPCClass)
	{
		PreSpawnClass = NPCSpawnData[0].NPCClass;
	}

	if (!PreSpawnClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("PreSpawnPoolActors: No NPC class available for pre-spawning"));
		return;
	}

	// Hidden off-screen spawn location
	const FVector HiddenLocation = FVector(0.0f, 0.0f, -50000.0f);
	const FRotator HiddenRotation = FRotator::ZeroRotator;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 i = 0; i < NPCPool.Num(); i++)
	{
		FNPCPoolEntry& Entry = NPCPool[i];
		if (Entry.NPCActor && IsValid(Entry.NPCActor))
		{
			continue; // Already has an actor
		}

		AOdysseyCharacter* NewActor = World->SpawnActor<AOdysseyCharacter>(
			PreSpawnClass, HiddenLocation, HiddenRotation, SpawnParams);

		if (NewActor)
		{
			// Immediately make dormant
			NewActor->SetActorHiddenInGame(true);
			NewActor->SetActorEnableCollision(false);
			NewActor->SetActorTickEnabled(false);

			Entry.NPCActor = NewActor;
			Entry.bPreSpawned = true;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("PreSpawnPoolActors: Failed to spawn actor for pool slot %d"), i);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Pre-spawned %d NPC actors into pool"), NPCPool.Num());
}

FNPCPoolEntry* ANPCSpawnManager::GetPooledNPC()
{
	// Find an unused entry (prefer one with a pre-spawned actor)
	int32 BestIndex = -1;
	for (int32 i = 0; i < NPCPool.Num(); i++)
	{
		if (!NPCPool[i].bInUse)
		{
			if (NPCPool[i].bPreSpawned && NPCPool[i].NPCActor && IsValid(NPCPool[i].NPCActor))
			{
				// Best case: pre-spawned and ready
				NPCPool[i].bInUse = true;
				return &NPCPool[i];
			}
			if (BestIndex == -1)
			{
				BestIndex = i;
			}
		}
	}

	// Fall back to a non-pre-spawned slot
	if (BestIndex >= 0)
	{
		NPCPool[BestIndex].bInUse = true;
		return &NPCPool[BestIndex];
	}

	// Expand pool if possible
	if (NPCPool.Num() < MaxPoolSize)
	{
		ExpandPool(1);
		int32 NewIndex = NPCPool.Num() - 1;
		NPCPool[NewIndex].bInUse = true;
		return &NPCPool[NewIndex];
	}

	UE_LOG(LogTemp, Warning, TEXT("NPC Pool exhausted (max %d). Cannot allocate."), MaxPoolSize);
	return nullptr;
}

void ANPCSpawnManager::ReturnNPCToPool(int32 PoolIndex)
{
	if (!NPCPool.IsValidIndex(PoolIndex))
	{
		return;
	}

	FNPCPoolEntry& Entry = NPCPool[PoolIndex];

	// Remove from spatial grid
	SpatialGrid.Remove(PoolIndex, Entry.GridCellX, Entry.GridCellY);

	// Make dormant
	if (Entry.NPCActor && IsValid(Entry.NPCActor))
	{
		SetNPCDormant(Entry.NPCActor);
		// Move to hidden location so it doesn't interfere
		Entry.NPCActor->SetActorLocation(FVector(0.0f, 0.0f, -50000.0f));
	}

	if (Entry.bActive)
	{
		ActiveNPCCount = FMath::Max(0, ActiveNPCCount - 1);
	}

	// Reset the entry but preserve the actor pointer for reuse
	AOdysseyCharacter* PreservedActor = Entry.NPCActor;
	bool bWasPreSpawned = Entry.bPreSpawned;
	ResetNPCPoolEntry(PoolIndex);
	Entry.NPCActor = PreservedActor;
	Entry.bPreSpawned = bWasPreSpawned;
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

	// Pre-spawn actors for new entries if in zero-allocation mode
	if (bPreSpawnPoolActors)
	{
		UWorld* World = GetWorld();
		TSubclassOf<AOdysseyCharacter> PreSpawnClass = DefaultNPCShipClass;
		if (!PreSpawnClass && NPCSpawnData.Num() > 0)
		{
			PreSpawnClass = NPCSpawnData[0].NPCClass;
		}

		if (World && PreSpawnClass)
		{
			const FVector HiddenLocation(0.0f, 0.0f, -50000.0f);
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			for (int32 i = NPCPool.Num() - ActualIncrease; i < NPCPool.Num(); i++)
			{
				FNPCPoolEntry& Entry = NPCPool[i];
				if (!Entry.NPCActor)
				{
					AOdysseyCharacter* NewActor = World->SpawnActor<AOdysseyCharacter>(
						PreSpawnClass, HiddenLocation, FRotator::ZeroRotator, SpawnParams);
					if (NewActor)
					{
						SetNPCDormant(NewActor);
						Entry.NPCActor = NewActor;
						Entry.bPreSpawned = true;
					}
				}
			}
		}
	}

	UE_LOG(LogTemp, Verbose, TEXT("Expanded NPC pool by %d (total: %d)"), ActualIncrease, NPCPool.Num());
}

// ============================================================================
// Spawning
// ============================================================================

bool ANPCSpawnManager::SpawnNPC(int32 SpawnDataIndex)
{
	if (!NPCSpawnData.IsValidIndex(SpawnDataIndex))
	{
		return false;
	}

	if (!CanSpawnMoreNPCs())
	{
		return false;
	}

	const FNPCSpawnData& SpawnData = NPCSpawnData[SpawnDataIndex];
	if (!SpawnData.NPCClass)
	{
		return false;
	}

	FNPCPoolEntry* PoolEntry = GetPooledNPC();
	if (!PoolEntry)
	{
		return false;
	}

	// Determine pool index for this entry
	int32 PoolIndex = static_cast<int32>(PoolEntry - NPCPool.GetData());

	// Create or reuse NPC actor
	if (!PoolEntry->NPCActor || !IsValid(PoolEntry->NPCActor))
	{
		// Need to spawn a new actor (shouldn't happen if pre-spawning is on)
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		PoolEntry->NPCActor = GetWorld()->SpawnActor<AOdysseyCharacter>(
			SpawnData.NPCClass, SpawnData.SpawnLocation, SpawnData.SpawnRotation, SpawnParams);

		if (!PoolEntry->NPCActor)
		{
			UE_LOG(LogTemp, Error, TEXT("SpawnNPC: Failed to spawn actor for spawn data %d"), SpawnDataIndex);
			PoolEntry->bInUse = false;
			return false;
		}
	}
	else
	{
		// Reuse pre-spawned actor: reposition
		PoolEntry->NPCActor->SetActorLocation(SpawnData.SpawnLocation);
		PoolEntry->NPCActor->SetActorRotation(SpawnData.SpawnRotation);
	}

	// Configure pool entry
	PoolEntry->SpawnDataIndex = SpawnDataIndex;
	PoolEntry->CurrentWaypointIndex = 0;
	PoolEntry->bMovingToWaypoint = false;
	PoolEntry->bWaitingAtWaypoint = false;
	PoolEntry->MoveStartTime = 0.0f;
	PoolEntry->WaitStartTime = 0.0f;
	PoolEntry->CachedDistanceToPlayer = FLT_MAX;
	PoolEntry->BehaviorLOD = ENPCBehaviorLOD::Dormant;

	// Configure NPCShip-specific properties if applicable
	if (ANPCShip* NPCShip = Cast<ANPCShip>(PoolEntry->NPCActor))
	{
		// Set patrol route on the behavior component if it has waypoints
		if (SpawnData.PatrolRoute.Waypoints.Num() > 0)
		{
			TArray<FVector> PatrolPoints;
			PatrolPoints.Reserve(SpawnData.PatrolRoute.Waypoints.Num());
			for (const FWaypoint& WP : SpawnData.PatrolRoute.Waypoints)
			{
				PatrolPoints.Add(WP.Location);
			}
			NPCShip->SetPatrolRoute(PatrolPoints);
			NPCShip->SetRespawnLocation(SpawnData.SpawnLocation, SpawnData.SpawnRotation);
		}
	}

	// Insert into spatial grid
	SpatialGrid.Insert(PoolIndex, SpawnData.SpawnLocation);
	SpatialGrid.WorldToCell(SpawnData.SpawnLocation, PoolEntry->GridCellX, PoolEntry->GridCellY);

	// Activate the NPC
	ActivateNPC(PoolIndex);

	// Start patrol if available and allowed
	if (SpawnData.PatrolRoute.Waypoints.Num() > 0 && CurrentLimits.bEnablePatrolling)
	{
		PoolEntry->bMovingToWaypoint = true;
		PoolEntry->MoveStartTime = GetWorld()->GetTimeSeconds();
	}

	OnNPCSpawned(PoolEntry->NPCActor, SpawnDataIndex);

	UE_LOG(LogTemp, Verbose, TEXT("SpawnNPC: pool[%d] <- spawnData[%d] at %s"),
		PoolIndex, SpawnDataIndex, *SpawnData.SpawnLocation.ToString());

	return true;
}

void ANPCSpawnManager::DespawnNPC(int32 PoolIndex)
{
	if (!NPCPool.IsValidIndex(PoolIndex))
	{
		return;
	}

	FNPCPoolEntry& Entry = NPCPool[PoolIndex];
	if (!Entry.bInUse)
	{
		return;
	}

	OnNPCDespawned(Entry.NPCActor, Entry.SpawnDataIndex);
	ReturnNPCToPool(PoolIndex);
}

void ANPCSpawnManager::ActivateNPC(int32 PoolIndex)
{
	if (!NPCPool.IsValidIndex(PoolIndex))
	{
		return;
	}

	FNPCPoolEntry& Entry = NPCPool[PoolIndex];
	if (!Entry.bInUse || Entry.bActive)
	{
		return;
	}

	if (Entry.NPCActor && IsValid(Entry.NPCActor))
	{
		// Determine initial LOD based on distance
		ENPCBehaviorLOD InitialLOD = ENPCBehaviorLOD::Full;
		if (bPlayerLocationValid)
		{
			float Dist = FVector::Dist(CachedPlayerLocation, Entry.NPCActor->GetActorLocation());
			Entry.CachedDistanceToPlayer = Dist;
			InitialLOD = ComputeBehaviorLOD(Dist);
		}

		// Make visible and enable based on LOD
		ApplyBehaviorLOD(PoolIndex, InitialLOD);

		Entry.bActive = true;
		ActiveNPCCount++;

		OnNPCActivated(Entry.NPCActor);

		UE_LOG(LogTemp, Verbose, TEXT("ActivateNPC: pool[%d] active=%d LOD=%d"),
			PoolIndex, ActiveNPCCount, static_cast<int32>(InitialLOD));
	}
}

void ANPCSpawnManager::DeactivateNPC(int32 PoolIndex)
{
	if (!NPCPool.IsValidIndex(PoolIndex))
	{
		return;
	}

	FNPCPoolEntry& Entry = NPCPool[PoolIndex];
	if (!Entry.bActive)
	{
		return;
	}

	if (Entry.NPCActor && IsValid(Entry.NPCActor))
	{
		SetNPCDormant(Entry.NPCActor);

		Entry.bActive = false;
		Entry.BehaviorLOD = ENPCBehaviorLOD::Dormant;
		ActiveNPCCount = FMath::Max(0, ActiveNPCCount - 1);

		OnNPCDeactivated(Entry.NPCActor);

		UE_LOG(LogTemp, Verbose, TEXT("DeactivateNPC: pool[%d] active=%d"), PoolIndex, ActiveNPCCount);
	}
}

// ============================================================================
// Performance Tier
// ============================================================================

void ANPCSpawnManager::UpdatePerformanceSettings()
{
	if (!MobileOptimizer)
	{
		return;
	}

	EPerformanceTier NewTier = MobileOptimizer->GetCurrentPerformanceTier();
	if (NewTier != CurrentPerformanceTier)
	{
		OnPerformanceTierChanged(NewTier);
	}
}

void ANPCSpawnManager::OnPerformanceTierChanged(EPerformanceTier NewTier)
{
	EPerformanceTier OldTier = CurrentPerformanceTier;
	CurrentPerformanceTier = NewTier;
	CurrentLimits = GetLimitsForTier(NewTier);

	UE_LOG(LogTemp, Warning, TEXT("Performance tier changed: %d -> %d, MaxNPCs: %d, Culling: %.0f"),
		static_cast<int32>(OldTier), static_cast<int32>(NewTier),
		CurrentLimits.MaxNPCs, CurrentLimits.CullingDistance);

	// Re-optimize NPC count for the new tier
	OptimizeNPCCount();
	OnPerformanceOptimized(NewTier, ActiveNPCCount);
}

void ANPCSpawnManager::OptimizeNPCCount()
{
	int32 TargetNPCs = GetMaxNPCsForCurrentTier();

	// --- Phase 1: Deactivate excess non-essential NPCs (farthest first) ---
	if (ActiveNPCCount > TargetNPCs)
	{
		// Collect active non-essential entries sorted by distance (farthest first)
		TArray<TPair<float, int32>> CandidatesForDeactivation;
		for (int32 i = 0; i < NPCPool.Num(); i++)
		{
			FNPCPoolEntry& Entry = NPCPool[i];
			if (Entry.bActive && Entry.bInUse && NPCSpawnData.IsValidIndex(Entry.SpawnDataIndex))
			{
				if (!NPCSpawnData[Entry.SpawnDataIndex].bEssential)
				{
					CandidatesForDeactivation.Add(TPair<float, int32>(Entry.CachedDistanceToPlayer, i));
				}
			}
		}

		// Sort descending by distance (farthest first to deactivate)
		CandidatesForDeactivation.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& B)
		{
			return A.Key > B.Key;
		});

		for (const auto& Pair : CandidatesForDeactivation)
		{
			if (ActiveNPCCount <= TargetNPCs)
			{
				break;
			}
			DeactivateNPC(Pair.Value);
		}
	}

	// --- Phase 2: Activate / spawn up to target (closest first) ---
	while (ActiveNPCCount < TargetNPCs)
	{
		bool bProgressMade = false;

		// First try to reactivate existing inactive pooled NPCs
		TArray<TPair<float, int32>> ReactivationCandidates;
		for (int32 i = 0; i < NPCPool.Num(); i++)
		{
			FNPCPoolEntry& Entry = NPCPool[i];
			if (Entry.bInUse && !Entry.bActive && Entry.NPCActor && IsValid(Entry.NPCActor))
			{
				float Dist = bPlayerLocationValid
					? FVector::Dist(CachedPlayerLocation, Entry.NPCActor->GetActorLocation())
					: 0.0f;
				ReactivationCandidates.Add(TPair<float, int32>(Dist, i));
			}
		}

		// Sort ascending (closest first)
		ReactivationCandidates.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& B)
		{
			return A.Key < B.Key;
		});

		for (const auto& Pair : ReactivationCandidates)
		{
			if (ActiveNPCCount >= TargetNPCs)
			{
				break;
			}
			ActivateNPC(Pair.Value);
			bProgressMade = true;
		}

		if (ActiveNPCCount >= TargetNPCs)
		{
			break;
		}

		// Try to spawn new NPCs from spawn data
		bool bSpawned = false;
		for (int32 i = 0; i < NPCSpawnData.Num(); i++)
		{
			// Check if this spawn data is already in use
			bool bAlreadyUsed = false;
			for (const FNPCPoolEntry& Entry : NPCPool)
			{
				if (Entry.bInUse && Entry.SpawnDataIndex == i)
				{
					bAlreadyUsed = true;
					break;
				}
			}

			if (!bAlreadyUsed)
			{
				if (SpawnNPC(i))
				{
					bSpawned = true;
					bProgressMade = true;
					break;
				}
			}
		}

		if (!bProgressMade)
		{
			break; // No more NPCs can be spawned or reactivated
		}
	}

	UE_LOG(LogTemp, Log, TEXT("OptimizeNPCCount: %d/%d active (tier %d)"),
		ActiveNPCCount, TargetNPCs, static_cast<int32>(CurrentPerformanceTier));
}

int32 ANPCSpawnManager::GetMaxNPCsForCurrentTier() const
{
	return CurrentLimits.MaxNPCs;
}

// ============================================================================
// Distance & LOD
// ============================================================================

void ANPCSpawnManager::UpdateNPCDistances()
{
	if (!RefreshPlayerLocation())
	{
		return;
	}

	// Process a batch of NPCs per call (staggered to spread load)
	const int32 BatchSize = FMath::Max(1, NPCPool.Num() / 3);
	int32 EndIndex = FMath::Min(DistanceStaggerCursor + BatchSize, NPCPool.Num());

	for (int32 i = DistanceStaggerCursor; i < EndIndex; i++)
	{
		FNPCPoolEntry& Entry = NPCPool[i];
		if (!Entry.bInUse || !Entry.NPCActor || !IsValid(Entry.NPCActor))
		{
			continue;
		}

		const FVector NPCLocation = Entry.NPCActor->GetActorLocation();
		const float Distance = FVector::Dist(CachedPlayerLocation, NPCLocation);
		Entry.CachedDistanceToPlayer = Distance;

		// Update spatial grid position
		int32 NewCellX, NewCellY;
		SpatialGrid.Move(i, Entry.GridCellX, Entry.GridCellY, NPCLocation, NewCellX, NewCellY);
		Entry.GridCellX = NewCellX;
		Entry.GridCellY = NewCellY;

		// Compute desired behavior LOD
		ENPCBehaviorLOD DesiredLOD = ComputeBehaviorLOD(Distance);

		// Determine if this NPC should be active based on distance and tier limits
		bool bShouldBeActive = (Distance <= CurrentLimits.CullingDistance);

		// Essential NPCs override distance culling
		if (NPCSpawnData.IsValidIndex(Entry.SpawnDataIndex) && NPCSpawnData[Entry.SpawnDataIndex].bEssential)
		{
			bShouldBeActive = true;
		}

		// Handle activation / deactivation
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
			if (!NPCSpawnData.IsValidIndex(Entry.SpawnDataIndex) || !NPCSpawnData[Entry.SpawnDataIndex].bEssential)
			{
				DeactivateNPC(i);
			}
		}

		// Apply behavior LOD transition (only for active NPCs)
		if (Entry.bActive && DesiredLOD != Entry.BehaviorLOD)
		{
			ApplyBehaviorLOD(i, DesiredLOD);
		}
	}

	DistanceStaggerCursor = (EndIndex >= NPCPool.Num()) ? 0 : EndIndex;
}

void ANPCSpawnManager::CullDistantNPCs()
{
	if (!bPlayerLocationValid)
	{
		return;
	}

	for (int32 i = 0; i < NPCPool.Num(); i++)
	{
		FNPCPoolEntry& Entry = NPCPool[i];
		if (!Entry.bActive || !Entry.NPCActor || !IsValid(Entry.NPCActor))
		{
			continue;
		}

		if (Entry.CachedDistanceToPlayer > CurrentLimits.CullingDistance)
		{
			if (!NPCSpawnData.IsValidIndex(Entry.SpawnDataIndex) || !NPCSpawnData[Entry.SpawnDataIndex].bEssential)
			{
				DeactivateNPC(i);
			}
		}
	}
}

void ANPCSpawnManager::ActivateNearbyNPCs()
{
	if (ActiveNPCCount >= CurrentLimits.MaxNPCs || !bPlayerLocationValid)
	{
		return;
	}

	// Use spatial grid for efficient neighbour query
	TArray<int32> NearbyIndices;
	SpatialGrid.QueryRadius(CachedPlayerLocation, CurrentLimits.CullingDistance, NearbyIndices);

	// Collect inactive candidates with distance
	TArray<TPair<float, int32>> Candidates;
	for (int32 Idx : NearbyIndices)
	{
		if (NPCPool.IsValidIndex(Idx))
		{
			FNPCPoolEntry& Entry = NPCPool[Idx];
			if (Entry.bInUse && !Entry.bActive && Entry.NPCActor && IsValid(Entry.NPCActor))
			{
				Candidates.Add(TPair<float, int32>(Entry.CachedDistanceToPlayer, Idx));
			}
		}
	}

	// Sort by distance ascending (closest first)
	Candidates.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& B)
	{
		return A.Key < B.Key;
	});

	for (const auto& Candidate : Candidates)
	{
		if (ActiveNPCCount >= CurrentLimits.MaxNPCs)
		{
			break;
		}
		ActivateNPC(Candidate.Value);
	}
}

float ANPCSpawnManager::GetDistanceToPlayer(const FVector& NPCLocation) const
{
	if (bPlayerLocationValid)
	{
		return FVector::Dist(CachedPlayerLocation, NPCLocation);
	}

	APawn* PlayerPawn = GetPlayerPawn();
	if (PlayerPawn)
	{
		return FVector::Dist(PlayerPawn->GetActorLocation(), NPCLocation);
	}

	return FLT_MAX;
}

ENPCBehaviorLOD ANPCSpawnManager::ComputeBehaviorLOD(float Distance) const
{
	if (Distance <= CurrentLimits.FullLODDistance)
	{
		return ENPCBehaviorLOD::Full;
	}
	if (Distance <= CurrentLimits.ReducedLODDistance)
	{
		return ENPCBehaviorLOD::Reduced;
	}
	if (Distance <= CurrentLimits.MinimalLODDistance)
	{
		return ENPCBehaviorLOD::Minimal;
	}
	return ENPCBehaviorLOD::Dormant;
}

void ANPCSpawnManager::ApplyBehaviorLOD(int32 PoolIndex, ENPCBehaviorLOD NewLOD)
{
	if (!NPCPool.IsValidIndex(PoolIndex))
	{
		return;
	}

	FNPCPoolEntry& Entry = NPCPool[PoolIndex];
	ENPCBehaviorLOD OldLOD = Entry.BehaviorLOD;

	if (OldLOD == NewLOD)
	{
		return;
	}

	Entry.BehaviorLOD = NewLOD;

	AOdysseyCharacter* NPC = Entry.NPCActor;
	if (!NPC || !IsValid(NPC))
	{
		return;
	}

	switch (NewLOD)
	{
	case ENPCBehaviorLOD::Full:
		SetNPCVisible(NPC, true);
		NPC->SetActorTickEnabled(true);
		NPC->SetActorEnableCollision(true);
		ConfigureNPCTickRate(NPC, NewLOD);
		break;

	case ENPCBehaviorLOD::Reduced:
		SetNPCVisible(NPC, true);
		NPC->SetActorTickEnabled(true);
		NPC->SetActorEnableCollision(true);
		ConfigureNPCTickRate(NPC, NewLOD);
		break;

	case ENPCBehaviorLOD::Minimal:
		// Visible but no logic tick and no collision
		SetNPCVisible(NPC, true);
		NPC->SetActorTickEnabled(false);
		NPC->SetActorEnableCollision(false);
		break;

	case ENPCBehaviorLOD::Dormant:
		SetNPCDormant(NPC);
		break;
	}

	OnNPCBehaviorLODChanged(NPC, OldLOD, NewLOD);
}

// ============================================================================
// Patrol System
// ============================================================================

void ANPCSpawnManager::UpdateNPCPatrols(float DeltaTime)
{
	if (!CurrentLimits.bEnablePatrolling)
	{
		return;
	}

	// Process a batch per call
	const int32 BatchSize = FMath::Max(1, CurrentLimits.PatrolBatchSize);
	int32 Processed = 0;

	while (Processed < BatchSize && PatrolStaggerCursor < NPCPool.Num())
	{
		FNPCPoolEntry& Entry = NPCPool[PatrolStaggerCursor];

		// Only update patrols for active NPCs at Full or Reduced LOD
		if (Entry.bActive && Entry.bInUse &&
			(Entry.BehaviorLOD == ENPCBehaviorLOD::Full || Entry.BehaviorLOD == ENPCBehaviorLOD::Reduced))
		{
			// Reduced LOD NPCs get simplified patrol (skip every other update)
			if (Entry.BehaviorLOD == ENPCBehaviorLOD::Reduced)
			{
				// Use a simple frame-skip: only update on even stagger cursors
				if ((PatrolStaggerCursor & 1) == 0)
				{
					UpdateNPCPatrol(PatrolStaggerCursor, DeltaTime);
				}
			}
			else
			{
				UpdateNPCPatrol(PatrolStaggerCursor, DeltaTime);
			}

			Processed++;
		}

		PatrolStaggerCursor++;
	}

	// Wrap around
	if (PatrolStaggerCursor >= NPCPool.Num())
	{
		PatrolStaggerCursor = 0;
	}
}

void ANPCSpawnManager::UpdateNPCPatrol(int32 PoolIndex, float DeltaTime)
{
	if (!NPCPool.IsValidIndex(PoolIndex))
	{
		return;
	}

	FNPCPoolEntry& Entry = NPCPool[PoolIndex];
	if (!Entry.bActive || !Entry.bInUse || !NPCSpawnData.IsValidIndex(Entry.SpawnDataIndex))
	{
		return;
	}

	const FNPCSpawnData& SpawnData = NPCSpawnData[Entry.SpawnDataIndex];
	const FPatrolRoute& Route = SpawnData.PatrolRoute;

	if (Route.Waypoints.Num() == 0)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const FWaypoint& CurrentWaypoint = Route.Waypoints[Entry.CurrentWaypointIndex];

	// --- Waiting at waypoint ---
	if (Entry.bWaitingAtWaypoint)
	{
		if (CurrentTime - Entry.WaitStartTime >= CurrentWaypoint.WaitTime)
		{
			Entry.bWaitingAtWaypoint = false;
			Entry.bMovingToWaypoint = true;
			Entry.MoveStartTime = CurrentTime;

			// Advance to next waypoint
			Entry.CurrentWaypointIndex = (Entry.CurrentWaypointIndex + 1) % Route.Waypoints.Num();
			if (!Route.bLooping && Entry.CurrentWaypointIndex == 0)
			{
				Entry.bMovingToWaypoint = false;
				return;
			}
		}
		return;
	}

	// --- Moving to waypoint ---
	if (Entry.bMovingToWaypoint)
	{
		AOdysseyCharacter* NPC = Entry.NPCActor;
		if (!NPC || !IsValid(NPC))
		{
			return;
		}

		const FVector CurrentLocation = NPC->GetActorLocation();
		const FVector TargetLocation = CurrentWaypoint.Location;

		// Check arrival
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
				Entry.CurrentWaypointIndex = (Entry.CurrentWaypointIndex + 1) % Route.Waypoints.Num();
				if (!Route.bLooping && Entry.CurrentWaypointIndex == 0)
				{
					return;
				}
				Entry.MoveStartTime = CurrentTime;
				Entry.bMovingToWaypoint = true;
			}
			return;
		}

		// Move towards waypoint
		const FVector Direction = (TargetLocation - CurrentLocation).GetSafeNormal();
		const float MovementDistance = Route.MovementSpeed * DeltaTime;
		const float DistanceToTarget = FVector::Dist(CurrentLocation, TargetLocation);

		FVector NewLocation;
		if (MovementDistance >= DistanceToTarget)
		{
			NewLocation = TargetLocation;
		}
		else
		{
			NewLocation = CurrentLocation + Direction * MovementDistance;
		}

		NPC->SetActorLocation(NewLocation);

		// Face movement direction
		if (!Direction.IsNearlyZero())
		{
			NPC->SetActorRotation(Direction.Rotation());
		}
	}
}

void ANPCSpawnManager::MoveNPCToWaypoint(int32 PoolIndex, const FWaypoint& Waypoint)
{
	if (!NPCPool.IsValidIndex(PoolIndex))
	{
		return;
	}

	FNPCPoolEntry& Entry = NPCPool[PoolIndex];
	if (!Entry.bActive || !Entry.NPCActor || !IsValid(Entry.NPCActor))
	{
		return;
	}

	Entry.bMovingToWaypoint = true;
	Entry.bWaitingAtWaypoint = false;
	Entry.MoveStartTime = GetWorld()->GetTimeSeconds();
}

bool ANPCSpawnManager::IsNPCAtWaypoint(int32 PoolIndex, const FWaypoint& Waypoint) const
{
	if (!NPCPool.IsValidIndex(PoolIndex))
	{
		return false;
	}

	const FNPCPoolEntry& Entry = NPCPool[PoolIndex];
	if (!Entry.NPCActor || !IsValid(Entry.NPCActor))
	{
		return false;
	}

	return FVector::Dist(Entry.NPCActor->GetActorLocation(), Waypoint.Location) <= 50.0f;
}

FWaypoint ANPCSpawnManager::GetNextWaypoint(int32 PoolIndex) const
{
	if (!NPCPool.IsValidIndex(PoolIndex))
	{
		return FWaypoint();
	}

	const FNPCPoolEntry& Entry = NPCPool[PoolIndex];
	if (!NPCSpawnData.IsValidIndex(Entry.SpawnDataIndex))
	{
		return FWaypoint();
	}

	const FPatrolRoute& Route = NPCSpawnData[Entry.SpawnDataIndex].PatrolRoute;
	if (Route.Waypoints.Num() == 0)
	{
		return FWaypoint();
	}

	int32 NextIndex = (Entry.CurrentWaypointIndex + 1) % Route.Waypoints.Num();
	return Route.Waypoints[NextIndex];
}

void ANPCSpawnManager::RegisterPatrolRoute(const FPatrolRoute& Route)
{
	if (Route.RouteId == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("RegisterPatrolRoute: RouteId is NAME_None, skipping"));
		return;
	}

	PatrolRouteRegistry.Add(Route.RouteId, Route);
	UE_LOG(LogTemp, Verbose, TEXT("Registered patrol route '%s' with %d waypoints"),
		*Route.RouteName, Route.Waypoints.Num());
}

const FPatrolRoute* ANPCSpawnManager::GetPatrolRoute(FName RouteId) const
{
	return PatrolRouteRegistry.Find(RouteId);
}

// ============================================================================
// Queries
// ============================================================================

bool ANPCSpawnManager::IsNPCActive(int32 PoolIndex) const
{
	if (!NPCPool.IsValidIndex(PoolIndex))
	{
		return false;
	}
	return NPCPool[PoolIndex].bActive;
}

AOdysseyCharacter* ANPCSpawnManager::GetNPCFromPool(int32 PoolIndex) const
{
	if (!NPCPool.IsValidIndex(PoolIndex))
	{
		return nullptr;
	}
	return NPCPool[PoolIndex].NPCActor;
}

TArray<AOdysseyCharacter*> ANPCSpawnManager::GetActiveNPCs() const
{
	TArray<AOdysseyCharacter*> Result;
	Result.Reserve(ActiveNPCCount);

	for (const FNPCPoolEntry& Entry : NPCPool)
	{
		if (Entry.bActive && Entry.NPCActor && IsValid(Entry.NPCActor))
		{
			Result.Add(Entry.NPCActor);
		}
	}
	return Result;
}

TArray<int32> ANPCSpawnManager::GetNPCsInRadius(const FVector& Center, float Radius) const
{
	TArray<int32> Indices;
	SpatialGrid.QueryRadius(Center, Radius, Indices);

	// Filter to only valid, in-use entries
	Indices.RemoveAll([this](int32 Idx)
	{
		return !NPCPool.IsValidIndex(Idx) || !NPCPool[Idx].bInUse;
	});

	return Indices;
}

int32 ANPCSpawnManager::GetPoolEntriesInUse() const
{
	int32 Count = 0;
	for (const FNPCPoolEntry& Entry : NPCPool)
	{
		if (Entry.bInUse)
		{
			Count++;
		}
	}
	return Count;
}

ENPCBehaviorLOD ANPCSpawnManager::GetNPCBehaviorLOD(int32 PoolIndex) const
{
	if (!NPCPool.IsValidIndex(PoolIndex))
	{
		return ENPCBehaviorLOD::Dormant;
	}
	return NPCPool[PoolIndex].BehaviorLOD;
}

// ============================================================================
// Debug
// ============================================================================

void ANPCSpawnManager::DebugDrawPatrolRoutes()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (int32 i = 0; i < NPCSpawnData.Num(); i++)
	{
		const FPatrolRoute& Route = NPCSpawnData[i].PatrolRoute;

		for (int32 j = 0; j < Route.Waypoints.Num(); j++)
		{
			const FWaypoint& WP = Route.Waypoints[j];

			// Draw waypoint sphere
			DrawDebugSphere(World, WP.Location, 25.0f, 8, FColor::Yellow, false, -1.0f, 0, 2.0f);

			// Draw line to next waypoint
			if (j < Route.Waypoints.Num() - 1 || Route.bLooping)
			{
				int32 NextIdx = (j + 1) % Route.Waypoints.Num();
				DrawDebugLine(World, WP.Location, Route.Waypoints[NextIdx].Location,
					FColor::Green, false, -1.0f, 0, 1.0f);
			}
		}
	}
}

void ANPCSpawnManager::DebugDrawNPCStates()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (int32 i = 0; i < NPCPool.Num(); i++)
	{
		const FNPCPoolEntry& Entry = NPCPool[i];
		if (!Entry.bInUse || !Entry.NPCActor || !IsValid(Entry.NPCActor))
		{
			continue;
		}

		const FVector NPCLocation = Entry.NPCActor->GetActorLocation();

		// Color by behavior LOD
		FColor StateColor;
		switch (Entry.BehaviorLOD)
		{
		case ENPCBehaviorLOD::Full:    StateColor = FColor::Green;  break;
		case ENPCBehaviorLOD::Reduced: StateColor = FColor::Yellow; break;
		case ENPCBehaviorLOD::Minimal: StateColor = FColor::Orange; break;
		case ENPCBehaviorLOD::Dormant: StateColor = FColor::Red;    break;
		default:                       StateColor = FColor::White;   break;
		}

		DrawDebugSphere(World, NPCLocation + FVector(0, 0, 100), 15.0f, 8, StateColor, false, -1.0f, 0, 2.0f);

		FString LODName;
		switch (Entry.BehaviorLOD)
		{
		case ENPCBehaviorLOD::Full:    LODName = TEXT("FULL");    break;
		case ENPCBehaviorLOD::Reduced: LODName = TEXT("REDUCED"); break;
		case ENPCBehaviorLOD::Minimal: LODName = TEXT("MINIMAL"); break;
		case ENPCBehaviorLOD::Dormant: LODName = TEXT("DORMANT"); break;
		}

		FString DebugText = FString::Printf(TEXT("Pool:%d Dist:%.0f %s %s"),
			i, Entry.CachedDistanceToPlayer,
			Entry.bActive ? TEXT("ACTIVE") : TEXT("INACTIVE"),
			*LODName);

		DrawDebugString(World, NPCLocation + FVector(0, 0, 120), DebugText, nullptr, StateColor, 0.0f);
	}
}

void ANPCSpawnManager::DebugDrawSpatialGrid()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Draw cells that contain NPCs
	for (const auto& CellPair : SpatialGrid.Cells)
	{
		if (CellPair.Value.Num() == 0)
		{
			continue;
		}

		// Decode cell key
		int32 CX = static_cast<int32>(static_cast<uint32>(CellPair.Key >> 32));
		int32 CY = static_cast<int32>(static_cast<uint32>(CellPair.Key & 0xFFFFFFFF));

		FVector CellMin(CX * SpatialGrid.CellSize, CY * SpatialGrid.CellSize, 0.0f);
		FVector CellMax = CellMin + FVector(SpatialGrid.CellSize, SpatialGrid.CellSize, 100.0f);
		FVector CellCenter = (CellMin + CellMax) * 0.5f;

		FColor CellColor = (CellPair.Value.Num() > 2) ? FColor::Red : FColor::Cyan;

		DrawDebugBox(World, CellCenter, (CellMax - CellMin) * 0.5f, CellColor, false, -1.0f, 0, 1.0f);

		FString CellText = FString::Printf(TEXT("(%d,%d) x%d"), CX, CY, CellPair.Value.Num());
		DrawDebugString(World, CellCenter + FVector(0, 0, 60), CellText, nullptr, CellColor, 0.0f);
	}
}

void ANPCSpawnManager::LogNPCSystemState()
{
	UE_LOG(LogTemp, Log, TEXT("=== NPC System State ==="));
	UE_LOG(LogTemp, Log, TEXT("Performance Tier: %d"), static_cast<int32>(CurrentPerformanceTier));
	UE_LOG(LogTemp, Log, TEXT("Active NPCs: %d / %d (max)"), ActiveNPCCount, CurrentLimits.MaxNPCs);
	UE_LOG(LogTemp, Log, TEXT("Pool Size: %d / %d (max)"), NPCPool.Num(), MaxPoolSize);
	UE_LOG(LogTemp, Log, TEXT("Pool In Use: %d"), GetPoolEntriesInUse());
	UE_LOG(LogTemp, Log, TEXT("Culling Distance: %.0f"), CurrentLimits.CullingDistance);
	UE_LOG(LogTemp, Log, TEXT("Patrolling: %s"), CurrentLimits.bEnablePatrolling ? TEXT("Enabled") : TEXT("Disabled"));
	UE_LOG(LogTemp, Log, TEXT("Patrol Batch Size: %d"), CurrentLimits.PatrolBatchSize);
	UE_LOG(LogTemp, Log, TEXT("Spatial Grid Cells: %d"), SpatialGrid.Cells.Num());
	UE_LOG(LogTemp, Log, TEXT("Registered Patrol Routes: %d"), PatrolRouteRegistry.Num());

	// LOD distribution
	int32 LODCounts[4] = { 0, 0, 0, 0 };
	for (const FNPCPoolEntry& Entry : NPCPool)
	{
		if (Entry.bActive)
		{
			LODCounts[static_cast<int32>(Entry.BehaviorLOD)]++;
		}
	}
	UE_LOG(LogTemp, Log, TEXT("LOD Distribution: Full=%d Reduced=%d Minimal=%d Dormant=%d"),
		LODCounts[0], LODCounts[1], LODCounts[2], LODCounts[3]);

	UE_LOG(LogTemp, Log, TEXT("========================="));
}

// ============================================================================
// Private Helpers
// ============================================================================

void ANPCSpawnManager::InitializePerformanceLimits()
{
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
	NPCSpawnData.Sort([](const FNPCSpawnData& A, const FNPCSpawnData& B)
	{
		// Essential first, then by priority descending
		if (A.bEssential != B.bEssential)
		{
			return A.bEssential > B.bEssential;
		}
		return A.Priority > B.Priority;
	});
}

int32 ANPCSpawnManager::FindNextAvailablePoolEntry() const
{
	for (int32 i = 0; i < NPCPool.Num(); i++)
	{
		if (!NPCPool[i].bInUse)
		{
			return i;
		}
	}
	return -1;
}

void ANPCSpawnManager::ResetNPCPoolEntry(int32 PoolIndex)
{
	if (!NPCPool.IsValidIndex(PoolIndex))
	{
		return;
	}

	FNPCPoolEntry& Entry = NPCPool[PoolIndex];
	Entry.SpawnDataIndex = -1;
	Entry.CurrentWaypointIndex = 0;
	Entry.MoveStartTime = 0.0f;
	Entry.WaitStartTime = 0.0f;
	Entry.bMovingToWaypoint = false;
	Entry.bWaitingAtWaypoint = false;
	Entry.CachedDistanceToPlayer = FLT_MAX;
	Entry.BehaviorLOD = ENPCBehaviorLOD::Dormant;
	Entry.GridCellX = 0;
	Entry.GridCellY = 0;
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

bool ANPCSpawnManager::RefreshPlayerLocation()
{
	APawn* PlayerPawn = GetPlayerPawn();
	if (PlayerPawn)
	{
		CachedPlayerLocation = PlayerPawn->GetActorLocation();
		bPlayerLocationValid = true;
		return true;
	}
	bPlayerLocationValid = false;
	return false;
}

void ANPCSpawnManager::StaggeredUpdate(float DeltaTime)
{
	// --- Distance checks ---
	DistanceCheckTimer += DeltaTime;
	if (DistanceCheckTimer >= DistanceCheckInterval)
	{
		UpdateNPCDistances();
		DistanceCheckTimer = 0.0f;
	}

	// --- Patrol updates (frequency scaled by performance tier) ---
	PatrolUpdateTimer += DeltaTime;
	if (PatrolUpdateTimer >= CurrentLimits.UpdateFrequency)
	{
		UpdateNPCPatrols(DeltaTime);
		PatrolUpdateTimer = 0.0f;
	}

	// --- Spatial grid full rebuild (infrequent, catches drift) ---
	SpatialRebuildTimer += DeltaTime;
	if (SpatialRebuildTimer >= SpatialGridRebuildInterval)
	{
		RebuildSpatialGrid();
		SpatialRebuildTimer = 0.0f;
	}

	// --- Pool validation (every 5 seconds) ---
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
	int32 ActualActiveCount = 0;

	for (int32 i = 0; i < NPCPool.Num(); i++)
	{
		FNPCPoolEntry& Entry = NPCPool[i];

		// Detect and clean up destroyed actors
		if (Entry.bInUse && (!Entry.NPCActor || !IsValid(Entry.NPCActor)))
		{
			UE_LOG(LogTemp, Warning, TEXT("ValidateNPCPool: pool[%d] actor invalid, cleaning up"), i);
			SpatialGrid.Remove(i, Entry.GridCellX, Entry.GridCellY);
			ResetNPCPoolEntry(i);
			Entry.NPCActor = nullptr;
			Entry.bInUse = false;
			Entry.bActive = false;
			Entry.bPreSpawned = false;
			continue;
		}

		if (Entry.bActive)
		{
			ActualActiveCount++;
		}
	}

	// Fix count if it drifted
	if (ActualActiveCount != ActiveNPCCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("ValidateNPCPool: ActiveCount mismatch (tracked=%d actual=%d), correcting"),
			ActiveNPCCount, ActualActiveCount);
		ActiveNPCCount = ActualActiveCount;
	}
}


void ANPCSpawnManager::RebuildSpatialGrid()
{
	SpatialGrid.Clear();

	for (int32 i = 0; i < NPCPool.Num(); i++)
	{
		FNPCPoolEntry& Entry = NPCPool[i];
		if (Entry.bInUse && Entry.NPCActor && IsValid(Entry.NPCActor))
		{
			FVector Loc = Entry.NPCActor->GetActorLocation();
			SpatialGrid.Insert(i, Loc);
			SpatialGrid.WorldToCell(Loc, Entry.GridCellX, Entry.GridCellY);
		}
	}
}

void ANPCSpawnManager::UpdateSpatialGridEntry(int32 PoolIndex)
{
	if (!NPCPool.IsValidIndex(PoolIndex))
	{
		return;
	}

	FNPCPoolEntry& Entry = NPCPool[PoolIndex];
	if (!Entry.NPCActor || !IsValid(Entry.NPCActor))
	{
		return;
	}

	int32 NewCellX, NewCellY;
	SpatialGrid.Move(PoolIndex, Entry.GridCellX, Entry.GridCellY,
		Entry.NPCActor->GetActorLocation(), NewCellX, NewCellY);
	Entry.GridCellX = NewCellX;
	Entry.GridCellY = NewCellY;
}

void ANPCSpawnManager::SetNPCDormant(AOdysseyCharacter* NPC)
{
	if (!NPC || !IsValid(NPC))
	{
		return;
	}

	NPC->SetActorHiddenInGame(true);
	NPC->SetActorEnableCollision(false);
	NPC->SetActorTickEnabled(false);
}

void ANPCSpawnManager::SetNPCVisible(AOdysseyCharacter* NPC, bool bVisible)
{
	if (!NPC || !IsValid(NPC))
	{
		return;
	}

	NPC->SetActorHiddenInGame(!bVisible);
}

void ANPCSpawnManager::ConfigureNPCTickRate(AOdysseyCharacter* NPC, ENPCBehaviorLOD LOD)
{
	if (!NPC || !IsValid(NPC))
	{
		return;
	}

	// Adjust primary tick interval based on LOD
	switch (LOD)
	{
	case ENPCBehaviorLOD::Full:
		NPC->PrimaryActorTick.TickInterval = 0.0f; // Every frame
		break;

	case ENPCBehaviorLOD::Reduced:
		NPC->PrimaryActorTick.TickInterval = 0.1f; // ~10 Hz
		break;

	case ENPCBehaviorLOD::Minimal:
	case ENPCBehaviorLOD::Dormant:
		NPC->SetActorTickEnabled(false);
		break;
	}

	// Also configure the behavior component tick rate if this is an NPCShip
	if (ANPCShip* Ship = Cast<ANPCShip>(NPC))
	{
		if (UNPCBehaviorComponent* Behavior = Ship->GetBehaviorComponent())
		{
			switch (LOD)
			{
			case ENPCBehaviorLOD::Full:
				Behavior->SetComponentTickEnabled(true);
				Behavior->PrimaryComponentTick.TickInterval = 0.0f;
				break;

			case ENPCBehaviorLOD::Reduced:
				Behavior->SetComponentTickEnabled(true);
				Behavior->PrimaryComponentTick.TickInterval = 0.2f; // ~5 Hz
				break;

			case ENPCBehaviorLOD::Minimal:
			case ENPCBehaviorLOD::Dormant:
				Behavior->SetComponentTickEnabled(false);
				break;
			}
		}
	}
}
