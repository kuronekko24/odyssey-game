# NPC Spawning & Performance System (Phase 4 - Task #12)

## Overview

Mobile-optimized NPC lifecycle manager for Odyssey with zero-allocation object pooling,
grid-based spatial partitioning, distance-based behavior LOD, and performance tier scaling.
Designed to maintain 60fps across all mobile device tiers.

## Architecture

```
ANPCSpawnManager
├── Object Pool (TArray<FNPCPoolEntry>)
│   ├── Pre-spawned ANPCShip actors (zero runtime allocation)
│   └── Pool entries track state, LOD, spatial grid coords
├── Spatial Grid (FNPCSpatialGrid)
│   ├── Hash-map based O(1) cell lookup
│   └── Radius queries for distance-based culling
├── Performance Tier Integration (UOdysseyMobileOptimizer)
│   ├── High:   12 NPCs, 5000u culling, full LOD to 1500u
│   ├── Medium:  8 NPCs, 3500u culling, full LOD to 1000u
│   └── Low:     4 NPCs, 2500u culling, no patrol, tight LOD
├── Behavior LOD System
│   ├── Full:    every-frame tick, full AI, collision
│   ├── Reduced: 10Hz tick, simplified patrol, collision
│   ├── Minimal: visible only, no tick, no collision
│   └── Dormant: hidden, no tick, no collision
└── Patrol Route System
    ├── Waypoint navigation with wait times
    ├── Shared route registry (FName-keyed)
    └── Staggered batch updates per frame
```

## Key Features

### 1. Zero-Allocation Object Pool
- Pool actors are pre-spawned in `BeginPlay()` at a hidden location
- `GetPooledNPC()` returns a pre-allocated actor - no `SpawnActor` at runtime
- `ReturnNPCToPool()` hides and repositions the actor without destroying it
- Pool expands on demand up to `MaxPoolSize`, with immediate pre-spawn for new slots
- Eliminates GC pressure and allocation stalls during gameplay

### 2. Performance Tier Integration
Automatically detects tier from `UOdysseyMobileOptimizer` and applies limits:

| Setting             | High   | Medium | Low    |
|---------------------|--------|--------|--------|
| Max NPCs            | 12     | 8      | 4      |
| Update Frequency    | 0.05s  | 0.1s   | 0.2s   |
| Culling Distance    | 5000u  | 3500u  | 2500u  |
| Patrolling          | Yes    | Yes    | No     |
| Full LOD Distance   | 1500u  | 1000u  | 600u   |
| Reduced LOD Distance| 3000u  | 2000u  | 1200u  |
| Patrol Batch Size   | 6      | 4      | 2      |

Tier changes trigger `OptimizeNPCCount()` which deactivates farthest-first or
activates closest-first to reach the new target.

### 3. Distance-Based Behavior LOD
Four tiers of NPC behavior complexity driven by distance to the player:

- **Full** (< FullLODDistance): Every-frame tick, full AI behavior, collision enabled.
  NPC behavior component runs at full update rate.
- **Reduced** (< ReducedLODDistance): 10Hz actor tick, 5Hz behavior component tick,
  simplified patrol (skip every other stagger update), collision enabled.
- **Minimal** (< MinimalLODDistance): Actor visible but tick and collision disabled.
  Visual presence only. Cheapest "alive" state.
- **Dormant** (beyond MinimalLODDistance): Hidden, no tick, no collision. Effectively
  free. Used for culled NPCs that remain in the pool for reactivation.

LOD transitions happen during the staggered distance check pass.

### 4. Spatial Partitioning
Grid-based spatial hash for efficient neighbour queries:

- Configurable cell size (default 1000 units)
- `QueryRadius()` touches only cells overlapping the search area
- NPC grid positions updated during staggered distance checks
- Full grid rebuild every 5 seconds to catch accumulated drift
- Used by `ActivateNearbyNPCs()` to avoid iterating the full pool

### 5. Staggered Update Scheduling
CPU cost is spread across multiple frames:

- **Distance checks**: Process 1/3 of the pool per check interval
- **Patrol updates**: Process `PatrolBatchSize` NPCs per patrol interval
- **Spatial grid rebuild**: Full rebuild every 5 seconds
- **Pool validation**: Every 5 seconds, detect destroyed actors and fix count drift

### 6. Patrol Route System
- Waypoint-based navigation with configurable wait times per point
- Looping and non-looping routes
- Shared route registry (`FName` keyed) for memory-efficient route reuse
- Patrol disabled automatically on Low performance tier
- Reduced LOD NPCs get half-rate patrol updates

