// NPCSpawnManager.h
// Mobile-optimized NPC lifecycle manager with zero-allocation object pooling,
// spatial partitioning, distance-based behavior LOD, and performance tier scaling.
// Phase 4: NPC Spawning & Performance (Task #12)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "OdysseyMobileOptimizer.h"
#include "NPCSpawnManager.generated.h"

class ANPCShip;
class AOdysseyCharacter;
class UOdysseyMobileOptimizer;

// ============================================================================
// Enumerations
// ============================================================================

/**
 * Behavior detail level driven by distance to player.
 * Higher LOD = less computation.
 */
UENUM(BlueprintType)
enum class ENPCBehaviorLOD : uint8
{
	/** Full AI: tick every frame, detection, patrol, combat. */
	Full = 0,
	/** Reduced AI: tick at lower frequency, simplified patrol only. */
	Reduced = 1,
	/** Minimal AI: visual presence only, no logic tick. */
	Minimal = 2,
	/** Dormant: hidden, collision off, no tick. */
	Dormant = 3
};

// ============================================================================
// Data Structures
// ============================================================================

/**
 * Single waypoint in a patrol route.
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FWaypoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint")
	FVector Location;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint")
	float WaitTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint")
	bool bCanInteract;

	FWaypoint()
		: Location(FVector::ZeroVector)
		, WaitTime(0.0f)
		, bCanInteract(false)
	{
	}

	FWaypoint(const FVector& InLocation, float InWaitTime = 0.0f, bool InCanInteract = false)
		: Location(InLocation)
		, WaitTime(InWaitTime)
		, bCanInteract(InCanInteract)
	{
	}
};

/**
 * Named patrol route that can be shared across multiple NPCs.
 * Routes are stored in a registry to avoid duplication.
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FPatrolRoute
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	FName RouteId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	FString RouteName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	TArray<FWaypoint> Waypoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	bool bLooping;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	float MovementSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	float ActivationDistance;

	FPatrolRoute()
		: RouteId(NAME_None)
		, RouteName(TEXT(""))
		, bLooping(true)
		, MovementSpeed(300.0f)
		, ActivationDistance(2000.0f)
	{
	}
};

/**
 * Spawn definition for a single NPC slot.
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FNPCSpawnData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Data")
	TSubclassOf<AOdysseyCharacter> NPCClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Data")
	FVector SpawnLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Data")
	FRotator SpawnRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Data")
	FPatrolRoute PatrolRoute;

	/** Higher priority NPCs spawn first on limited devices. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Data")
	int32 Priority;

	/** Always spawn, regardless of performance tier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Data")
	bool bEssential;

	FNPCSpawnData()
		: NPCClass(nullptr)
		, SpawnLocation(FVector::ZeroVector)
		, SpawnRotation(FRotator::ZeroRotator)
		, Priority(0)
		, bEssential(false)
	{
	}
};

/**
 * Object pool entry tracking a single NPC's runtime state.
 * Designed for cache-friendly iteration: all hot data is packed here
 * so the pool array can be iterated without pointer chasing.
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FNPCPoolEntry
{
	GENERATED_BODY()

	// --- Actor reference ---
	UPROPERTY()
	AOdysseyCharacter* NPCActor;

	// --- Pool state flags ---
	UPROPERTY()
	bool bInUse;

	UPROPERTY()
	bool bActive;

	UPROPERTY()
	bool bPreSpawned;

	// --- Spawn data link ---
	UPROPERTY()
	int32 SpawnDataIndex;

	// --- Patrol state ---
	UPROPERTY()
	int32 CurrentWaypointIndex;

	UPROPERTY()
	float MoveStartTime;

	UPROPERTY()
	float WaitStartTime;

	UPROPERTY()
	bool bMovingToWaypoint;

	UPROPERTY()
	bool bWaitingAtWaypoint;

	// --- Distance / LOD state (updated during distance pass) ---
	UPROPERTY()
	float CachedDistanceToPlayer;

	UPROPERTY()
	ENPCBehaviorLOD BehaviorLOD;

	// --- Spatial grid coordinates ---
	int32 GridCellX;
	int32 GridCellY;

	FNPCPoolEntry()
		: NPCActor(nullptr)
		, bInUse(false)
		, bActive(false)
		, bPreSpawned(false)
		, SpawnDataIndex(-1)
		, CurrentWaypointIndex(0)
		, MoveStartTime(0.0f)
		, WaitStartTime(0.0f)
		, bMovingToWaypoint(false)
		, bWaitingAtWaypoint(false)
		, CachedDistanceToPlayer(FLT_MAX)
		, BehaviorLOD(ENPCBehaviorLOD::Dormant)
		, GridCellX(0)
		, GridCellY(0)
	{
	}
};

/**
 * Per-tier performance limits. Tuned for 60 fps targets.
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FNPCPerformanceLimits
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Limits")
	int32 MaxNPCs;

	/** Seconds between distance checks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Limits")
	float UpdateFrequency;

	/** NPCs farther than this become Dormant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Limits")
	float CullingDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Limits")
	bool bEnablePatrolling;

	/** Distance thresholds for behavior LOD transitions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Limits")
	float FullLODDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Limits")
	float ReducedLODDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Limits")
	float MinimalLODDistance;

	/** How many NPCs to update per patrol tick (stagger budget). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Limits")
	int32 PatrolBatchSize;

	FNPCPerformanceLimits()
		: MaxNPCs(8)
		, UpdateFrequency(0.1f)
		, CullingDistance(3000.0f)
		, bEnablePatrolling(true)
		, FullLODDistance(1000.0f)
		, ReducedLODDistance(2000.0f)
		, MinimalLODDistance(3000.0f)
		, PatrolBatchSize(4)
	{
	}

	FNPCPerformanceLimits(int32 InMaxNPCs, float InUpdateFreq, float InCullingDist, bool InEnablePatrol,
		float InFullLOD, float InReducedLOD, float InMinimalLOD, int32 InPatrolBatch)
		: MaxNPCs(InMaxNPCs)
		, UpdateFrequency(InUpdateFreq)
		, CullingDistance(InCullingDist)
		, bEnablePatrolling(InEnablePatrol)
		, FullLODDistance(InFullLOD)
		, ReducedLODDistance(InReducedLOD)
		, MinimalLODDistance(InMinimalLOD)
		, PatrolBatchSize(InPatrolBatch)
	{
	}
};

// ============================================================================
// Spatial Grid
// ============================================================================

/**
 * Lightweight spatial hash grid for O(1) neighbour queries.
 * Used to find NPCs near the player without iterating the full pool.
 * Each cell stores indices into the NPC pool array.
 */
