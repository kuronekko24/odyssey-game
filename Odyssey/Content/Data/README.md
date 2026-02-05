# Resource Mining and Gathering System

This folder contains the data-driven resource system for Odyssey's mining and crafting gameplay.

## Core Components

### C++ Classes
- **UOdysseyInventoryComponent**: Manages player resource storage and stacking
- **AResourceNode**: Mineable resource nodes with regeneration and depletion
- **FResourceStack**: Stackable resource container with overflow handling
- **FResourceData**: Data table structure for resource configuration

### Resource Types
1. **Raw Materials** (ID 1-9)
   - Silicate (ID: 1) - Basic building material
   - Carbon (ID: 2) - Organic manufacturing component

2. **Refined Materials** (ID 10-19)
   - Refined Silicate (ID: 10) - Processed silicate
   - Refined Carbon (ID: 11) - Purified carbon

3. **Advanced Materials** (ID 20-99)
   - Composite Material (ID: 20) - High-value crafted material

4. **Currency** (ID 100+)
   - OMEN (ID: 100) - Galactic currency

## Resource Node Configuration

### Node States
- **Full**: 80-100% resources remaining
- **Depleting**: 1-79% resources remaining
- **Depleted**: 0% resources remaining
- **Regenerating**: Actively restoring resources

### Node Properties
```cpp
struct FResourceNodeData {
    EResourceType ResourceType;     // Type of resource
    int32 MaxResourceAmount;        // Maximum capacity
    int32 CurrentResourceAmount;    // Current amount available
    float MiningDifficulty;         // Mining efficiency modifier
    float RegenerationRate;         // Resources per second regen
    bool bCanRegenerate;           // Whether node regenerates
}
```

## Mining Mechanics

### Mining Formula
```
ActualAmount = MinerPower / NodeDifficulty
ClampedAmount = Min(ActualAmount, CurrentResources)
```

### Character Mining Stats
- **Mining Power**: Base amount extracted per operation
- **Mining Speed**: Operations per second
- **Inventory Capacity**: Maximum resource stacks carried

## Inventory System

### Stack Management
- Resources automatically stack up to MaxStackSize
- Overflow creates new stacks when space available
- Empty stacks are automatically cleaned up
- Full inventory prevents further resource collection

### Stack Limits
- Raw Materials: 100 per stack
- Refined Materials: 50 per stack
- Advanced Materials: 25 per stack
- Currency (OMEN): 10,000 per stack

## Blueprint Integration

### Creating Resource Nodes
1. Create Blueprint from AResourceNode
2. Configure mesh and collision components
3. Set resource type and capacity in NodeData
4. Assign state materials (Full/Depleting/Depleted)
5. Configure regeneration settings

### Inventory Events
```cpp
OnInventoryChanged();              // Any inventory modification
OnResourceAdded(Type, Amount);     // Specific resource added
OnResourceRemoved(Type, Amount);   // Specific resource removed
OnInventoryFull();                 // Inventory at capacity
```

### Mining Events
```cpp
OnResourceMined(Amount);           // Resource extracted from node
OnNodeDepleted();                  // Node completely mined out
OnNodeRegenerated();               // Depleted node restored
OnStateChanged(OldState, NewState); // Node state transition
```

## Data Tables

### ResourceDataTable.json
Contains all resource definitions, values, and properties. Use this to:
- Configure resource names and descriptions
- Set stack sizes and market values
- Define crafting and selling permissions
- Assign UI icons for resources

## Performance Notes

### Optimization Features
- Resource nodes only tick when being mined or regenerating
- Inventory operations use efficient array management
- Empty stack cleanup prevents memory bloat
- Resource data cached for fast lookups

### Mobile Considerations
- Touch-friendly interaction spheres (200+ unit radius)
- Visual feedback for mining state changes
- Battery-efficient regeneration calculations
- Memory-optimized stack management

## Demo Integration

This system supports the 10-minute demo loop:
- **Minutes 1-2**: Tutorial shows basic mining of Silicate
- **Minutes 3-4**: Players mine both Silicate and Carbon
- **Minutes 5-6**: Full inventory drives players to crafting
- Resources feed directly into the crafting and trading systems