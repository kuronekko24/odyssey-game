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

| City | Planet/System | Specialty | Market Focus |
|------|---------------|-----------|--------------|
| **Uurf** | Uurf / Sol Proxima | Starter world, Earth-like | General goods, high volume |
| **Spira** | Spira / Vulcan Belt | Industrial hub | Raw materials, ship parts |
| **Banx** | Banx / Central Nexus | Financial hub | All goods, lowest fees |
| **Cryo Haven** | Glacius | Research | Tech components, rare elements |
| **Dust Port** | Aridian Reach | Frontier | Mining equipment, survival gear |
| **Nexus Gate** | Outer Ring | Crossroads | Luxury goods, exotic materials |

### Uurf - The Starter World

Uurf is humanity's foothold in the galaxy - a lush, Earth-like planet and the first home for all new players.

**Characteristics:**
- **Environment:** Temperate, Earth-like conditions
- **Resources:** Plentiful common minerals, abundant organics
- **Economy:** Thriving, stable, beginner-friendly
- **Zone:** Mostly Friendly (PvE), Mild in outer regions
- **Population:** Highest in the galaxy

**Why Start Here:**
- Safe environment to learn game mechanics
- Abundant resources for early progression
- Active market with fair prices
- Tutorial takes place here
- Easy access to Sol Proxima system

**Uurf Regions:**
| Region | Zone | Resources | Notes |
|--------|------|-----------|-------|
| Central Hub | Friendly | Services only | Main city, no extraction |
| Verdant Plains | Friendly | Common ore, organics | Tutorial area |
| Iron Valleys | Friendly | Iron, copper, silicon | Beginner mining |
| Coastal Flats | Friendly | Organics, crystals | Safe farming |
| Northern Reaches | Mild | Uncommon ores | Slightly higher risk |

### City Special Capabilities

Each major city has unique services that make it the best location for specific activities.

#### Uurf (Sol Proxima) - Starter World & Trade Capital
- **Specialty:** Commerce, Banking & New Player Experience
- **Unique Perks:**
  - Lowest market transaction fees (2% vs 5% standard)
  - OMEN banking: earn interest on stored currency
  - Largest market volume - fastest buy/sell
  - Tutorial and starter missions
  - Beginner crafting stations (no fee)
- **Exclusive:** Starter blueprints, trade license certification

#### Spira (Vulcan Belt) - Industrial Heart
- **Specialty:** Ore Refining & Ship Construction
- **Unique Perks:**
  - 25% reduced refining costs
  - 15% faster refining speed
  - Access to T7-T8 refining (exclusive)
  - Shipyard with all hull sizes available
- **Exclusive:** Legendary alloys can ONLY be refined here

#### Banx (Central Nexus) - Financial Relic
- **Specialty:** Finance & Universal Trading
- **Unique Perks:**
  - Lowest marketplace fees in galaxy (1%)
  - Cross-market trading (access all city markets remotely)
  - OMEN exchange and currency services
  - Escrow services for large trades
  - Commodity futures contracts
- **Exclusive:** Galactic Trade License (reduces fees everywhere)
- **Lore:** An ancient station from the era of unified galactic banking. Its autonomous systems still run the most efficient financial networks known. No faction controls Banx - it remains neutral ground.

**Banx Location:**
Positioned at the Central Nexus - equidistant from all major systems. A natural crossroads for inter-system trade.

| Route | Distance |
|-------|----------|
| Banx → Uurf | Medium |
| Banx → Spira | Medium |
| Banx → Cryo Haven | Medium |
| Banx → Dust Port | Medium |
| Banx → Nexus Gate | Medium |

#### Cryo Haven (Glacius) - Research Hub
- **Specialty:** Technology & Blueprints
- **Unique Perks:**
  - Blueprint research lab (discover new recipes)
  - Reverse engineering facility (learn from items)
  - Tech component crafting bonus (+10% quality)
  - Sensor and electronics specialization
- **Exclusive:** Prototype (T7) blueprints only researched here

#### Dust Port (Aridian Reach) - Frontier Outpost
- **Specialty:** Mining & Survival Equipment
- **Unique Perks:**
  - Best prices on mining equipment
  - Drone assembly bonus (20% cheaper)
  - Survival gear crafting specialization
  - Expedition contracts to Hardcore zones