struct FNPCSpatialGrid
{
	/** World-space size of each grid cell (units). */
	float CellSize;

	/** Cell storage: hashed cell coordinate -> pool indices. */
	TMap<uint64, TArray<int32>> Cells;

	FNPCSpatialGrid()
		: CellSize(1000.0f)
	{
	}

	void Initialize(float InCellSize);
	void Clear();
	void Insert(int32 PoolIndex, const FVector& WorldLocation);
	void Remove(int32 PoolIndex, int32 CellX, int32 CellY);
	void Move(int32 PoolIndex, int32 OldCellX, int32 OldCellY, const FVector& NewWorldLocation, int32& OutNewCellX, int32& OutNewCellY);

	/** Gather all pool indices within a radius of the given world position. */
	void QueryRadius(const FVector& Center, float Radius, TArray<int32>& OutIndices) const;

	/** Convert world coordinates to cell coordinates. */
	void WorldToCell(const FVector& WorldLocation, int32& OutCellX, int32& OutCellY) const;

	/** Pack cell coords into a single uint64 key. */
	static uint64 CellKey(int32 CellX, int32 CellY);
};

// ============================================================================
// ANPCSpawnManager
// ============================================================================

/**
 * Mobile-optimized NPC lifecycle manager.
 *
 * Features:
 * - Zero-allocation object pool with pre-spawned NPC ship actors
 * - Grid-based spatial partitioning for distance queries
 * - Distance-based behavior LOD (Full / Reduced / Minimal / Dormant)
 * - Performance tier integration (High 12, Medium 8, Low 4)
 * - Staggered update scheduling to spread CPU cost across frames
 * - Shared patrol route registry for memory efficiency
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ODYSSEY_API ANPCSpawnManager : public AActor
{
	GENERATED_BODY()

public:
	ANPCSpawnManager();

public:
	// ================================================================
	// Configuration
	// ================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Management")
	TArray<FNPCSpawnData> NPCSpawnData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Management")
	TSubclassOf<ANPCShip> DefaultNPCShipClass;

	// --- Performance tier presets ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	FNPCPerformanceLimits HighPerformanceLimits;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	FNPCPerformanceLimits MediumPerformanceLimits;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	FNPCPerformanceLimits LowPerformanceLimits;

	// --- Object pool sizing ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Pool")
	int32 MaxPoolSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Pool")
	int32 InitialPoolSize;

	/** If true, actors are spawned up-front in BeginPlay. Eliminates all runtime allocation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Pool")
	bool bPreSpawnPoolActors;

	// --- Spatial grid ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial")
	float SpatialGridCellSize;

	// --- Update intervals ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Updates")
	float DistanceCheckInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Updates")
	float PatrolUpdateInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Updates")
	float SpatialGridRebuildInterval;


protected:

	// ================================================================
	// Runtime State
	// ================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Object Pool")
	TArray<FNPCPoolEntry> NPCPool;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Performance")
	UOdysseyMobileOptimizer* MobileOptimizer;

	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	EPerformanceTier CurrentPerformanceTier;

	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	FNPCPerformanceLimits CurrentLimits;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	int32 ActiveNPCCount;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	float LastUpdateTime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	bool bInitialized;

	// --- Shared patrol route registry ---
	UPROPERTY()
	TMap<FName, FPatrolRoute> PatrolRouteRegistry;

	// --- Spatial partitioning ---
	FNPCSpatialGrid SpatialGrid;

	// --- Timers / stagger state ---
	float DistanceCheckTimer;
	float PatrolUpdateTimer;
	float SpatialRebuildTimer;
	int32 PatrolStaggerCursor;      // Round-robin index for staggered patrol updates
	int32 DistanceStaggerCursor;    // Round-robin for distance checks when pool > batch size

	// --- Cached player location (refreshed each distance pass) ---
	FVector CachedPlayerLocation;
	bool bPlayerLocationValid;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;

	// ================================================================
	// Initialization / Shutdown
	// ================================================================

	UFUNCTION(BlueprintCallable, Category = "NPC Management")
	void InitializeNPCSystem();

	UFUNCTION(BlueprintCallable, Category = "NPC Management")
	void ShutdownNPCSystem();

	// ================================================================
	// Object Pool
	// ================================================================

	UFUNCTION(BlueprintCallable, Category = "Object Pool")
	void InitializeNPCPool();

	UFUNCTION(BlueprintCallable, Category = "Object Pool")
	FNPCPoolEntry* GetPooledNPC();

	UFUNCTION(BlueprintCallable, Category = "Object Pool")
	void ReturnNPCToPool(int32 PoolIndex);

	UFUNCTION(BlueprintCallable, Category = "Object Pool")
	void ExpandPool(int32 AdditionalSize);

	// ================================================================
	// Spawning
	// ================================================================

	UFUNCTION(BlueprintCallable, Category = "NPC Management")
	bool SpawnNPC(int32 SpawnDataIndex);

	UFUNCTION(BlueprintCallable, Category = "NPC Management")
	void DespawnNPC(int32 PoolIndex);

	UFUNCTION(BlueprintCallable, Category = "NPC Management")
	void ActivateNPC(int32 PoolIndex);

	UFUNCTION(BlueprintCallable, Category = "NPC Management")
	void DeactivateNPC(int32 PoolIndex);

	// ================================================================
	// Performance Tier
	// ================================================================

	UFUNCTION(BlueprintCallable, Category = "Performance")
	void UpdatePerformanceSettings();

	UFUNCTION(BlueprintCallable, Category = "Performance")
	void OnPerformanceTierChanged(EPerformanceTier NewTier);

	UFUNCTION(BlueprintCallable, Category = "Performance")
	void OptimizeNPCCount();

	UFUNCTION(BlueprintCallable, Category = "Performance")
	int32 GetMaxNPCsForCurrentTier() const;

	// ================================================================
	// Distance & LOD
	// ================================================================

	UFUNCTION(BlueprintCallable, Category = "Culling")
	void UpdateNPCDistances();

	UFUNCTION(BlueprintCallable, Category = "Culling")
	void CullDistantNPCs();

	UFUNCTION(BlueprintCallable, Category = "Culling")
	void ActivateNearbyNPCs();

	UFUNCTION(BlueprintCallable, Category = "Culling")
	float GetDistanceToPlayer(const FVector& NPCLocation) const;

	/** Compute behavior LOD for a given distance. */
	UFUNCTION(BlueprintCallable, Category = "Culling")
	ENPCBehaviorLOD ComputeBehaviorLOD(float Distance) const;

	/** Apply behavior LOD to a pool entry (adjusts tick rate, visibility, etc). */
	void ApplyBehaviorLOD(int32 PoolIndex, ENPCBehaviorLOD NewLOD);

	// ================================================================
	// Patrol System
	// ================================================================

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	void UpdateNPCPatrols(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	void UpdateNPCPatrol(int32 PoolIndex, float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	void MoveNPCToWaypoint(int32 PoolIndex, const FWaypoint& Waypoint);

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	bool IsNPCAtWaypoint(int32 PoolIndex, const FWaypoint& Waypoint) const;

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	FWaypoint GetNextWaypoint(int32 PoolIndex) const;

	// --- Patrol route registry ---

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	void RegisterPatrolRoute(const FPatrolRoute& Route);

	UFUNCTION(BlueprintCallable, Category = "Patrol")
	const FPatrolRoute* GetPatrolRoute(FName RouteId) const;

	// ================================================================
	// Queries
	// ================================================================

	UFUNCTION(BlueprintCallable, Category = "Utility")
	int32 GetActiveNPCCount() const { return ActiveNPCCount; }

	UFUNCTION(BlueprintCallable, Category = "Utility")
	int32 GetPoolSize() const { return NPCPool.Num(); }

	UFUNCTION(BlueprintCallable, Category = "Utility")
	bool IsNPCActive(int32 PoolIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Utility")
	AOdysseyCharacter* GetNPCFromPool(int32 PoolIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Utility")
	TArray<AOdysseyCharacter*> GetActiveNPCs() const;

	/** Return pool indices of NPCs within radius of a world position using the spatial grid. */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	TArray<int32> GetNPCsInRadius(const FVector& Center, float Radius) const;

	UFUNCTION(BlueprintCallable, Category = "Utility")
	int32 GetPoolEntriesInUse() const;

	UFUNCTION(BlueprintCallable, Category = "Utility")
	ENPCBehaviorLOD GetNPCBehaviorLOD(int32 PoolIndex) const;

	// ================================================================
	// Debug
	// ================================================================

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void DebugDrawPatrolRoutes();

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void DebugDrawNPCStates();

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void DebugDrawSpatialGrid();

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void LogNPCSystemState();

	// ================================================================
	// Blueprint Events
	// ================================================================

	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnNPCSpawned(AOdysseyCharacter* NPC, int32 SpawnDataIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnNPCDespawned(AOdysseyCharacter* NPC, int32 SpawnDataIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnNPCActivated(AOdysseyCharacter* NPC);

	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnNPCDeactivated(AOdysseyCharacter* NPC);

	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnPerformanceOptimized(EPerformanceTier Tier, int32 ActiveNPCs);

	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnNPCBehaviorLODChanged(AOdysseyCharacter* NPC, ENPCBehaviorLOD OldLOD, ENPCBehaviorLOD NewLOD);

private:
	// --- Internal helpers ---
	void InitializePerformanceLimits();
	FNPCPerformanceLimits GetLimitsForTier(EPerformanceTier Tier) const;
	bool CanSpawnMoreNPCs() const;
	void SortNPCSpawnDataByPriority();
	int32 FindNextAvailablePoolEntry() const;
	void ResetNPCPoolEntry(int32 PoolIndex);
	APawn* GetPlayerPawn() const;
	bool RefreshPlayerLocation();
	void StaggeredUpdate(float DeltaTime);
	bool IsWithinActivationDistance(const FVector& NPCLocation) const;
	void ValidateNPCPool();
	void PreSpawnPoolActors();
	void RebuildSpatialGrid();
	void UpdateSpatialGridEntry(int32 PoolIndex);
	void SetNPCDormant(AOdysseyCharacter* NPC);
	void SetNPCVisible(AOdysseyCharacter* NPC, bool bVisible);
	void ConfigureNPCTickRate(AOdysseyCharacter* NPC, ENPCBehaviorLOD LOD);
};
