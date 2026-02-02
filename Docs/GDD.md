# Odyssey - Game Design Document

## Overview

**Title:** Odyssey
**Genre:** Sci-Fi Space Exploration / Resource Management / Economy Sim
**Art Style:** 2.5D Isometric
**Platform:** Mobile-first (iOS/Android), PC/Console later
**Setting:** Futuristic galaxy with explorable planets
**Currency:** OMEN
**Economy:** Player-driven (all crafted goods are player-made)

---

## Core Pillars

1. **Exploration** - Discover planets, scan for resources, uncover secrets
2. **Extraction & Production** - Mine, refine, craft, automate
3. **Trade & Economy** - Transport goods, play the market, build wealth
4. **Fleet Building** - Upgrade ships, build a fleet, expand operations

---

## Core Gameplay Loop

```
Explore Planet → Extract Resources → Refine & Craft → Sell/Trade → Upgrade → Expand
```

### Session Loop (10-20 min)
1. Travel to a planet or check automated operations
2. Extract resources or collect from drones
3. Refine materials, craft goods
4. Transport to market, sell for OMEN
5. Reinvest in upgrades or new operations

### Progression Loop (days/weeks)
1. Unlock new planets and regions
2. Build larger fleets and automation networks
3. Dominate market sectors
4. Establish trade routes and passive income

---

## The Galaxy

### Structure
- **5 Major Systems** - Each with a major city/station
- **Multiple Planets per System** - Varying resources, danger levels
- **Trade Routes** - Connections between systems

### Major Cities (Trade Hubs)

All major cities are **Friendly Zones** - no PvP allowed within city limits.

| City | System | Specialty | Market Focus |
|------|--------|-----------|--------------|
| **Nova Prime** | Sol Proxima | Capital, balanced | General goods, high volume |
| **Forge Station** | Vulcan Belt | Industrial | Raw materials, ship parts |
| **Cryo Haven** | Glacius | Research | Tech components, rare elements |
| **Dust Port** | Aridian Reach | Frontier | Mining equipment, survival gear |
| **Nexus Gate** | Outer Ring | Crossroads | Luxury goods, exotic materials |

### System Zones

| System | Inner Zone | Outer Zone | Deep Space |
|--------|------------|------------|------------|
| **Sol Proxima** | Friendly | Mild | Mild |
| **Vulcan Belt** | Friendly | Mild | Full |
| **Glacius** | Friendly | Full | Full |
| **Aridian Reach** | Mild | Full | Hardcore |
| **Outer Ring** | Mild | Full | Hardcore |
| **Uncharted Space** | - | Hardcore | Hardcore |

### Planet Types

| Type | Resources | Hazards | Notes |
|------|-----------|---------|-------|
| **Terrestrial** | Common ores, organics | Mild weather | Beginner-friendly |
| **Volcanic** | Rare metals, crystals | Heat, eruptions | High risk/reward |
| **Ice World** | Frozen gases, cryo-elements | Cold, storms | Specialized equipment needed |
| **Gas Giant (Moons)** | Exotic gases, radioactives | Radiation, gravity | Advanced extraction |
| **Barren/Asteroid** | Pure minerals, ancient tech | Low gravity, debris | Efficient but sparse |

---

## Resource System

### Resource Tiers

**Tier 1 - Raw Materials** (Extracted)
- Iron Ore, Copper Ore, Silicon
- Carbon, Hydrogen, Oxygen
- Common Crystals

**Tier 2 - Refined Materials** (Processed)
- Steel, Alloys, Polymers
- Fuel Cells, Coolants
- Purified Crystals

**Tier 3 - Components** (Crafted)
- Circuit Boards, Power Cores
- Thrusters, Hull Plates
- Sensor Arrays

**Tier 4 - Finished Goods** (Assembled)
- Ship Modules, Weapons
- Drones, Mining Rigs
- Tools, Equipment

### Extraction Methods

| Method | Speed | Yield | Automation |
|--------|-------|-------|------------|
| **Manual Mining** | Slow | Low | None |
| **Mining Drill** | Medium | Medium | Semi-auto |
| **Extractor Rig** | Fast | High | Full auto |
| **Drone Swarm** | Fastest | Highest | Full auto, mobile |

---

## Crafting System