- **Exclusive:** Advanced mining drones only assembled here

#### Nexus Gate (Outer Ring) - Crossroads
- **Specialty:** Exotic Goods & Black Market
- **Unique Perks:**
  - Access to exotic/rare materials market
  - Faction reputation broker (change standings)
  - Smuggler contacts (high-risk contracts)
  - Gateway to Uncharted Space
- **Exclusive:** Exotic materials trading (T8 resources)

### System Zones

| System | Inner Zone | Outer Zone | Deep Space |
|--------|------------|------------|------------|
| **Sol Proxima** | Friendly | Mild | Mild |
| **Vulcan Belt** | Friendly | Mild | Full |
| **Central Nexus** | Friendly | Friendly | Mild |
| **Glacius** | Friendly | Full | Full |
| **Aridian Reach** | Mild | Full | Hardcore |
| **Outer Ring** | Mild | Full | Hardcore |
| **Uncharted Space** | - | Hardcore | Hardcore |

### Planet Instances

All planets are **shared instances** - every player exists in the same world. What you see, others see. Resources are contested. Territory matters.

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

## Storage System

### Planetary Storage

Each major city provides personal storage facilities. Storage is **per-city** - items stored at Nova Prime are not accessible at Forge Station.

#### What Can Be Stored
- Raw materials and refined goods
- Equipment and components
- Ships (docked)
- Drones
- Blueprints

#### Storage Tiers

| Tier | Item Capacity | Ship Bays | Cost to Upgrade |
|------|---------------|-----------|-----------------|
| **Basic** | 500 units | 1 | Free (starter) |
| **Standard** | 2,000 units | 2 | 5,000 OMEN |
| **Extended** | 10,000 units | 4 | 25,000 OMEN |
| **Premium** | 50,000 units | 8 | 100,000 OMEN |
| **Elite** | 250,000 units | 16 | 500,000 OMEN |
| **Unlimited** | 1,000,000 units | 32 | 2,000,000 OMEN |

**Notes:**
- Each city's storage is upgraded separately
- Ship bays hold one ship each regardless of size
- Stored items are 100% safe (no risk of loss)
- Retrieve items only when docked at that city

#### Storage Strategy

Players must decide where to store goods based on:
- **Proximity** - Store near where you operate
- **City Bonuses** - Store materials near processing facilities
- **Market Access** - Store goods near where you sell
- **Cost** - Upgrading all 5 cities is expensive

**Example Strategy:**
- Store raw ore at **Spira** (refining bonus)
- Store tech components at **Cryo Haven** (crafting bonus)
- Store trade goods at **Uurf** (low fees, high volume)
- Store expedition gear at **Dust Port** (frontier access)

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

### Hull System

Ships are defined by their **hull**, which determines size, capacity, and equipment tier limit.

#### Hull Sizes

| Size | Cargo Capacity | Equipment Slots | Base Speed | Example |
|------|----------------|-----------------|------------|---------|
| **XS (Extra Small)** | 50 units | 4 | Very Fast | Shuttle, Probe |
| **S (Small)** | 200 units | 6 | Fast | Scout, Interceptor |
| **M (Medium)** | 1,000 units | 8 | Medium | Freighter, Miner |
| **L (Large)** | 5,000 units | 10 | Slow | Heavy Freighter, Cruiser |
| **XL (Extra Large)** | 25,000 units | 12 | Very Slow | Capital, Carrier |

#### Hull Tiers (1-8)

Every hull has a tier that determines what equipment it can use.

| Tier | Quality | Equipment Limit | Availability |
|------|---------|-----------------|--------------|
| **T1** | Scrap | T1 only | Tutorial, starter |
| **T2** | Basic | T1-T2 | Common craft |
| **T3** | Standard | T1-T3 | Intermediate |
| **T4** | Quality | T1-T4 | Advanced craft |
| **T5** | Superior | T1-T5 | Rare materials |
| **T6** | Elite | T1-T6 | Full PvP zones |
| **T7** | Prototype | T1-T7 | Hardcore zones |
| **T8** | Legendary | T1-T8 | Endgame, faction rewards |

