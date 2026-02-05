# Crafting System

This folder contains the complete crafting system for Odyssey, enabling players to transform raw materials into refined goods and advanced components.

## Core Components

### C++ Classes
- **UOdysseyCraftingComponent**: Main crafting component with queue management
- **ACraftingStation**: Interactive stations that provide crafting bonuses
- **FCraftingRecipe**: Data structure for recipe definitions
- **FCraftingQueue**: Queue item for managing crafting operations

### Recipe System

#### Recipe Structure
```cpp
struct FCraftingRecipe {
    FString RecipeName;                    // Display name
    FString Description;                   // Recipe description
    TArray<FCraftingIngredient> Ingredients; // Required materials
    TArray<FCraftingOutput> Outputs;       // Produced items
    float CraftingTime;                    // Base time to craft
    int32 RequiredCraftingLevel;          // Level requirement
    bool bIsUnlocked;                      // Whether recipe is available
    int32 ExperienceReward;               // XP gained on completion
}
```

#### Available Recipes
1. **Refine Silicate**: 3 Silicate → 1 Refined Silicate (2s, Level 1)
2. **Refine Carbon**: 2 Carbon → 1 Refined Carbon (2.5s, Level 1)
3. **Composite Material**: 2 Refined Silicate + 1 Refined Carbon → 1 Composite (5s, Level 3)
4. **Batch Refining**: Efficient bulk processing (Unlocks at Level 5)
5. **Advanced Composite**: Enhanced recipes (Unlocks at Level 7)

## Crafting Mechanics

### Queue System
- Players can queue up to 3 crafting operations simultaneously
- Queue processes items sequentially
- Real-time progress tracking with remaining time
- Cancellation with partial material refund

### Success Rates
- Most basic recipes have 100% success rate
- Advanced recipes may have 85-95% success chance
- Station bonuses improve success rates
- Crafting level provides permanent success bonus

### Experience and Leveling
- Each completed recipe awards experience points
- Level progression follows exponential curve (Level² × 100 XP)
- Each level provides:
  - +5% crafting speed bonus
  - +2% success rate bonus
  - Access to higher-tier recipes

## Crafting Stations

### Station Types
1. **Basic Station**
   - +20% crafting speed
   - +10% success rate
   - +2 crafting slots
   - Supports: Basic Refining

2. **Advanced Station**
   - +40% crafting speed
   - +20% success rate
   - +4 crafting slots
   - Supports: Basic + Advanced Materials

3. **Industrial Station**
   - +60% crafting speed
   - +30% success rate
   - +6 crafting slots
   - Supports: All recipe categories

### Station Interaction
- Players must be within interaction range to use stations
- Station bonuses apply while player is using the station
- Automatic bonus removal when player leaves or disconnects
- Visual effects indicate station operation status

## Integration with Resource System

### Material Flow
1. **Raw Materials**: Mined from resource nodes (Silicate, Carbon)
2. **Refined Materials**: Basic processing recipes
3. **Advanced Materials**: Complex multi-ingredient recipes
4. **Trading**: All crafted goods can be sold for OMEN currency

### Recipe Progression
- Start with basic refining recipes (immediately unlocked)
- Composite recipes unlock at level 3
- Batch processing unlocks at level 5
- Advanced recipes unlock at level 7

## Blueprint Implementation

### Creating Crafting UI
```cpp
// Get available recipes for current station
TArray<FName> Recipes = CraftingStation->GetAvailableRecipesForStation(CraftingComponent);

// Start crafting operation
bool Success = CraftingComponent->StartCrafting(RecipeID, Quantity);

// Monitor crafting progress
float TotalTime = CraftingComponent->GetTotalCraftingTime();
TArray<FCraftingQueue> Queue = CraftingComponent->GetCraftingQueue();
```

### Crafting Events
```cpp
OnCraftingStarted(RecipeID, Quantity);         // Queue item added
OnCraftingCompleted(RecipeID, Quantity, Success); // Item finished
OnCraftingCancelled(RecipeID);                 // Item cancelled
OnCraftingLevelUp(NewLevel);                   // Player leveled up
```

## Demo Integration

### 10-Minute Experience Flow
- **Minutes 1-2**: Tutorial introduces basic refining
- **Minutes 3-4**: Player crafts first Refined Silicate
- **Minutes 5-6**: Discovery of Composite recipe at level 3
- **Minutes 7-8**: Advanced crafting with station bonuses
- **Minutes 9-10**: Trading crafted goods for OMEN currency

### Quick Start Configuration
- All basic recipes unlocked immediately
- Fast leveling for demo (reduced XP requirements)
- Instant craft option for tutorial demonstrations
- Clear visual progression feedback

## Performance Optimization

### Mobile Considerations
- Efficient queue processing with delta time accumulation
- Minimal memory allocation during crafting operations
- Background crafting continues during UI navigation
- Battery-efficient timing calculations

### Data Management
- Recipe data loaded from Data Tables for easy configuration
- Cached recipe lookups for performance
- Automatic cleanup of empty crafting queue slots
- Optimized success chance calculations

## Configuration Files

### CraftingRecipes.json
Contains all recipe definitions, including:
- Ingredient requirements and quantities
- Output items and success rates
- Crafting times and level requirements
- Experience rewards and unlock conditions

### Usage Notes
- Recipes can be modified without code changes
- New recipe categories easily added
- Crafting balance adjustable through data files
- Station bonuses configurable per station type