### Crafting Stations

| Station | Function | Location |
|---------|----------|----------|
| **Portable Workbench** | Basic tools, repairs | Anywhere |
| **Refinery** | Ore → Refined materials | Ship or Planet |
| **Fabricator** | Components, equipment | Ship or Station |
| **Assembly Bay** | Ship parts, drones | Station only |
| **Shipyard** | Full ships | Major city only |

### Crafting Flow
```
Raw Ore → [Refinery] → Refined Metal → [Fabricator] → Components → [Assembly] → Finished Product
```

### Blueprint System
- Blueprints unlock crafting recipes
- Found through exploration, research, or purchased
- Rarity affects efficiency and output quality
- Players can sell crafted goods but NOT blueprints (knowledge is earned)

---

## Automation & Drones

### Drone Types

| Drone | Function | Complexity |
|-------|----------|------------|
| **Mining Drone** | Extracts resources autonomously | Basic |
| **Hauler Drone** | Transports between points | Basic |
| **Refinery Drone** | Mobile processing | Advanced |
| **Scout Drone** | Surveys planets, finds deposits | Advanced |
| **Defense Drone** | Protects operations | Advanced |

### Automation Chains
Players can set up automated workflows:
```
Mining Drone → Hauler → Refinery → Hauler → Ship Cargo → Auto-sell at Market
```

### Limits & Balance
- Drone count limited by Command Module level
- Drones require maintenance (fuel, repairs)
- Automation requires upfront investment
- Manual play always has efficiency bonuses

---

## Ships & Fleet

### Ship Classes

| Class | Role | Cargo | Speed | Combat |
|-------|------|-------|-------|--------|
| **Scout** | Exploration | Tiny | Fast | None |
| **Freighter** | Hauling | Large | Slow | Light |
| **Miner** | Extraction | Medium | Slow | None |
| **Gunship** | Combat/Escort | Small | Medium | Heavy |
| **Capital** | Command/Base | Massive | Very Slow | Medium |

### Ship Components (All Player-Crafted)
- **Hull** - Durability, cargo space
- **Engine** - Speed, fuel efficiency
- **Power Core** - Energy for systems
- **Cargo Hold** - Storage capacity
- **Weapons** - Defense capability
- **Shields** - Damage absorption
- **Scanner** - Detection range
- **Command Module** - Drone control capacity

### Fleet Management
- Own multiple ships
- Assign ships to routes (automated trade)
- Hire NPC crew or use alone
- Fleet size limited by player progression

---

## Economy & Markets

### OMEN Currency
- Universal currency across all systems
- Earned through selling goods
- Spent on: blueprints, station services, repairs, NPC services

### Market Mechanics

**Supply & Demand**
- Prices fluctuate based on player activity
- Each city has different demands
- Rare goods command premiums
- Oversupply crashes prices

**Market Types**
| Type | Description |
|------|-------------|
| **Buy Orders** | Players request items at set price |
| **Sell Orders** | Players list items for sale |
| **Instant Buy** | Buy at lowest sell price |
| **Instant Sell** | Sell at highest buy price |

**Trade Routes**
- Buy low in one system, sell high in another
- Distance affects profit potential
- Danger increases with value

### What CAN Be Traded
- Raw resources (only non-crafted items)
- Refined materials
- Components
- Equipment and tools
- Ship parts and modules
- Drones
- Ships

### What CANNOT Be Traded
- Blueprints (must be found/earned)
- Player progression/skills
- Reputation

---

## Player Progression

### Skills/Licenses

| Category | Skills |
|----------|--------|
| **Mining** | Extraction speed, yield bonus, rare finds |
| **Refining** | Processing speed, material efficiency |
| **Crafting** | Quality bonus, faster crafting, less waste |
| **Piloting** | Ship handling, fuel efficiency, speed |
| **Trading** | Better prices, market insights, reputation |
| **Command** | More drones, fleet size, automation efficiency |

### Reputation
- Reputation with each major city
- Unlocks better prices, exclusive contracts
- Lost through illegal activities or abandoning contracts

---

## Combat (Secondary System)

### Threats
- Pirates in trade routes
- Hostile fauna on planets
- Rival players (PvP zones)
- Environmental hazards