**Tier Rules:**
- A T5 hull can equip any equipment T1-T5
- A T3 hull CANNOT equip T4+ equipment
- Higher tier = better base stats + higher equipment ceiling
- Same size hull at higher tier is strictly better

### Ship Classes

| Class | Size | Role | Specialty |
|-------|------|------|-----------|
| **Shuttle** | XS | Transport | Fast travel, no combat |
| **Scout** | S | Exploration | Speed, sensors, stealth |
| **Interceptor** | S | Combat | Fast attack, light weapons |
| **Freighter** | M | Hauling | Max cargo for size |
| **Miner** | M | Extraction | Built-in mining bonuses |
| **Gunship** | M | Combat | Balanced offense/defense |
| **Heavy Freighter** | L | Hauling | Massive cargo capacity |
| **Cruiser** | L | Combat | Heavy weapons, tough |
| **Capital** | XL | Command | Fleet bonuses, drone control |
| **Carrier** | XL | Support | Launches fighter drones |

### Equipment System

All equipment is player-crafted and has tiers (T1-T8).

#### Equipment Categories

| Category | Function | Slot Type |
|----------|----------|-----------|
| **Weapons** | Damage dealing | Weapon slot |
| **Shields** | Damage absorption | Defense slot |
| **Armor** | Hull reinforcement | Defense slot |
| **Thrusters** | Speed, maneuverability | Engine slot |
| **Power Core** | Energy capacity | Core slot |
| **Sensors** | Detection, scanning | Utility slot |
| **Cargo Modules** | Extra storage | Utility slot |
| **Command Array** | Drone control | Utility slot |

#### Equipment Slots by Hull Size

| Hull Size | Weapon | Defense | Engine | Core | Utility |
|-----------|--------|---------|--------|------|---------|
| **XS** | 1 | 1 | 1 | 1 | 0 |
| **S** | 2 | 1 | 1 | 1 | 1 |
| **M** | 2 | 2 | 1 | 1 | 2 |
| **L** | 3 | 3 | 2 | 1 | 1 |
| **XL** | 4 | 4 | 2 | 2 | 2 |

#### Weapon Types

| Type | Range | Damage | Notes |
|------|-------|--------|-------|
| **Laser** | Medium | Energy | Consistent, drains shields |
| **Cannon** | Short | Kinetic | High burst, slow fire |
| **Missile** | Long | Explosive | Tracking, ammo-based |
| **Railgun** | Very Long | Kinetic | Sniper, charge time |
| **Mining Laser** | Short | Special | Resource extraction only |

#### Shield Types

| Type | Strength | Recharge | Notes |
|------|----------|----------|-------|
| **Standard** | Medium | Medium | Balanced |
| **Reinforced** | High | Slow | Tank builds |
| **Rapid** | Low | Fast | Hit and run |
| **Reflective** | Medium | Medium | Energy resist |
| **Ablative** | Medium | Medium | Kinetic resist |

#### Thruster Types

| Type | Speed | Agility | Fuel Use |
|------|-------|---------|----------|
| **Economy** | Slow | Low | Very Low |
| **Standard** | Medium | Medium | Medium |
| **Performance** | Fast | High | High |
| **Afterburner** | Boost | Low | Very High |

#### Sensor Types

| Type | Range | Function |
|------|-------|----------|
| **Basic Scanner** | Short | Nearby objects |
| **Deep Scanner** | Long | Distant detection |
| **Resource Scanner** | Medium | Mineral deposits |
| **Combat Scanner** | Medium | Enemy loadouts |
| **Stealth Detector** | Medium | Reveal cloaked ships |

### Fleet Management
- Own multiple ships
- Switch active ship at stations
- Assign ships to fleet (follow you)
- Fleet size limited by Command skill
- Ships in fleet consume fuel when traveling

---

## Economy & Markets

### Economic Philosophy

Odyssey runs a **closed-loop player-driven economy**:
- The game **ONLY generates raw resources** (minerals spawn in the world)
- **ALL equipment, items, ships are player-crafted** - no NPC vendors sell gear
- PvP deaths create **item sinks** - goods leave the economy
- Lost items are **recycled** back via NPC loot drops and quest rewards
- OMEN currency has multiple **sinks** to prevent inflation

