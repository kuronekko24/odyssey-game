# Dynamic Economy Simulation System (Task #5)

## Overview

The Odyssey Dynamic Economy is a real-time market simulation engine that drives
a player-driven economy with supply/demand dynamics, dynamic pricing, trade
opportunities, and economic chain reactions.

## Architecture

```
UOdysseyEconomyManager (master controller)
├── UMarketDataComponent (per-market)     - Supply/demand tracking
├── UPriceFluctuationSystem (per-market)  - Dynamic price calculation
├── UTradeRouteAnalyzer (singleton)       - Cross-market opportunity detection
├── UEconomicEventSystem (singleton)      - Market-affecting event generation
├── UEconomyRippleEffect (singleton)      - Chain reaction propagation [NEW]
└── UEconomySaveSystem (singleton)        - Persistent state serialization [NEW]
```

## File Structure

```
Economy/
├── OdysseyEconomyTypes.h        - All data structures, enums, delegates
├── OdysseyEconomyManager.h      - Master controller header (canonical)
├── OdysseyEconomyManager.cpp    - Master controller implementation
├── EconomyRippleEffect.h        - Ripple propagation header
├── EconomyRippleEffect.cpp      - Ripple propagation implementation
├── EconomySaveSystem.h          - Save/load header
├── EconomySaveSystem.cpp        - Save/load implementation
└── README.md                    - This file
```

Existing subsystem files remain in the Source/Odyssey/ root:
- `UMarketDataComponent.h/cpp`
- `UPriceFluctuationSystem.h/cpp`
- `UTradeRouteAnalyzer.h/cpp`
- `UEconomicEventSystem.h/cpp`

## Component Responsibilities

### UOdysseyEconomyManager
- Creates and owns all subsystems
- Unified API for all economy operations
- Integrates combat and crafting systems
- Manages market registration
- Drives ripple effects from events and player actions
- Provides player-facing economy queries

### UMarketDataComponent (per market)
- Tracks current supply, max supply, supply rate
- Tracks demand rate, base demand, demand elasticity
- Calculates supply/demand ratio and scarcity index
- Simulates production and consumption over time
- Maintains price history for trend analysis

### UPriceFluctuationSystem (per market)
- Calculates buy/sell prices from supply/demand
- Applies market volatility (5 levels: Stable to Extreme)
- Price smoothing to prevent jarring changes
- Volume-based price impact for large orders
- Event modifier stacking with time-based expiration
- Price shock decay system

### UTradeRouteAnalyzer
- Analyzes all market pairs for arbitrage opportunities
- Calculates net profit considering taxes, fuel, risk
- Ranks opportunities by composite score
- Round-trip profit optimization
- Periodic re-analysis on configurable interval

### UEconomicEventSystem
- Generates random events (resource discovery, pirate attacks, etc.)
- Template-based event creation with severity scaling
- Event lifecycle management (activate, expire, cancel)
- Chain reaction event generation
- News headline generation for narrative

### UEconomyRippleEffect [NEW]
- Wave-front propagation (BFS through trade route graph)
- Per-hop dampening for realistic distance falloff
- Cycle detection prevents infinite loops
- 6 ripple types: SupplyShock, DemandShock, PriceShock,
  TradeDisruption, CombatZone, CraftingDemand
- Mobile-optimized: one wave per tick

### UEconomySaveSystem [NEW]
- Full economy snapshot capture
- USaveGame-based disk persistence
- Autosave with configurable interval
- Save data validation and version migration
- Per-market granular state restoration

## Integration Points

### Combat -> Economy
```cpp
// When combat occurs, report it:
EconomyManager->ReportCombatEvent(Attacker, Victim, Damage, bWasKill);

// The manager will:
// 1. Find nearest market
// 2. Create CombatZone ripple (reduces supply, increases demand)
// 3. Possibly trigger PirateActivity event at high intensity
```

### Crafting -> Economy
```cpp
// When player crafts an item:
EconomyManager->ReportCraftingActivity(
    EResourceType::Carbon, 10,              // consumed
    EResourceType::RefinedCarbon, 5);       // produced

// The manager will:
// 1. Increase demand multiplier for Carbon
// 2. Create CraftingDemand ripple through connected markets
```

### Event Bus Integration
```cpp
EconomyManager->ConnectToEventBus(UOdysseyEventBus::Get());
// Subscribes to: DamageDealt, InteractionCompleted
// Mining interactions feed supply into nearest market
```

## Configuration

All tuning parameters are in `FEconomyConfiguration`:

| Parameter | Default | Description |
|-----------|---------|-------------|
| TickIntervalSeconds | 1.0 | Economy update frequency |
| PriceUpdateIntervalSeconds | 5.0 | How often prices recalculate |
| SupplyDemandPriceInfluence | 0.7 | S/D weight in price formula |
| PriceSmoothingFactor | 0.8 | Price change dampening |
| MaxActiveEvents | 5 | Simultaneous economic events |
| MaxActiveRipples | 10 | Simultaneous ripple waves |
| RippleDefaultDampening | 0.3 | 30% magnitude lost per hop |
| RippleMaxPropagationDepth | 4 | Max hops from origin |
| MaxMarketsToUpdatePerTick | 5 | Mobile performance limit |

## Mobile Optimization

- **Staggered updates**: MaxMarketsToUpdatePerTick limits per-frame work
- **Lazy analysis**: Trade routes re-analyzed on interval, not every frame
- **Wave propagation**: Ripples advance one hop per tick, not full BFS
- **History limits**: MaxPriceHistoryEntries caps memory usage
- **Tick intervals**: All components use >0.5s tick intervals

## Usage Example

```cpp
// 1. Create manager on game mode or persistent actor
UOdysseyEconomyManager* Manager = NewObject<UOdysseyEconomyManager>(GameMode);
Manager->RegisterComponent();
Manager->InitializeEconomy(FEconomyConfiguration());
Manager->ConnectToEventBus(UOdysseyEventBus::Get());

// 2. Create markets
Manager->CreateMarket(
    FMarketId(FName("StationAlpha"), 1),
    TEXT("Station Alpha"),
    FVector(1000, 0, 0),
    EMarketLocationType::Station);

Manager->CreateMarket(
    FMarketId(FName("OutpostBeta"), 1),
    TEXT("Outpost Beta"),
    FVector(5000, 3000, 0),
    EMarketLocationType::Outpost);

// 3. Auto-generate trade routes
Manager->GetTradeRouteAnalyzer()->GenerateAllRoutes(100.0f);

// 4. Player buys resources
Manager->ExecuteBuy(
    FMarketId(FName("StationAlpha"), 1),
    EResourceType::Carbon,
    50,
    PlayerCharacter);

// 5. Check trade opportunities
TArray<FTradeOpportunity> Opportunities =
    Manager->GetRecommendedTrades(PlayerCharacter, 5);

// 6. Save economy state
Manager->QuickSave();
```
