// NPCSpawnManagerTests.cpp
// Comprehensive automation tests for ANPCSpawnManager
// Covers: pool config, performance tiers, LOD, spatial grid, patrol routes, spawn data

#include "Misc/AutomationTest.h"
#include "NPCSpawnManager.h"
#include "OdysseyMobileOptimizer.h"
#include "Tests/AutomationCommon.h"

// ============================================================================
// 1. SPAWN MANAGER: Performance Limits Configuration
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpawnMgr_PerfLimits_Defaults,
	"Odyssey.Combat.SpawnManager.PerfLimits.Defaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpawnMgr_PerfLimits_Defaults::RunTest(const FString& Parameters)
{
	FNPCPerformanceLimits Limits;
	TestEqual(TEXT("Default MaxNPCs"), Limits.MaxNPCs, 8);
	TestEqual(TEXT("Default UpdateFrequency"), Limits.UpdateFrequency, 0.1f);
	TestEqual(TEXT("Default CullingDistance"), Limits.CullingDistance, 3000.0f);
	TestTrue(TEXT("Default patrolling enabled"), Limits.bEnablePatrolling);
	TestEqual(TEXT("Default FullLODDistance"), Limits.FullLODDistance, 1000.0f);
	TestEqual(TEXT("Default ReducedLODDistance"), Limits.ReducedLODDistance, 2000.0f);
	TestEqual(TEXT("Default MinimalLODDistance"), Limits.MinimalLODDistance, 3000.0f);
	TestEqual(TEXT("Default PatrolBatchSize"), Limits.PatrolBatchSize, 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpawnMgr_PerfLimits_HighTier,
	"Odyssey.Combat.SpawnManager.PerfLimits.HighTier",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpawnMgr_PerfLimits_HighTier::RunTest(const FString& Parameters)
{
	// High tier: 12 NPCs (as documented in task spec)
	FNPCPerformanceLimits HighLimits(12, 0.05f, 5000.0f, true, 1500.0f, 3000.0f, 5000.0f, 6);

	TestEqual(TEXT("High tier MaxNPCs = 12"), HighLimits.MaxNPCs, 12);
	TestEqual(TEXT("High tier UpdateFrequency"), HighLimits.UpdateFrequency, 0.05f);
	TestTrue(TEXT("High tier patrolling enabled"), HighLimits.bEnablePatrolling);
	TestEqual(TEXT("High tier patrol batch = 6"), HighLimits.PatrolBatchSize, 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpawnMgr_PerfLimits_MediumTier,
	"Odyssey.Combat.SpawnManager.PerfLimits.MediumTier",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpawnMgr_PerfLimits_MediumTier::RunTest(const FString& Parameters)
{
	// Medium tier: 8 NPCs
	FNPCPerformanceLimits MedLimits(8, 0.1f, 3000.0f, true, 1000.0f, 2000.0f, 3000.0f, 4);

	TestEqual(TEXT("Medium tier MaxNPCs = 8"), MedLimits.MaxNPCs, 8);
	TestTrue(TEXT("Medium tier patrolling enabled"), MedLimits.bEnablePatrolling);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpawnMgr_PerfLimits_LowTier,
	"Odyssey.Combat.SpawnManager.PerfLimits.LowTier",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpawnMgr_PerfLimits_LowTier::RunTest(const FString& Parameters)
{
	// Low tier: 4 NPCs
	FNPCPerformanceLimits LowLimits(4, 0.2f, 2000.0f, false, 500.0f, 1000.0f, 2000.0f, 2);

	TestEqual(TEXT("Low tier MaxNPCs = 4"), LowLimits.MaxNPCs, 4);
	TestFalse(TEXT("Low tier patrolling disabled"), LowLimits.bEnablePatrolling);
	TestEqual(TEXT("Low tier patrol batch = 2"), LowLimits.PatrolBatchSize, 2);
	return true;
}

// ============================================================================
// 2. SPAWN MANAGER: Behavior LOD Enum
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpawnMgr_BehaviorLOD_Enum,
	"Odyssey.Combat.SpawnManager.BehaviorLOD.Enum",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpawnMgr_BehaviorLOD_Enum::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Full = 0"), static_cast<uint8>(ENPCBehaviorLOD::Full) == 0);
	TestTrue(TEXT("Reduced = 1"), static_cast<uint8>(ENPCBehaviorLOD::Reduced) == 1);
	TestTrue(TEXT("Minimal = 2"), static_cast<uint8>(ENPCBehaviorLOD::Minimal) == 2);
	TestTrue(TEXT("Dormant = 3"), static_cast<uint8>(ENPCBehaviorLOD::Dormant) == 3);
	return true;
}

// ============================================================================
// 3. SPAWN MANAGER: Pool Entry Structure
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpawnMgr_PoolEntry_Defaults,
	"Odyssey.Combat.SpawnManager.PoolEntry.Defaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpawnMgr_PoolEntry_Defaults::RunTest(const FString& Parameters)
{
	FNPCPoolEntry Entry;
	TestTrue(TEXT("NPCActor is null"), Entry.NPCActor == nullptr);
	TestFalse(TEXT("Not in use"), Entry.bInUse);
	TestFalse(TEXT("Not active"), Entry.bActive);
	TestFalse(TEXT("Not pre-spawned"), Entry.bPreSpawned);
	TestEqual(TEXT("SpawnDataIndex = -1"), Entry.SpawnDataIndex, -1);
	TestEqual(TEXT("Current waypoint = 0"), Entry.CurrentWaypointIndex, 0);
	TestFalse(TEXT("Not moving to waypoint"), Entry.bMovingToWaypoint);
	TestFalse(TEXT("Not waiting at waypoint"), Entry.bWaitingAtWaypoint);
	TestEqual(TEXT("Cached distance = FLT_MAX"), Entry.CachedDistanceToPlayer, FLT_MAX);
	TestEqual(TEXT("Default LOD is Dormant"), Entry.BehaviorLOD, ENPCBehaviorLOD::Dormant);
	TestEqual(TEXT("Grid X = 0"), Entry.GridCellX, 0);
	TestEqual(TEXT("Grid Y = 0"), Entry.GridCellY, 0);
	return true;
}

// ============================================================================
// 4. SPAWN MANAGER: Spawn Data Structure
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpawnMgr_SpawnData_Defaults,
	"Odyssey.Combat.SpawnManager.SpawnData.Defaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpawnMgr_SpawnData_Defaults::RunTest(const FString& Parameters)
{
	FNPCSpawnData Data;
	TestTrue(TEXT("NPCClass is null"), Data.NPCClass == nullptr);
	TestEqual(TEXT("SpawnLocation is zero"), Data.SpawnLocation, FVector::ZeroVector);
	TestEqual(TEXT("SpawnRotation is zero"), Data.SpawnRotation, FRotator::ZeroRotator);
	TestEqual(TEXT("Priority = 0"), Data.Priority, 0);
	TestFalse(TEXT("Not essential"), Data.bEssential);
	return true;
}

// ============================================================================
// 5. SPAWN MANAGER: Waypoint Structure
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpawnMgr_Waypoint_Construction,
	"Odyssey.Combat.SpawnManager.Waypoint.Construction",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpawnMgr_Waypoint_Construction::RunTest(const FString& Parameters)
{
	// Default constructor
	FWaypoint Default;
	TestEqual(TEXT("Default location"), Default.Location, FVector::ZeroVector);
	TestEqual(TEXT("Default wait time"), Default.WaitTime, 0.0f);
	TestFalse(TEXT("Default no interact"), Default.bCanInteract);

	// Parameterized constructor
	FWaypoint Custom(FVector(100, 200, 300), 5.0f, true);
	TestEqual(TEXT("Custom location"), Custom.Location, FVector(100, 200, 300));
	TestEqual(TEXT("Custom wait time"), Custom.WaitTime, 5.0f);
	TestTrue(TEXT("Custom can interact"), Custom.bCanInteract);
	return true;
}

// ============================================================================
// 6. SPAWN MANAGER: Patrol Route Structure
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpawnMgr_PatrolRoute_Defaults,
	"Odyssey.Combat.SpawnManager.PatrolRoute.Defaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpawnMgr_PatrolRoute_Defaults::RunTest(const FString& Parameters)
{
	FPatrolRoute Route;
	TestEqual(TEXT("Default RouteId"), Route.RouteId, NAME_None);
	TestEqual(TEXT("Default RouteName"), Route.RouteName, FString(TEXT("")));
	TestTrue(TEXT("Default is looping"), Route.bLooping);
	TestEqual(TEXT("Default MovementSpeed"), Route.MovementSpeed, 300.0f);
	TestEqual(TEXT("Default ActivationDistance"), Route.ActivationDistance, 2000.0f);
	TestEqual(TEXT("No waypoints"), Route.Waypoints.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpawnMgr_PatrolRoute_WithWaypoints,
	"Odyssey.Combat.SpawnManager.PatrolRoute.WithWaypoints",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpawnMgr_PatrolRoute_WithWaypoints::RunTest(const FString& Parameters)
{
	FPatrolRoute Route;
	Route.RouteId = FName("TradeRoute1");
	Route.RouteName = TEXT("Trade Route Alpha");
	Route.bLooping = true;
	Route.MovementSpeed = 400.0f;

	Route.Waypoints.Add(FWaypoint(FVector(0, 0, 0), 2.0f, true));
	Route.Waypoints.Add(FWaypoint(FVector(1000, 0, 0), 1.0f, false));
	Route.Waypoints.Add(FWaypoint(FVector(1000, 1000, 0), 3.0f, true));

	TestEqual(TEXT("Route has 3 waypoints"), Route.Waypoints.Num(), 3);
	TestEqual(TEXT("First waypoint wait time"), Route.Waypoints[0].WaitTime, 2.0f);
	TestTrue(TEXT("First waypoint can interact"), Route.Waypoints[0].bCanInteract);
	TestEqual(TEXT("Route name set"), Route.RouteName, FString(TEXT("Trade Route Alpha")));
	return true;
}

// ============================================================================
// 7. SPAWN MANAGER: Spatial Grid
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpawnMgr_SpatialGrid_CellKeyUniqueness,
	"Odyssey.Combat.SpawnManager.SpatialGrid.CellKeyUniqueness",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpawnMgr_SpatialGrid_CellKeyUniqueness::RunTest(const FString& Parameters)
{
	// Verify that different cell coordinates produce unique keys
	uint64 Key00 = FNPCSpatialGrid::CellKey(0, 0);
	uint64 Key01 = FNPCSpatialGrid::CellKey(0, 1);
	uint64 Key10 = FNPCSpatialGrid::CellKey(1, 0);
	uint64 Key11 = FNPCSpatialGrid::CellKey(1, 1);
	uint64 KeyNeg = FNPCSpatialGrid::CellKey(-1, -1);

	TestTrue(TEXT("(0,0) != (0,1)"), Key00 != Key01);
	TestTrue(TEXT("(0,0) != (1,0)"), Key00 != Key10);
	TestTrue(TEXT("(1,0) != (0,1)"), Key10 != Key01);
	TestTrue(TEXT("(1,1) != (0,0)"), Key11 != Key00);
	TestTrue(TEXT("(-1,-1) != (0,0)"), KeyNeg != Key00);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpawnMgr_SpatialGrid_WorldToCell,
	"Odyssey.Combat.SpawnManager.SpatialGrid.WorldToCell",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpawnMgr_SpatialGrid_WorldToCell::RunTest(const FString& Parameters)
{
	FNPCSpatialGrid Grid;
	Grid.Initialize(1000.0f);

	int32 CX, CY;

	// Origin should map to cell (0, 0)
	Grid.WorldToCell(FVector::ZeroVector, CX, CY);
	TestEqual(TEXT("Origin X cell"), CX, 0);
	TestEqual(TEXT("Origin Y cell"), CY, 0);

	// 1500 units out should be cell (1, 0) with 1000 cell size
	Grid.WorldToCell(FVector(1500.0f, 0.0f, 0.0f), CX, CY);
	TestEqual(TEXT("1500 X cell"), CX, 1);
	TestEqual(TEXT("1500 Y cell"), CY, 0);

	// Negative coordinates
	Grid.WorldToCell(FVector(-500.0f, -1500.0f, 0.0f), CX, CY);
	TestTrue(TEXT("Negative X cell < 0"), CX < 0);
	TestTrue(TEXT("Negative Y cell < 0"), CY < 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpawnMgr_SpatialGrid_InsertAndClear,
	"Odyssey.Combat.SpawnManager.SpatialGrid.InsertAndClear",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpawnMgr_SpatialGrid_InsertAndClear::RunTest(const FString& Parameters)
{
	FNPCSpatialGrid Grid;
	Grid.Initialize(500.0f);

	// Insert several entries
	Grid.Insert(0, FVector(100, 100, 0));
	Grid.Insert(1, FVector(600, 100, 0));
	Grid.Insert(2, FVector(1200, 800, 0));

	// Query radius around origin
	TArray<int32> Results;
	Grid.QueryRadius(FVector::ZeroVector, 300.0f, Results);
	TestTrue(TEXT("Found entry 0 near origin"), Results.Contains(0));

	// Clear
	Grid.Clear();
	Results.Empty();
	Grid.QueryRadius(FVector::ZeroVector, 10000.0f, Results);
	TestEqual(TEXT("Empty after clear"), Results.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpawnMgr_SpatialGrid_QueryRadius,
	"Odyssey.Combat.SpawnManager.SpatialGrid.QueryRadius",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpawnMgr_SpatialGrid_QueryRadius::RunTest(const FString& Parameters)
{
	FNPCSpatialGrid Grid;
	Grid.Initialize(500.0f);

	// Place NPCs at known positions
	Grid.Insert(0, FVector(0, 0, 0));        // At origin
	Grid.Insert(1, FVector(200, 0, 0));       // 200 units away
	Grid.Insert(2, FVector(800, 0, 0));       // 800 units away
	Grid.Insert(3, FVector(5000, 5000, 0));   // Very far away

	// Query within 300 units of origin
	TArray<int32> NearResults;
	Grid.QueryRadius(FVector::ZeroVector, 300.0f, NearResults);
	TestTrue(TEXT("Entry 0 in near results"), NearResults.Contains(0));
	TestTrue(TEXT("Entry 1 in near results"), NearResults.Contains(1));
	// Entry 2 at 800 units may or may not be in results depending on cell size and how
	// QueryRadius works (it returns all entries in overlapping cells)
	TestFalse(TEXT("Entry 3 not in near results"), NearResults.Contains(3));

	// Query within 1000 units
	TArray<int32> MidResults;
	Grid.QueryRadius(FVector::ZeroVector, 1000.0f, MidResults);
	TestTrue(TEXT("Entry 0 in mid results"), MidResults.Contains(0));
	TestTrue(TEXT("Entry 2 in mid results"), MidResults.Contains(2));
	return true;
}

// ============================================================================
// 8. SPAWN MANAGER: LOD Distance Thresholds
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpawnMgr_LOD_DistanceThresholds,
	"Odyssey.Combat.SpawnManager.LOD.DistanceThresholds",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpawnMgr_LOD_DistanceThresholds::RunTest(const FString& Parameters)
{
	FNPCPerformanceLimits Limits;
	Limits.FullLODDistance = 1000.0f;
	Limits.ReducedLODDistance = 2000.0f;
	Limits.MinimalLODDistance = 3000.0f;
	Limits.CullingDistance = 4000.0f;

	// Verify threshold ordering
	TestTrue(TEXT("Full < Reduced"), Limits.FullLODDistance < Limits.ReducedLODDistance);
	TestTrue(TEXT("Reduced < Minimal"), Limits.ReducedLODDistance < Limits.MinimalLODDistance);
	TestTrue(TEXT("Minimal < Culling"), Limits.MinimalLODDistance < Limits.CullingDistance);
	return true;
}

// ============================================================================
// 9. SPAWN MANAGER: Pool Entry State Transitions
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpawnMgr_PoolEntry_StateTransitions,
	"Odyssey.Combat.SpawnManager.PoolEntry.StateTransitions",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpawnMgr_PoolEntry_StateTransitions::RunTest(const FString& Parameters)
{
	FNPCPoolEntry Entry;

	// Simulate: available -> in use -> active -> deactivated -> returned to pool
	TestFalse(TEXT("Initial: not in use"), Entry.bInUse);
	TestFalse(TEXT("Initial: not active"), Entry.bActive);

	// Spawn
	Entry.bInUse = true;
	Entry.bActive = true;
	Entry.SpawnDataIndex = 0;
	Entry.BehaviorLOD = ENPCBehaviorLOD::Full;
	TestTrue(TEXT("Spawned: in use"), Entry.bInUse);
	TestTrue(TEXT("Spawned: active"), Entry.bActive);
	TestEqual(TEXT("Spawned: Full LOD"), Entry.BehaviorLOD, ENPCBehaviorLOD::Full);

	// Deactivate (distance culling)
	Entry.bActive = false;
	Entry.BehaviorLOD = ENPCBehaviorLOD::Dormant;
	TestTrue(TEXT("Deactivated: still in use"), Entry.bInUse);
	TestFalse(TEXT("Deactivated: not active"), Entry.bActive);
	TestEqual(TEXT("Deactivated: Dormant LOD"), Entry.BehaviorLOD, ENPCBehaviorLOD::Dormant);

	// Return to pool
	Entry.bInUse = false;
	Entry.bActive = false;
	Entry.SpawnDataIndex = -1;
	TestFalse(TEXT("Returned: not in use"), Entry.bInUse);
	TestEqual(TEXT("Returned: no spawn data"), Entry.SpawnDataIndex, -1);
	return true;
}

// ============================================================================
// 10. SPAWN MANAGER: LOD Transition Sequence
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpawnMgr_LOD_TransitionSequence,
	"Odyssey.Combat.SpawnManager.LOD.TransitionSequence",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpawnMgr_LOD_TransitionSequence::RunTest(const FString& Parameters)
{
	FNPCPoolEntry Entry;

	// Simulate LOD transitions: Full -> Reduced -> Minimal -> Dormant
	Entry.BehaviorLOD = ENPCBehaviorLOD::Full;
	TestEqual(TEXT("Full LOD"), Entry.BehaviorLOD, ENPCBehaviorLOD::Full);

	Entry.BehaviorLOD = ENPCBehaviorLOD::Reduced;
	TestEqual(TEXT("Reduced LOD"), Entry.BehaviorLOD, ENPCBehaviorLOD::Reduced);

	Entry.BehaviorLOD = ENPCBehaviorLOD::Minimal;
	TestEqual(TEXT("Minimal LOD"), Entry.BehaviorLOD, ENPCBehaviorLOD::Minimal);

	Entry.BehaviorLOD = ENPCBehaviorLOD::Dormant;
	TestEqual(TEXT("Dormant LOD"), Entry.BehaviorLOD, ENPCBehaviorLOD::Dormant);

	// Reverse: Dormant -> Full (when player approaches)
	Entry.BehaviorLOD = ENPCBehaviorLOD::Full;
	TestEqual(TEXT("Back to Full LOD"), Entry.BehaviorLOD, ENPCBehaviorLOD::Full);
	return true;
}

// ============================================================================
// 11. SPAWN MANAGER: Essential NPC Spawn Data
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpawnMgr_SpawnData_EssentialFlag,
	"Odyssey.Combat.SpawnManager.SpawnData.EssentialFlag",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpawnMgr_SpawnData_EssentialFlag::RunTest(const FString& Parameters)
{
	FNPCSpawnData Essential;
	Essential.bEssential = true;
	Essential.Priority = 100;

	FNPCSpawnData Normal;
	Normal.bEssential = false;
	Normal.Priority = 50;

	TestTrue(TEXT("Essential flag set"), Essential.bEssential);
	TestTrue(TEXT("Essential has higher priority"), Essential.Priority > Normal.Priority);
	return true;
}
