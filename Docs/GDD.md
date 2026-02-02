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

### City Special Capabilities

Each major city has unique services that make it the best location for specific activities.

#### Nova Prime (Sol Proxima) - Trade Capital
- **Specialty:** Commerce & Banking
- **Unique Perks:**
  - Lowest market transaction fees (2% vs 5% standard)
  - OMEN banking: earn interest on stored currency
  - Largest market volume - fastest buy/sell
  - Trade license certification (unlocks advanced trading)
- **Exclusive:** T7-T8 trade contracts only available here

#### Forge Station (Vulcan Belt) - Industrial Heart
- **Specialty:** Ore Refining & Ship Construction
- **Unique Perks:**
  - 25% reduced refining costs
  - 15% faster refining speed
  - Access to T7-T8 refining (exclusive)
  - Shipyard with all hull sizes available
- **Exclusive:** Legendary alloys can ONLY be refined here

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
- Store raw ore at **Forge Station** (refining bonus)
- Store tech components at **Cryo Haven** (crafting bonus)
- Store trade goods at **Nova Prime** (low fees)
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

## New Player Experience

### Tutorial Flow
Players complete a guided tutorial before accessing the open world.

**Tutorial Sequence:**
1. **Awakening** - Player wakes on damaged ship, learn basic controls
2. **First Landing** - Land on tutorial planet, learn movement/interaction
3. **Extraction 101** - Mine first resources with hand tool
4. **Refining Basics** - Use ship's refinery to process ore
5. **First Craft** - Create a basic mining drill
6. **To Market** - Travel to Nova Prime, sell goods for first OMEN
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