```
[World] → Raw Resources → [Players] → Crafted Goods → [Market] → [Players]
                                            ↓
                                      [PvP Deaths]
                                            ↓
                                    [Game Inventory]
                                            ↓
                              [NPC Drops / Quest Chests]
                                            ↓
                                      [Players]
```

---

### Resource Extraction

All crafting begins with raw material extraction from planets.

#### Raw Material Categories

| Category | Examples | Found On | Uses |
|----------|----------|----------|------|
| **Stone** | Granite,Ite,Ite, Sandite | All planets | Basic construction, filler |
| **Common Ore** | Iron, Copper, Aluminum | Terrestrial, Barren | T1-T3 metals, components |
| **Industrial Ore** | Titanium, Tungsten, Cobalt | Volcanic, Asteroid | T4-T5 alloys, armor |
| **Rare Ore** | Platinum, Iridium, Osmium | Deep deposits, Moons | T6-T7 components |
| **Exotic Ore** | Neutronium, Darkmatter Residue | Hardcore zones only | T8 legendary gear |
| **Crystals** | Quartz, Ruby, Sapphire | Caves, volcanic vents | Energy systems, optics |
| **Mystical Gems** | Voidstone, Ethercrystal, Soulite | Ancient ruins, anomalies | T7-T8 special equipment |
| **Gases** | Hydrogen, Helium-3, Xenon | Gas giants, ice worlds | Fuel, coolants |
| **Organics** | Biomass, Polymers, Compounds | Terrestrial, forests | Consumables, plastics |

#### Resource Rarity by Zone

| Zone | Resource Tiers Available |
|------|--------------------------|
| Friendly | Common, some Industrial |
| Mild | Common, Industrial, some Rare |
| Full | Industrial, Rare, some Exotic |
| Hardcore | Rare, Exotic, Mystical Gems |

---

### Crafting Pipeline

Every resource has a purpose in the production chain.

```
Raw Ore → [Refine] → Ingots/Materials → [Craft] → Components → [Assemble] → Final Product
```

#### Production Stages

| Stage | Input | Output | Station |
|-------|-------|--------|---------|
| **Extraction** | Planet deposits | Raw ore, gems, gases | Mining equipment |
| **Refining** | Raw ore | Ingots, pure materials | Refinery |
| **Processing** | Gases, organics | Fuel, compounds | Processor |
| **Crafting** | Materials | Components, parts | Fabricator |
| **Assembly** | Components | Equipment, modules | Assembly Bay |
| **Construction** | Modules, hulls | Ships, structures | Shipyard |

#### Crafted Item Categories

| Category | Examples | Tradeable |
|----------|----------|-----------|
| **Consumables** | Fuel cells, repair kits, ammo | Yes |
| **Tools** | Mining drills, scanners | Yes |
| **Equipment** | Weapons, shields, thrusters | Yes |
| **Components** | Circuit boards, power cores | Yes |
| **Ship Modules** | Cargo holds, engines | Yes |
| **Ship Hulls** | Scout hull, freighter hull | Yes |
| **Complete Ships** | Assembled vessels | Yes |
| **Drones** | Mining, hauler, combat drones | Yes |

---

### OMEN Currency

#### Sources (How OMEN Enters Economy)
- **Selling goods** to other players (primary)
- **NPC contracts** - missions pay OMEN rewards
- **Bounties** - killing wanted players
- **Quest rewards** - story/side missions
- **Salvage** - selling NPC wreckage

#### Sinks (How OMEN Leaves Economy)

| Sink | Cost Range | Notes |
|------|------------|-------|
| **Repair Costs** | 1-10% of ship value | Based on damage taken |
| **Towing Costs** | 5-20% of ship value | Emergency rescue service |
| **Marketplace Fees** | 1-5% | Varies by city |
| **Blueprint Research** | 1K - 1M OMEN | Learning new recipes |
| **Station Services** | Variable | Refining, crafting station use |
| **Storage Upgrades** | 5K - 2M OMEN | Per city |
| **License Fees** | 10K - 500K OMEN | Trading, piloting certifications |
| **Insurance Premiums** | 1-5% of ship value | Per coverage period |
| **Faction Taxes** | 1-10% of earnings | If in a faction |
| **Warp Gate Tolls** | 100 - 10K OMEN | Fast travel between systems |

