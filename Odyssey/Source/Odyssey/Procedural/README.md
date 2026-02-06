# Procedural Planet & Resource Generation System

## Architecture Overview

This subsystem generates diverse planets with biomes, resources, exploration content,
and economic specialization. All generation is seed-based for deterministic
regeneration, minimizing save data on mobile.

## File Structure

```
Procedural/
  ProceduralTypes.h              - Shared enums, structs, constants
  ExplorationRewardSystem.h/.cpp - Discovery mechanics and rewards
  PlanetaryEconomyComponent.cpp  - Economy implementation (header in parent)
  ProceduralPlanetManager.h/.cpp - Master coordinator tying all systems together
  README.md                      - This file
```

## Core Components

### UProceduralPlanetManager (Master Coordinator)
- Initializes and coordinates all subsystems
- Manages planet/system/galaxy generation lifecycle
- Lazy-loads exploration content when player approaches
- Handles save/load via seed-based regeneration
- Provides unified query API for all procedural content

### UOdysseyPlanetGenerator (Planet Generation)
- Seed-based planet generation with orbital mechanics
- Star system and galaxy region generation
- Biome region layout using Voronoi-like distribution
- Point of interest placement
- Procedural name generation

### UOdysseyBiomeDefinitionSystem (Biome Definitions)
- 12 distinct biome types with unique characteristics
- Environmental hazards and gameplay modifiers
- Biome-specific resource weights
- Visual data for rendering
- Biome compatibility for realistic adjacency

### UOdysseyResourceDistributionSystem (Resource Placement)
- Strategic resource clustering via Poisson disk sampling
- Biome-influenced resource type selection
- Rarity tiers with weighted distribution
- Trade route opportunity analysis
- Resource scarcity calculation

### UOdysseyPlanetaryEconomyComponent (Planetary Economy)
- 9 economic specializations (Mining, Agriculture, Tech, etc.)
- 10 trade good types with production/consumption chains
- Dynamic supply/demand pricing
- Economic relationships between planets
- GDP calculation and wealth tracking

### UExplorationRewardSystem (Exploration & Discovery)
- 15 discovery types with biome affinity
- 6 rarity tiers (Common through Mythic)
- Multi-mode scanning system (Passive, Active, Deep, Anomaly)
- Fog-of-war grid tracking
- Milestone progression with rewards
- Resource and OMEN reward distribution

## Design Principles

1. **Seed Determinism**: All generation uses seeded random functions,
   so planets can be regenerated from just a seed + delta state.

2. **Mobile Performance**: Lazy generation, minimal memory footprint,
   tick-interval throttling, grid-based fog instead of per-pixel.

3. **Economic Meaning**: Every planet has a specialization that creates
   natural trade imbalances, driving inter-planet commerce.

4. **Exploration Loop**: Discoveries are placed to reward venturing into
   hazardous biomes with rarer/higher-value findings.

5. **Serialization Efficiency**: Save data stores only seeds and deltas
   (discovered items, depleted resources, economy state changes).
