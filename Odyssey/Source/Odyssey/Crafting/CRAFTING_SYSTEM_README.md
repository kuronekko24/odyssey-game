# Advanced Crafting & Manufacturing System (Task #7)

## Overview

The Advanced Crafting & Manufacturing System drives Odyssey's player-driven economy through multi-step production chains, automation networks, quality variation, and skill-based progression. It creates sustained economic demand by making crafting both accessible and deeply optimizable.

## Architecture

```
UOdysseyCraftingManager (Master Controller)
├── UOdysseyCraftingRecipeComponent    -- Recipe database, variations, blueprint research
├── UOdysseyAutomationNetworkSystem    -- Node-graph automation for mass production
├── UOdysseyQualityControlSystem       -- Quality tiers, value calculation, equipment effects
├── UOdysseyCraftingSkillSystem        -- Skill trees, experience, mastery bonuses
├── UOdysseyProductionChainPlanner     -- Chain analysis, cost estimation, plan execution
└── UOdysseyCraftingUIDataProvider     -- Mobile-optimized cached UI data
```

## Components

### UOdysseyCraftingManager
**File:** `OdysseyCraftingManager.h/.cpp`
**Role:** Master orchestrator for all crafting operations.

- Manages crafting job lifecycle (start, pause, cancel, complete)
- Coordinates with all subsystems (quality, skill, automation)
- Facility registration and upgrade management
- Job queue with priority-based processing
- Mobile-optimized with configurable batch processing intervals

Key types:
- `EItemQuality` -- 7-tier quality system (Scrap through Legendary)
- `ECraftingTier` -- 6-tier facility progression (Primitive through Quantum)
- `EItemCategory` -- 9 item categories for recipe organization
- `FCraftedItem` -- Extended item with quality, crafter ID, stat modifiers
- `FAdvancedCraftingRecipe` -- Full recipe with alt inputs, prerequisites, automation support
- `FCraftingJob` -- Active job with progress tracking and produced items
- `FCraftingFacility` -- Station configuration with speed/quality/energy modifiers

### UOdysseyCraftingRecipeComponent
**File:** `OdysseyCraftingRecipeComponent.h/.cpp`
**Role:** Dynamic recipe system with discovery and research mechanics.

- Recipe variation system with alternative ingredients/outputs
- Blueprint research queue with parallel research support
- Experimentation system for discovering recipe variations
- Efficiency calculations combining skill and facility bonuses
- Recipe information queries (difficulty, profit margin, ingredient usage)

Key types:
- `FRecipeVariation` -- Alternative input/output configuration with modifiers
- `FCraftingBlueprint` -- Research item that unlocks recipe groups
- `FBlueprintResearchProgress` -- Active research tracking
- `FRecipeEfficiencyModifiers` -- Aggregated efficiency from all sources

### UOdysseyAutomationNetworkSystem
**File:** `OdysseyAutomationNetworkSystem.h/.cpp`
**Role:** Player-built automation networks for mass production.

- Node-based production graph (Input, Output, Processing, Storage, Splitter, Merger, Filter)
- Resource flow simulation through connections with transfer rates and filters
- Production line grouping with aggregate metrics
- Automatic bottleneck detection with optimization recommendations
- Energy consumption tracking per node and per line
- Cycle detection prevents invalid graph configurations

Key types:
- `EAutomationNodeType` -- 7 node types for building production graphs
- `FAutomationNode` -- Complete node with buffers, metrics, and processing state
- `FAutomationConnection` -- Directed edge with transfer rate and resource filters
- `FResourceBuffer` -- Resource storage with capacity management
- `FProductionLine` -- Grouped nodes with aggregate efficiency metrics
- `FBottleneckAnalysis` -- Diagnostic results with severity and recommendations

### UOdysseyQualityControlSystem
**File:** `OdysseyQualityControlSystem.h/.cpp`
**Role:** Quality determination and item value differentiation.

