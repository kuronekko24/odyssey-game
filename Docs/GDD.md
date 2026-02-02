# Odyssey - Game Design Document

## Overview

**Title:** Odyssey
**Genre:** RPG/Adventure with hybrid combat
**Art Style:** 2.5D Isometric
**Platform:** Mobile-first (iOS/Android), PC/Console later
**Target Audience:** Mid-core gamers who enjoy RPGs on mobile
**Monetization:** Free-to-play with cosmetic/convenience IAP

---

## Core Pillars

1. **Accessible Combat** - Easy to pick up, deep enough to master
2. **Bite-sized Sessions** - Playable in 5-15 minute chunks
3. **Meaningful Progression** - Every session should feel rewarding
4. **Social Connection** - Co-op with friends, compete with rivals

---

## Core Gameplay Loop

```
Explore → Fight → Loot → Upgrade → Explore (stronger)
```

### Session Loop (5-15 min)
1. Select a quest or dungeon
2. Navigate the area
3. Engage in combat encounters
4. Collect loot and rewards
5. Return to hub/save progress

### Progression Loop (days/weeks)
1. Complete story chapters
2. Unlock new areas and abilities
3. Gear up for harder content
4. Engage in multiplayer activities

---

## Combat System

### Hybrid Approach
Combines real-time action with strategic elements.

**Action Elements:**
- Direct control of character movement
- Combo-based basic attacks
- Dodge/roll with i-frames
- Active skill usage with cooldowns

**Strategic Elements:**
- Elemental rock-paper-scissors (Fire > Nature > Water > Fire)
- Status effects (Burn, Freeze, Poison, Stun)
- Enemy telegraph reading
- Optional pause for skill selection (accessibility)

### Controls (Mobile)
- **Left thumb:** Virtual joystick for movement
- **Right thumb:** Attack button, skill buttons (1-4)
- **Gesture:** Swipe to dodge

---

## Character System

### Player Character
- Customizable appearance (gender, face, hair, colors)
- Class selection affects starting stats and skills
- Can unlock cross-class skills later

### Classes (Initial 3)
| Class | Role | Primary Stat | Playstyle |
|-------|------|--------------|-----------|
| Warrior | Melee DPS/Tank | Strength | In your face, high damage |
| Ranger | Ranged DPS | Agility | Kiting, positioning |
| Mage | Burst/Support | Intelligence | High risk/reward, elemental |

### Stats
- **Health (HP)** - How much damage you can take
- **Stamina (SP)** - Used for dodging and sprinting
- **Strength** - Melee damage, carrying capacity
- **Agility** - Attack speed, dodge distance, crit chance
- **Intelligence** - Skill damage, mana pool
- **Vitality** - HP pool, HP regen

---

## Progression Systems

### Experience & Leveling
- XP from combat, quests, exploration
- Each level grants stat points and unlocks
- Level cap: 50 (expandable in updates)

### Skills
- Each class has a skill tree with 15-20 skills
- Equip up to 4 active skills at once
- Passive skills unlock at milestones

### Equipment
- Slots: Weapon, Helmet, Chest, Boots, 2x Accessories
- Rarity tiers: Common, Uncommon, Rare, Epic, Legendary
- Equipment has random stat rolls
- Enhancement system to power up gear

---

## World & Setting

### Premise
You are an adventurer awakened with no memory in a world where ancient gods have vanished, leaving behind dungeons filled with their power. Seek the truth of your past while uncovering what happened to the gods.

### Areas (Phase 1)
1. **Verdant Hollow** - Starting forest area, tutorials
2. **Sunstone Village** - Main hub, NPCs, shops
3. **Ruins of Althea** - First dungeon, fallen temple

### NPCs
- **Mira** - Innkeeper, quest giver, exposition
- **Forge Master Dorn** - Blacksmith, equipment crafting
- **Elder Sage** - Skill trainer, lore keeper

---

## Multiplayer (Future Phases)

### Co-op PvE
- 2-4 player parties
- Instanced dungeons
- Boss raids (4+ players)

### PvP
- 1v1 Arena
- 3v3 Team Arena
- Seasonal rankings with rewards

### MMO-lite Elements
- Shared hub zones
- World bosses on timers
- Guilds with perks

---

## Monetization Strategy

### Principles
- No pay-to-win
- Cosmetics-focused
- Respect player time

### Revenue Streams
1. **Cosmetic Shop** - Skins, pets, mounts, effects
2. **Battle Pass** - Seasonal content track (free + premium)
3. **Convenience** - Extra inventory, XP boosters
4. **Currency Packs** - Premium currency for shop

### What is NOT Sold
- Direct power (stats, damage)
- Exclusive gameplay content
- Must-have items

---

## Art Direction

### 2.5D Isometric Style
- Hand-painted textures on 3D geometry
- Strong silhouettes for mobile readability
- Vibrant but not oversaturated colors
- Clear enemy telegraphs (glows, particles)

### UI Principles
- Large touch targets (min 44pt)
- Clear iconography
- Minimal text where possible
- Consistent color language

---

## Audio Direction

### Music
- Orchestral with folk elements
- Dynamic layers based on combat state
- Distinct themes per area

### Sound Effects
- Satisfying combat feedback
- Clear ability cues
- Ambient environmental sounds

---

## Technical Targets

### Performance
- 30 FPS stable on mid-range devices (2020+)
- 60 FPS option on high-end
- Max 3GB RAM usage
- <100ms input latency

### Supported Devices
- **iOS:** iPhone 8+, iOS 14+
- **Android:** Android 9+, 3GB RAM minimum

---

## Milestones

### Vertical Slice (Phase 1 Complete)
- [ ] One playable area (Verdant Hollow)
- [ ] One class fully functional (Warrior)
- [ ] 5 enemy types
- [ ] Basic quest loop
- [ ] Inventory and equipment
- [ ] Save/load
- [ ] 10-minute playable demo

### Alpha
- [ ] All 3 classes
- [ ] 3 areas
- [ ] Core story beats
- [ ] Tutorial flow

### Beta
- [ ] Co-op multiplayer
- [ ] Polish pass
- [ ] Monetization integration
- [ ] Localization

---

## Open Questions

1. Should stamina regenerate passively or require actions?
2. Auto-attack vs. manual attack for accessibility?
3. Portrait or landscape orientation?
4. Synchronous or asynchronous multiplayer?

---

*Document Version: 0.1*
*Last Updated: 2026-02-02*