### Combat Style
- Ship-to-ship: Tactical, positioning-based
- On-foot: Survival-focused, avoid or defend

### Zone System

| Zone | PvP | On Death | Resources | Typical Locations |
|------|-----|----------|-----------|-------------------|
| **Friendly** | None | No loss | Common | Major cities, starting areas, trade routes |
| **Mild** | Optional | Lose cargo | Uncommon | Outer system planets, minor routes |
| **Full** | Enabled | Lose cargo + equipment | Rare | Frontier systems, contested territories |
| **Hardcore** | Enabled | Lose entire fleet | Exotic | Deep space, uncharted regions, endgame zones |

**Zone Rules:**
- **Friendly Zones** - Complete safety, no player attacks possible. Lower resource yields. All 5 major cities are Friendly.
- **Mild Zones** - PvP requires both parties to be flagged (duel/war). Destroyed ships drop cargo but hull and equipment are saved.
- **Full Zones** - Open PvP. Ships drop cargo AND equipped modules. Hull survives but stripped. Insurance can recover base hull.
- **Hardcore Zones** - Open PvP. Total loss on destruction. Ships, cargo, equipment, drones - all gone. Highest tier resources only found here.

**Risk vs Reward:**
```
Friendly:  ★☆☆☆☆ Risk  |  ★☆☆☆☆ Reward
Mild:      ★★☆☆☆ Risk  |  ★★☆☆☆ Reward
Full:      ★★★★☆ Risk  |  ★★★★☆ Reward
Hardcore:  ★★★★★ Risk  |  ★★★★★ Reward
```

**Insurance System:**
- Available in Friendly and Mild zones only
- Covers base hull value (not equipment/cargo)
- Premium based on ship class and zone
- Cooldown period between claims

---

## Multiplayer Features

### Cooperative
- Joint mining operations
- Shared automation networks
- Fleet convoys for protection
- Guild/Corporation system

### Competitive
- Market competition
- Territory control
- PvP combat zones
- Leaderboards (wealth, production, exploration)

### Social
- Player shops in cities
- Contract system (hire other players)
- Chat, guilds, friends list

---

## Monetization (F2P)

### Principles
- No pay-to-win
- Cannot buy OMEN directly
- Cosmetics and convenience only

### Revenue Streams
1. **Cosmetics** - Ship skins, character outfits, drone skins
2. **Convenience** - Extra save slots, UI themes, cosmetic pets
3. **Premium Pass** - Seasonal cosmetic rewards track
4. **Station Services** - Faster repairs, priority docking (time-save)

### NOT Sold
- Resources, materials, items
- OMEN currency
- Blueprints
- Ships or equipment
- Competitive advantages

---

## Mobile UX

### Controls
- **Left thumb:** Virtual joystick (movement/flight)
- **Right thumb:** Action buttons (mine, interact, fire)
- **Tap:** Select, confirm
- **Pinch:** Zoom
- **Swipe:** Camera rotation

### Session Design
- Auto-save constantly
- Operations continue offline (limited)
- Quick actions for checking markets
- Notifications for completed operations

---

## Technical Targets

### Performance
- 30 FPS stable on mid-range devices
- 60 FPS option on high-end
- Seamless planet transitions (streaming)
- Server-authoritative economy

### Supported Devices
- **iOS:** iPhone 8+, iOS 14+
- **Android:** Android 9+, 3GB RAM minimum

---

## Vertical Slice (Phase 1)

### Deliverables
- [ ] 1 Star system with 3 planets
- [ ] 1 Major city (Nova Prime) with market
- [ ] Basic ship (starter Scout/Freighter)
- [ ] Mining loop: find → extract → refine → sell
- [ ] 5 resource types
- [ ] Basic crafting (tools, components)
- [ ] 1 Drone type (Mining Drone)
- [ ] Save/load system
- [ ] 15-minute playable demo

### Success Criteria
- Player can earn OMEN through gameplay loop
- Market prices respond to supply
- Automation provides passive income
- Core loop feels satisfying

---

## Open Questions

1. **Offline progression** - How much can happen while player is away?
2. **Planet instances** - Shared world or instanced per player?
3. **Starting experience** - Tutorial mission or freeform?

---

*Document Version: 0.2*
*Last Updated: 2026-02-02*
