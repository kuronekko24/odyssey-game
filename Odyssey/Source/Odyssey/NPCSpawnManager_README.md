# NPC Spawning & Performance System

## Overview

The NPC Spawning & Performance System provides mobile-optimized NPC management for Odyssey with object pooling, performance-based scaling, and patrol route functionality.

## Key Features

### 1. Object Pooling
- Pre-allocated NPC pool to eliminate runtime allocation
- Configurable pool size (default: 12 initial, 20 maximum)
- Automatic pool expansion when needed
- Efficient reuse of NPC actors

### 2. Performance Tier Integration
- **High Performance**: Up to 12 NPCs, 4000 unit culling distance
- **Medium Performance**: Up to 8 NPCs, 3000 unit culling distance  
- **Low Performance**: Up to 4 NPCs, 2000 unit culling distance, no patrolling

### 3. Distance-Based Culling
- NPCs activate/deactivate based on player proximity
- Configurable culling distances per performance tier
- Essential NPCs always remain active
- Smooth transitions to maintain performance

### 4. Waypoint Patrol System
- Define patrol routes with multiple waypoints
- Configurable wait times at each waypoint
- Looping and non-looping routes supported
- Performance-aware patrol updates

### 5. Priority System
- NPCs spawn in priority order on limited devices
- Essential NPCs always spawn regardless of performance
- Automatic optimization based on current performance tier

## Core Classes

### ANPCSpawnManager
Main manager class responsible for NPC lifecycle, performance optimization, and patrol management.

**Key Properties:**
- `NPCSpawnData`: Array of NPC spawn configurations
- `NPCPool`: Object pool for NPC instances
- `CurrentPerformanceTier`: Current performance level from mobile optimizer
- `ActiveNPCCount`: Number of currently active NPCs

**Key Methods:**
- `InitializeNPCSystem()`: Initialize the entire NPC system
- `SpawnNPC(int32 SpawnDataIndex)`: Spawn an NPC from pool
- `OptimizeNPCCount()`: Adjust active NPC count based on performance
- `UpdateNPCPatrols()`: Update patrol movement for active NPCs

### FNPCSpawnData
Defines spawn configuration for individual NPCs.

**Properties:**
- `NPCClass`: The NPC class to spawn
- `SpawnLocation/SpawnRotation`: Initial spawn transform
- `PatrolRoute`: Waypoint-based movement pattern
- `Priority`: Spawn priority (higher = spawns first)
- `bEssential`: Always spawn regardless of performance

### FPatrolRoute
Defines waypoint-based movement patterns.

**Properties:**
- `Waypoints`: Array of FWaypoint positions
- `bLooping`: Whether route repeats infinitely
- `MovementSpeed`: Units per second movement speed
- `ActivationDistance`: Distance at which patrol becomes active

### FWaypoint
Individual waypoint in a patrol route.

**Properties:**
- `Location`: World position of waypoint
- `WaitTime`: Seconds to wait at this waypoint
- `bCanInteract`: Whether NPCs can be interacted with here

## Performance Integration

The system integrates with `UOdysseyMobileOptimizer` to automatically adjust NPC counts and behavior based on device performance:

### Performance Limits per Tier:
```cpp
// High Performance (Desktop/High-end mobile)
MaxNPCs = 12
UpdateFrequency = 0.05f
CullingDistance = 4000.0f
bEnablePatrolling = true

// Medium Performance (Standard mobile)
MaxNPCs = 8
UpdateFrequency = 0.1f
CullingDistance = 3000.0f
bEnablePatrolling = true

// Low Performance (Low-end mobile)
MaxNPCs = 4
UpdateFrequency = 0.2f
CullingDistance = 2000.0f
bEnablePatrolling = false
```

## Usage Example

### Basic Setup
```cpp
// Create NPC Spawn Manager
ANPCSpawnManager* SpawnManager = GetWorld()->SpawnActor<ANPCSpawnManager>();

// Configure spawn data
FNPCSpawnData NPCData;
NPCData.NPCClass = AOdysseyCharacter::StaticClass();
NPCData.SpawnLocation = FVector(0, 0, 0);
NPCData.Priority = 50;
NPCData.bEssential = true;

// Create patrol route
FPatrolRoute Route;
Route.Waypoints.Add(FWaypoint(FVector(0, 0, 0), 2.0f, true));
Route.Waypoints.Add(FWaypoint(FVector(500, 0, 0), 1.0f, false));
Route.Waypoints.Add(FWaypoint(FVector(500, 500, 0), 2.0f, true));
Route.bLooping = true;
Route.MovementSpeed = 200.0f;

NPCData.PatrolRoute = Route;

// Add to spawn manager
SpawnManager->NPCSpawnData.Add(NPCData);

// Initialize system
SpawnManager->InitializeNPCSystem();
```

### Runtime Management
```cpp
// Check current state
int32 ActiveCount = SpawnManager->GetActiveNPCCount();
int32 MaxAllowed = SpawnManager->GetMaxNPCsForCurrentTier();

// Force optimization
SpawnManager->OptimizeNPCCount();

// Get all active NPCs
TArray<AOdysseyCharacter*> ActiveNPCs = SpawnManager->GetActiveNPCs();

// Debug information
SpawnManager->LogNPCSystemState();
SpawnManager->DebugDrawPatrolRoutes();
```

## Architecture Details

### Object Pool Management
- Pool entries track usage, activation state, and patrol progress
- Actors are hidden/shown rather than destroyed/created
- Pool automatically expands up to maximum size when needed
- Validation system ensures pool integrity

### Distance-Based Optimization
- Staggered distance checks every 2 seconds
- NPCs beyond culling distance are deactivated
- Priority system ensures important NPCs stay active
- Smooth activation/deactivation prevents performance spikes

### Patrol System
- Staggered updates to process subset of NPCs per frame
- State machine tracks movement and waiting phases
- Performance-aware update frequencies
- Automatic route validation and waypoint progression

### Performance Monitoring
- Integrates with mobile optimizer for tier changes
- Automatic NPC count adjustment on performance changes
- Essential NPCs protected from optimization
- Configurable limits per performance tier

## Best Practices

1. **Priority Assignment**: Use higher priorities for gameplay-critical NPCs
2. **Essential Flag**: Mark story-important NPCs as essential
3. **Patrol Routes**: Keep routes within reasonable distances for performance
4. **Pool Sizing**: Set initial pool size to expected concurrent NPC count
5. **Update Frequencies**: Balance responsiveness with performance impact

## Debugging

### Debug Functions
- `DebugDrawPatrolRoutes()`: Visualize waypoint paths
- `DebugDrawNPCStates()`: Show NPC activation status
- `LogNPCSystemState()`: Print comprehensive system stats

### Console Commands
The system logs important events to help with debugging:
- NPC spawn/despawn events
- Performance tier changes
- Pool management operations
- Patrol state transitions

## Integration Notes

### Dependencies
- Requires `UOdysseyMobileOptimizer` for performance integration
- Uses `AOdysseyCharacter` as base NPC class
- Integrates with Unreal's standard Actor/Component system

### Mobile Optimization
- Designed specifically for mobile performance constraints
- Automatic scaling based on device capabilities
- Minimal memory allocation during gameplay
- Efficient update patterns to maintain frame rate

## Future Enhancements

Potential areas for expansion:
- AI behavior trees integration
- Dynamic spawn point generation
- NPC interaction system integration
- Save/load support for NPC states
- Network replication for multiplayer