---

### Item Sinks (PvP Death Economy)

When a player dies in PvP, items are lost based on zone:

| Zone | Cargo Lost | Equipment Lost | Hull Lost |
|------|------------|----------------|-----------|
| Friendly | 0% | 0% | 0% |
| Mild | 100% | 0% | 0% |
| Full | 100% | 100% | 0% |
| Hardcore | 100% | 100% | 100% |

#### What Happens to Lost Items

```
Player Death → Items Dropped
                    ↓
         ┌─────────┴─────────┐
         ↓                   ↓
    70% to Killer      30% to Game
    (Lootable)         (Destroyed/Recycled)
```

**The 30% Game Take:**
- Items are removed from player economy
- Added to **Game Inventory** pool
- Redistributed as loot from:
  - NPC enemy drops
  - Boss defeat rewards
  - Quest completion chests
  - Exploration discoveries

**Why This Matters:**
- Prevents infinite item accumulation
- Creates demand for crafters (gear gets destroyed)
- Rewards PvE players with recycled gear
- Keeps economy balanced

#### Item Recycling Rates

| Source | Loot Quality |
|--------|--------------|
| Common NPCs | T1-T3 items, basic materials |
| Elite NPCs | T3-T5 items, components |
| Bosses | T5-T7 items, rare drops |
| World Bosses | T6-T8 items, unique cosmetics |
| Quest Chests | Scaled to quest difficulty |

---

### Market Mechanics

#### OMEN Currency Flow
- Universal currency across all systems
- Earned through selling goods
- Spent on: services, fees, sinks listed above

#### Supply & Demand
- Prices fluctuate based on player activity
- Each city has different demands
- Rare goods command premiums
- Oversupply crashes prices

#### Market Types
| Type | Description |
|------|-------------|
| **Buy Orders** | Players request items at set price |
| **Sell Orders** | Players list items for sale |
| **Instant Buy** | Buy at lowest sell price |
| **Instant Sell** | Sell at highest buy price |

#### Trade Routes
- Buy low in one system, sell high in another
- Distance affects profit potential
- Danger increases with value

### What CAN Be Traded
- Raw resources
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
- OMEN (no direct player-to-player transfer, market only)

---

## Player Progression

### Skill System Overview

Players progress by earning **Experience (EXP)** in specific skill trees. Each tree unlocks higher tier crafting, better bonuses, and specialized abilities.

**Core Principle:** Specialization is rewarded. Mastering one tree is more valuable than spreading thin across all.

---

### Skill Trees

#### Crafting Skill Trees

| Tree | Focus | Unlocks |
|------|-------|---------|
| **Metallurgy** | Ore refining, alloys | Refined metals, alloys, armor plates |
| **Engineering** | Components, machinery | Circuit boards, engines, power cores |
| **Weaponsmith** | Weapons, ammo | Lasers, cannons, missiles, railguns |
| **Shipwright** | Hulls, ship modules | Ship frames, cargo holds, thrusters |
| **Electronics** | Sensors, shields, tech | Scanners, shields, command arrays |
| **Dronecraft** | Drones, automation | Mining drones, haulers, combat drones |
| **Chemistry** | Fuel, consumables | Fuel cells, repair kits, boosters |

#### Non-Crafting Skill Trees

| Tree | Focus | Unlocks |
|------|-------|---------|
| **Mining** | Resource extraction | Better yields, rare finds, faster extraction |
| **Piloting** | Ship handling | Speed bonuses, fuel efficiency, maneuvers |
| **Trading** | Market mastery | Lower fees, price insights, bulk deals |
| **Combat** | Fighting ability | Damage bonuses, accuracy, defense |
| **Command** | Fleet & drones | More drones, larger fleets, AI efficiency |
| **Exploration** | Discovery | Better scanning, hidden locations, anomalies |

---

### Skill Progression (Tiers 1-8)

Each skill tree has 8 tiers matching the equipment tier system.