- Multi-factor quality calculation (skill + facility + material + random variance)
- Critical craft system (bonus quality tier on exceptional rolls)
- Quality-based value multipliers (Scrap 0.25x through Legendary 20x)
- Equipment stat modification by quality tier
- Market demand modeling with scarcity-based pricing
- Item inspection and authenticity verification
- Temporary quality bonus system (catalysts, buffs)

Key types:
- `FQualityTierConfig` -- Threshold, value multiplier, stat bonus, color per tier
- `FQualityRollResult` -- Detailed roll breakdown with all applied modifiers
- `FQualityEquipmentEffect` -- Damage, defense, durability, slot multipliers
- `FQualityMarketDemand` -- Demand, price, and scarcity multipliers

### UOdysseyCraftingSkillSystem
**File:** `OdysseyCraftingSkillSystem.h/.cpp`
**Role:** Skill progression affecting crafting outcomes.

- 9 skill categories (General, Material Processing, Weapons, Armor, Ship Modules, Electronics, Chemistry, Research, Automation)
- 15+ default skills with prerequisite chains
- Experience gain from crafting with quality multipliers
- Skill point allocation system with respec support
- Mastery bonuses for category specialization (6 masteries)
- Recipe unlocks tied to skill progression

Key types:
- `FCraftingSkill` -- Skill with level, XP, and per-level bonuses
- `FCraftingMasteryBonus` -- Category-wide multipliers for specialists
- `FSkillProgressInfo` -- Current progress and statistics
- `FSkillTreeNode` -- UI layout data for skill tree rendering

### UOdysseyProductionChainPlanner (NEW)
**File:** `OdysseyProductionChainPlanner.h/.cpp`
**Role:** Analyzes recipe dependency graphs and generates optimal production plans.

- Recursive chain resolution from target product to raw materials
- Inventory-aware planning (subtracts already-owned materials)
- Complete cost breakdown with profit margin analysis
- Recipe profitability ranking
- Plan execution with step-by-step progress tracking
- Feasibility checking with detailed blocking reason reporting

Key types:
- `FProductionStep` -- Single step in a production plan with dependencies
- `FProductionPlan` -- Complete plan with all steps, materials, and estimates
- `FProductionCostBreakdown` -- Material, energy, time costs vs output value

### UOdysseyCraftingUIDataProvider (NEW)
**File:** `OdysseyCraftingUIDataProvider.h/.cpp`
**Role:** Mobile-optimized UI data cache.

- Pre-computes recipe status, job progress, and skill data at fixed intervals
- Eliminates per-frame polling of crafting subsystems
- Configurable refresh rate for mobile power savings
- Pre-formatted display strings (time remaining, ingredient status, bonus descriptions)
- Category-based filtering for tabbed UI layouts

Key types:
- `FRecipeDisplayData` -- Complete recipe state including material/skill availability
- `FJobDisplayData` -- Job progress with formatted text and status colors
- `FSkillDisplayData` -- Skill tree node with progress and bonus summary

## Production Chain Design

### Multi-Step Processing
```
Raw Materials (Gathered)
  └── Processed Materials (Refining: Tier 1)
        └── Components (Assembly: Tier 2-3)
              └── Equipment (Manufacturing: Tier 3-4)
                    └── Advanced Equipment (Engineering: Tier 4-5)
```

Each step adds value and requires increasingly specialized skills and facilities. This creates natural trade opportunities where players specialize in different chain segments.

### Economic Demand Loop
1. Combat creates demand for ammunition and equipment repairs
2. Ship upgrades drive demand for high-quality ship modules
3. Automation networks consume materials at scale
4. Quality variation creates premium markets for skilled crafters
5. Blueprint research gates advanced recipes behind time investment

## Mobile Optimization

- **Batch job processing** -- Jobs update at configurable intervals (default 10Hz) rather than every frame
- **UI data caching** -- Display data refreshes at 2Hz via `UOdysseyCraftingUIDataProvider`
- **Tick intervals** -- Subsystems use staggered tick intervals (Recipe: 0.5s, Quality: 1.0s, Automation: 0.1s)
- **Max processing limits** -- Job batch size and recipe refresh limits prevent frame spikes