### 7. Priority & Essential NPC System
- `Priority` field (higher = spawns first on constrained devices)
- `bEssential` flag: NPC always active regardless of tier limits or distance
- `OptimizeNPCCount()` deactivates non-essential farthest-first, reactivates closest-first

## Core Classes

### ANPCSpawnManager (AActor)
Main lifecycle manager. Place one in the level.

**Configuration:**
- `NPCSpawnData`: Array of `FNPCSpawnData` spawn definitions
- `DefaultNPCShipClass`: Class for pre-spawned pool actors
- `High/Medium/LowPerformanceLimits`: Per-tier settings
- `MaxPoolSize` / `InitialPoolSize`: Pool sizing
- `bPreSpawnPoolActors`: Enable zero-allocation mode (default true)
- `SpatialGridCellSize`: Grid cell size in units

**Key Methods:**
- `InitializeNPCSystem()` / `ShutdownNPCSystem()`: Full lifecycle
- `SpawnNPC(int32)` / `DespawnNPC(int32)`: Slot-level control
- `OptimizeNPCCount()`: Auto-adjust to current tier
- `GetNPCsInRadius(FVector, float)`: Spatial query
- `RegisterPatrolRoute(FPatrolRoute)`: Shared route registration
- `LogNPCSystemState()`: Console diagnostics

### FNPCPoolEntry
Per-NPC runtime state. Packed for cache-friendly iteration.

Fields: actor pointer, pool/active flags, spawn data index, patrol state,
cached distance, behavior LOD, spatial grid cell coordinates.

### FNPCSpatialGrid
Lightweight hash-map spatial grid. Not a USTRUCT (no reflection overhead).

### ENPCBehaviorLOD
`Full`, `Reduced`, `Minimal`, `Dormant` - drives tick rate and visibility.

## Usage Example

```cpp
// Create spawn manager (or place in level via editor)
ANPCSpawnManager* Manager = GetWorld()->SpawnActor<ANPCSpawnManager>();

// Configure default ship class for pre-spawning
Manager->DefaultNPCShipClass = ANPCShip::StaticClass();

// Define spawn data
FNPCSpawnData PirateSpawn;
PirateSpawn.NPCClass = APirateShip::StaticClass();
PirateSpawn.SpawnLocation = FVector(1000, 2000, 0);
PirateSpawn.Priority = 50;
PirateSpawn.bEssential = false;

// Create patrol route
FPatrolRoute PirateRoute;
PirateRoute.RouteId = FName("PirateRoute_01");
PirateRoute.Waypoints.Add(FWaypoint(FVector(1000, 2000, 0), 2.0f));
PirateRoute.Waypoints.Add(FWaypoint(FVector(3000, 2000, 0), 1.0f));
PirateRoute.Waypoints.Add(FWaypoint(FVector(3000, 4000, 0), 2.0f));
PirateRoute.bLooping = true;
PirateRoute.MovementSpeed = 400.0f;
PirateSpawn.PatrolRoute = PirateRoute;

Manager->NPCSpawnData.Add(PirateSpawn);
Manager->InitializeNPCSystem();
```

## Debugging

```cpp
Manager->DebugDrawPatrolRoutes();     // Yellow spheres + green lines
Manager->DebugDrawNPCStates();        // Color-coded LOD indicators
Manager->DebugDrawSpatialGrid();      // Grid cells with occupancy counts
Manager->LogNPCSystemState();         // Full console dump with LOD distribution
```

## Performance Budget

Target: < 0.5ms per frame for NPC management on mobile.

- Staggered distance checks: ~0.05ms per batch (1/3 pool)
- Staggered patrol updates: ~0.02ms per batch (4-6 NPCs)
- Spatial grid queries: ~0.01ms per radius query
- Pool validation: ~0.01ms every 5 seconds
- LOD transitions: ~0.005ms per transition (set flags only)

## Integration Points

- `UOdysseyMobileOptimizer`: Performance tier detection and change notifications
- `ANPCShip`: Pre-spawned pool actors, behavior component LOD configuration
- `UNPCBehaviorComponent`: Tick interval scaling for Reduced LOD
- `UNPCHealthComponent`: Unaffected by LOD (event-driven, low overhead)
- `UOdysseyEventBus`: NPC lifecycle events broadcast on spawn/despawn