| Tier | Title | EXP Required | Crafting Unlocked |
|------|-------|--------------|-------------------|
| **T1** | Novice | 0 | Basic items, starter gear |
| **T2** | Apprentice | 1,000 | Common quality items |
| **T3** | Journeyman | 5,000 | Standard quality items |
| **T4** | Craftsman | 15,000 | Quality items |
| **T5** | Expert | 40,000 | Superior items |
| **T6** | Master | 100,000 | Elite items |
| **T7** | Grandmaster | 250,000 | Prototype items |
| **T8** | Legendary | 500,000 | Legendary items |

#### Earning EXP

| Activity | EXP Gained |
|----------|------------|
| Craft an item | Based on item tier and complexity |
| Successful high-tier craft | Bonus EXP |
| First-time craft | 2x EXP bonus |
| Craft for another player | Small EXP bonus |
| Research new blueprint | Large EXP bonus |

---

### Crafting Tier Bonuses

As you level a crafting tree, you gain cumulative bonuses:

| Tier | Material Cost | Success Rate | Craft Speed | Special |
|------|---------------|--------------|-------------|---------|
| T1 | 100% | 70% | 100% | - |
| T2 | 95% | 75% | 95% | - |
| T3 | 90% | 80% | 90% | Batch crafting (2x) |
| T4 | 85% | 85% | 85% | - |
| T5 | 80% | 90% | 80% | Batch crafting (5x) |
| T6 | 75% | 95% | 75% | Critical craft chance |
| T7 | 70% | 98% | 70% | Batch crafting (10x) |
| T8 | 60% | 99% | 60% | Masterwork chance |

**Bonus Explanations:**
- **Material Cost** - % of base materials required
- **Success Rate** - Chance craft succeeds (failure = partial material loss)
- **Craft Speed** - % of base crafting time
- **Batch Crafting** - Craft multiple items at once
- **Critical Craft** - Chance for +1 quality tier output
- **Masterwork** - Chance for unique bonuses on item

---

### Crafting Requirements

Higher tier items require more than just skill level.

#### Equipment Requirements by Tier

| Tier | Skill Level | Station Required | Special Requirements |
|------|-------------|------------------|---------------------|
| T1 | T1 | Portable Workbench | None |
| T2 | T2 | Portable Workbench | None |
| T3 | T3 | Basic Fabricator | None |
| T4 | T4 | Advanced Fabricator | None |
| T5 | T5 | Advanced Fabricator | Crafting Drone assist |
| T6 | T6 | Station Assembly Bay | Specialized tools |
| T7 | T7 | Station Assembly Bay | Research license |
| T8 | T8 | City Shipyard (specific) | Master certification |

#### Station Locations

| Station | Where Found |
|---------|-------------|
| Portable Workbench | Anywhere (personal) |
| Basic Fabricator | Ships (M+ size), Stations |
| Advanced Fabricator | Large ships, Major cities |
| Station Assembly Bay | Major cities only |
| City Shipyard | Specific cities (Spira best) |

---

### Specialization

Players are encouraged to specialize in 1-2 crafting trees.

**Why Specialize:**
- EXP requirements increase exponentially
- T8 mastery takes significant time investment
- Specialists command premium prices
- Reputation builds in your niche

**Specialization Paths:**

| Path | Primary Tree | Secondary Tree | Role |
|------|--------------|----------------|------|
| **Armorer** | Metallurgy | Weaponsmith | Combat gear |
| **Shipbuilder** | Shipwright | Engineering | Ship construction |
| **Tech Specialist** | Electronics | Engineering | High-tech components |
| **Drone Engineer** | Dronecraft | Electronics | Automation systems |
| **Industrial** | Metallurgy | Chemistry | Bulk materials |
| **Combat Crafter** | Weaponsmith | Electronics | Weapons + shields |

**Market Dynamics:**
- Specialists sell to each other
- Shipbuilder needs Armorer's plates
- Drone Engineer needs Tech Specialist's sensors
- Creates interdependent economy

---

### Blueprints

Blueprints unlock specific recipes within a skill tree.

#### How to Acquire Blueprints