## Integration Points

### Resource System
- `UOdysseyInventoryComponent` -- Ingredient consumption and output storage
- `EResourceType` -- Shared resource type enum across all systems

### Economy
- `UOdysseyTradingComponent` -- Market price queries for profit calculations
- Quality-based value multipliers feed into market pricing
- Crafted items enter the trading system with crafter attribution

### Combat
- Equipment quality affects weapon damage, armor defense, and module efficiency
- Ammunition crafting supports sustained combat operations

### Character Progression
- Crafting skills integrate with overall character advancement
- Skill points earned through crafting feed into the skill tree
- Mastery bonuses provide long-term specialization goals

## File Listing

| File | Lines | Description |
|------|-------|-------------|
| `OdysseyCraftingManager.h` | ~450 | Master controller header |
| `OdysseyCraftingManager.cpp` | ~650 | Master controller implementation |
| `OdysseyCraftingRecipeComponent.h` | ~300 | Recipe system header |
| `OdysseyCraftingRecipeComponent.cpp` | ~500 | Recipe system implementation |
| `OdysseyAutomationNetworkSystem.h` | ~400 | Automation system header |
| `OdysseyAutomationNetworkSystem.cpp` | ~700 | Automation system implementation |
| `OdysseyQualityControlSystem.h` | ~350 | Quality system header |
| `OdysseyQualityControlSystem.cpp` | ~450 | Quality system implementation |
| `OdysseyCraftingSkillSystem.h` | ~350 | Skill system header |
| `OdysseyCraftingSkillSystem.cpp` | ~650 | Skill system implementation |
| `OdysseyProductionChainPlanner.h` | ~200 | Chain planner header |
| `OdysseyProductionChainPlanner.cpp` | ~350 | Chain planner implementation |
| `OdysseyCraftingUIDataProvider.h` | ~250 | UI data provider header |
| `OdysseyCraftingUIDataProvider.cpp` | ~350 | UI data provider implementation |
| `CraftingTypes.h` | ~15 | Convenience aggregation header |

**Total: ~6,000 lines of implementation**

## Usage Example

```cpp
// Setup: Crafting manager auto-creates subsystems in BeginPlay()
UOdysseyCraftingManager* CraftingMgr = Actor->FindComponentByClass<UOdysseyCraftingManager>();

// Start a crafting job
FGuid JobID = CraftingMgr->StartCraftingJob(FName("RefinedSilicate"), 5);

// Check production chain for a complex item
TArray<FName> Chain = CraftingMgr->GetProductionChain(FName("WarpDrive"));
TArray<FCraftingIngredient> TotalMaterials = CraftingMgr->CalculateChainMaterials(FName("WarpDrive"), 1);

// Generate a full production plan
UOdysseyProductionChainPlanner* Planner = Actor->FindComponentByClass<UOdysseyProductionChainPlanner>();
FProductionPlan Plan = Planner->GenerateProductionPlan(FName("WarpDrive"), 1, true);

// Set up automation
UOdysseyAutomationNetworkSystem* Automation = CraftingMgr->GetAutomationSystem();
FGuid InputNode = Automation->CreateNode(EAutomationNodeType::Input, FVector(0, 0, 0));
FGuid ProcessNode = Automation->CreateNode(EAutomationNodeType::Processing, FVector(200, 0, 0));
FGuid OutputNode = Automation->CreateNode(EAutomationNodeType::Output, FVector(400, 0, 0));
Automation->CreateConnection(InputNode, 0, ProcessNode, 0);
Automation->CreateConnection(ProcessNode, 0, OutputNode, 0);
Automation->AssignRecipeToNode(ProcessNode, FName("RefinedSilicate"));

// UI data for mobile
UOdysseyCraftingUIDataProvider* UIData = Actor->FindComponentByClass<UOdysseyCraftingUIDataProvider>();
TArray<FRecipeDisplayData> Recipes = UIData->GetCraftableRecipeDisplayData();
```
