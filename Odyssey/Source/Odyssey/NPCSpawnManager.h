#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "OdysseyMobileOptimizer.h"
#include "NPCSpawnManager.generated.h"

class AOdysseyCharacter;
class UOdysseyMobileOptimizer;

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
	{
		Location = FVector::ZeroVector;
		WaitTime = 0.0f;
		bCanInteract = false;
	}

	FWaypoint(const FVector& InLocation, float InWaitTime = 0.0f, bool InCanInteract = false)
		: Location(InLocation)
		, WaitTime(InWaitTime)
		, bCanInteract(InCanInteract)
	{
	}
};

USTRUCT(BlueprintType)
struct ODYSSEY_API FPatrolRoute
{
	GENERATED_BODY()

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
	{
		RouteName = TEXT("");
		bLooping = true;
		MovementSpeed = 300.0f;
		ActivationDistance = 2000.0f;
	}
};

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Data")
	int32 Priority; // Higher priority NPCs spawn first on limited devices

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Data")
	bool bEssential; // Always spawn, regardless of performance tier

	FNPCSpawnData()
	{
		NPCClass = nullptr;
		SpawnLocation = FVector::ZeroVector;
		SpawnRotation = FRotator::ZeroRotator;
		Priority = 0;
		bEssential = false;
	}
};

USTRUCT(BlueprintType)
struct ODYSSEY_API FNPCPoolEntry
{
	GENERATED_BODY()

	UPROPERTY()
	AOdysseyCharacter* NPCActor;

	UPROPERTY()
	bool bInUse;

	UPROPERTY()
	bool bActive;

	UPROPERTY()
	int32 SpawnDataIndex;

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

	FNPCPoolEntry()
	{
		NPCActor = nullptr;
		bInUse = false;
		bActive = false;
		SpawnDataIndex = -1;
		CurrentWaypointIndex = 0;
		MoveStartTime = 0.0f;
		WaitStartTime = 0.0f;
		bMovingToWaypoint = false;
		bWaitingAtWaypoint = false;
	}
};

USTRUCT(BlueprintType)
struct ODYSSEY_API FNPCPerformanceLimits
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Limits")
	int32 MaxNPCs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Limits")
	float UpdateFrequency;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Limits")
	float CullingDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Limits")
	bool bEnablePatrolling;

	FNPCPerformanceLimits()
	{
		MaxNPCs = 8;
		UpdateFrequency = 0.1f;
		CullingDistance = 3000.0f;
		bEnablePatrolling = true;
	}

	FNPCPerformanceLimits(int32 InMaxNPCs, float InUpdateFreq, float InCullingDist, bool InEnablePatrol)
		: MaxNPCs(InMaxNPCs)
		, UpdateFrequency(InUpdateFreq)
		, CullingDistance(InCullingDist)
		, bEnablePatrolling(InEnablePatrol)
	{
	}
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ODYSSEY_API ANPCSpawnManager : public AActor
{
	GENERATED_BODY()

public:
	ANPCSpawnManager();

protected:
	// Core settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Management")
	TArray<FNPCSpawnData> NPCSpawnData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	FNPCPerformanceLimits HighPerformanceLimits;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	FNPCPerformanceLimits MediumPerformanceLimits;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	FNPCPerformanceLimits LowPerformanceLimits;

	// Pool management
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Object Pool")
	TArray<FNPCPoolEntry> NPCPool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Pool")
	int32 MaxPoolSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Object Pool")
	int32 InitialPoolSize;

	// Performance integration
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Performance")
	UOdysseyMobileOptimizer* MobileOptimizer;

	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	EPerformanceTier CurrentPerformanceTier;

	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	FNPCPerformanceLimits CurrentLimits;

	// Runtime state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	int32 ActiveNPCCount;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	float LastUpdateTime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
	bool bInitialized;

	// Update management
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Updates")
	float DistanceCheckInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Updates")
	float PatrolUpdateInterval;

	// Timers
	float DistanceCheckTimer;
	float PatrolUpdateTimer;
	int32 CurrentUpdateIndex; // For staggered updates

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// Initialization
	UFUNCTION(BlueprintCallable, Category = "NPC Management")
	void InitializeNPCSystem();

	UFUNCTION(BlueprintCallable, Category = "NPC Management")
	void ShutdownNPCSystem();

	// Pool management
	UFUNCTION(BlueprintCallable, Category = "Object Pool")
	void InitializeNPCPool();

	UFUNCTION(BlueprintCallable, Category = "Object Pool")
	FNPCPoolEntry* GetPooledNPC();

	UFUNCTION(BlueprintCallable, Category = "Object Pool")
	void ReturnNPCToPool(int32 PoolIndex);

	UFUNCTION(BlueprintCallable, Category = "Object Pool")
	void ExpandPool(int32 AdditionalSize);

	// NPC spawning and management
	UFUNCTION(BlueprintCallable, Category = "NPC Management")
	bool SpawnNPC(int32 SpawnDataIndex);

	UFUNCTION(BlueprintCallable, Category = "NPC Management")
	void DespawnNPC(int32 PoolIndex);

	UFUNCTION(BlueprintCallable, Category = "NPC Management")
	void ActivateNPC(int32 PoolIndex);

	UFUNCTION(BlueprintCallable, Category = "NPC Management")
	void DeactivateNPC(int32 PoolIndex);

	// Performance management
	UFUNCTION(BlueprintCallable, Category = "Performance")
	void UpdatePerformanceSettings();

	UFUNCTION(BlueprintCallable, Category = "Performance")
	void OnPerformanceTierChanged(EPerformanceTier NewTier);

	UFUNCTION(BlueprintCallable, Category = "Performance")
	void OptimizeNPCCount();

	UFUNCTION(BlueprintCallable, Category = "Performance")
	int32 GetMaxNPCsForCurrentTier() const;

	// Distance-based culling
	UFUNCTION(BlueprintCallable, Category = "Culling")
	void UpdateNPCDistances();

	UFUNCTION(BlueprintCallable, Category = "Culling")
	void CullDistantNPCs();

	UFUNCTION(BlueprintCallable, Category = "Culling")
	void ActivateNearbyNPCs();

	UFUNCTION(BlueprintCallable, Category = "Culling")
	float GetDistanceToPlayer(const FVector& NPCLocation) const;

	// Patrol system
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

	// Utility functions
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

	// Debug functions
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void DebugDrawPatrolRoutes();

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void DebugDrawNPCStates();

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void LogNPCSystemState();

	// Events
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

private:
	// Internal helper functions
	void InitializePerformanceLimits();
	FNPCPerformanceLimits GetLimitsForTier(EPerformanceTier Tier) const;
	bool CanSpawnMoreNPCs() const;
	void SortNPCSpawnDataByPriority();
	int32 FindNextAvailablePoolEntry() const;
	void ResetNPCPoolEntry(int32 PoolIndex);
	APawn* GetPlayerPawn() const;
	void StaggeredUpdate(float DeltaTime);
	bool IsWithinActivationDistance(const FVector& NPCLocation) const;
	void ValidateNPCPool();
};