| Method | Blueprint Types |
|--------|-----------------|
| **Tutorial** | T1 basic blueprints (free) |
| **Skill Level Up** | Core blueprints for that tier |
| **Research (Cryo Haven)** | Discover new variants |
| **Quest Rewards** | Unique/special blueprints |
| **Boss Drops** | Rare/prototype blueprints |
| **Exploration** | Ancient tech blueprints |
| **Faction Rewards** | Faction-specific designs |

#### Blueprint Properties

| Property | Description |
|----------|-------------|
| **Tier** | T1-T8, must have matching skill |
| **Category** | Which skill tree it belongs to |
| **Materials** | Required inputs |
| **Station** | Required crafting station |
| **Time** | Base craft duration |
| **Output** | What it produces |

#### Blueprint Rarity

| Rarity | Availability | Bonus |
|--------|--------------|-------|
| **Common** | Skill level up, vendors | None |
| **Uncommon** | Research, quests | 5% material reduction |
| **Rare** | Boss drops, exploration | 10% material reduction |
| **Epic** | World bosses, events | 15% + special variant |
| **Legendary** | T8 content, unique sources | 20% + unique properties |

**Note:** Blueprints are account-bound, cannot be traded.

---

### Reputation
- Reputation with each major city
- Unlocks better prices, exclusive contracts
- Lost through illegal activities or abandoning contracts
- High reputation unlocks city-specific blueprints

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

### Competitive
- Market competition
- Territory control
- PvP combat zones
- Leaderboards (wealth, production, exploration)

### Social
- Player shops in cities
- Contract system (hire other players)
- Chat, friends list

---

## Guilds & Factions (Future Feature)

### Guilds
Player-created organizations for cooperative play.

| Feature | Description |
|---------|-------------|
| **Guild Bank** | Shared resources and OMEN |
| **Guild Station** | Shared crafting/refining facilities |
| **Territory Claims** | Control planets or regions |
| **Guild Contracts** | Coordinated missions and goals |
| **Member Ranks** | Permissions and hierarchy |

### Factions
Large-scale alliances of guilds with political power.

| Feature | Description |
|---------|-------------|
| **Faction Territory** | Control entire systems |
| **Faction Wars** | Large-scale PvP conflicts |
| **Faction Bonuses** | Shared perks for members |
| **Political Influence** | Affect market taxes, zone rules |

### Militia System
Factions maintain standing military forces for territory defense.

**Militia Units:**
- **Patrol Ships** - AI-controlled faction ships that patrol territory
- **Defense Platforms** - Stationary turrets at key points
- **Response Fleets** - Auto-dispatch when members are attacked
- **Bounty Hunters** - Track and hunt hostile players

**Militia Funding:**
- Funded by faction taxes on member transactions
- Higher funding = stronger/more militia presence
- Members can donate ships and equipment
- Militia strength visible to all (deterrence)

**Militia Rules:**
- Only active in faction-controlled territory
- Cannot enter Friendly zones
- Engages anyone flagged hostile to faction
- Does not protect non-members (mercenary contracts possible)

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
- **No offline progression** - Game pauses when you're away
- Quick resume from where you left off
- All operations require active play

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
- [ ] 1 Star system (Sol Proxima) with Uurf + 2 planets
- [ ] 1 Major city (Uurf Central Hub) with market
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

## New Player Experience

### Tutorial Flow
Players complete a guided tutorial before accessing the open world.

**Tutorial Sequence:**
1. **Awakening** - Player wakes on damaged ship in Uurf orbit, learn basic controls
2. **First Landing** - Land on Uurf (Verdant Plains), learn movement/interaction
3. **Extraction 101** - Mine first resources with hand tool
4. **Refining Basics** - Use ship's refinery to process ore
5. **First Craft** - Create a basic mining drill
6. **To Market** - Travel to Uurf Central Hub, sell goods for first OMEN
7. **Your Journey Begins** - Tutorial complete, open world unlocked

**Tutorial Rewards:**
- Starter ship (basic Scout/Freighter hybrid)
- Starter mining drill
- 500 OMEN
- First blueprint (Basic Components)

**Post-Tutorial:**
- Full access to Sol Proxima system
- Other systems unlock via progression
- Tutorial can be replayed from menu (no rewards)

---

*Document Version: 0.3*
*Last Updated: 2026-02-02*
