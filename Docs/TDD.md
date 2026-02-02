# Odyssey - Technical Design Document

## Overview

This document outlines the technical architecture and implementation details for Odyssey.

---

## Engine & Tools

### Primary Engine
- **Unreal Engine 5.4** (or latest stable)
- Blueprint-first development, C++ for optimization later

### Development Tools
- **IDE:** Visual Studio Code / Rider (for C++ later)
- **Version Control:** Git with Git LFS for large assets
- **Project Management:** GitHub Projects / Notion

### Target SDKs
- **iOS:** Xcode 15+, iOS SDK 17+
- **Android:** Android Studio, SDK 34+, NDK r25+

---

## Project Architecture

### Folder Structure
```
Odyssey/
├── Source/Odyssey/              # C++ source (future)
│   ├── Core/                    # Core game classes
│   ├── Characters/              # Character classes
│   ├── Combat/                  # Combat system
│   └── Odyssey.Build.cs         # Build configuration
├── Content/
│   ├── Blueprints/
│   │   ├── Core/
│   │   │   ├── BP_GameMode.uasset
│   │   │   ├── BP_GameState.uasset
│   │   │   └── BP_PlayerState.uasset
│   │   ├── Characters/
│   │   │   ├── Player/
│   │   │   │   └── BP_PlayerCharacter.uasset
│   │   │   └── Enemies/
│   │   │       └── BP_EnemyBase.uasset
│   │   ├── Combat/
│   │   │   ├── BP_CombatComponent.uasset
│   │   │   └── BP_DamageHandler.uasset
│   │   ├── UI/
│   │   │   ├── WBP_HUD.uasset
│   │   │   ├── WBP_Inventory.uasset
│   │   │   └── WBP_VirtualJoystick.uasset
│   │   └── Items/
│   │       ├── BP_ItemBase.uasset
│   │       └── BP_EquipmentBase.uasset
│   ├── Maps/
│   │   ├── MainMenu.umap
│   │   ├── VerdantHollow.umap
│   │   └── SunstoneVillage.umap
│   ├── Art/
│   │   ├── Characters/
│   │   ├── Environment/
│   │   ├── UI/
│   │   └── VFX/
│   ├── Audio/
│   │   ├── Music/
│   │   ├── SFX/
│   │   └── Ambient/
│   └── Data/
│       ├── DT_Items.uasset       # Item data table
│       ├── DT_Enemies.uasset     # Enemy data table
│       ├── DT_Skills.uasset      # Skill data table
│       └── DT_Quests.uasset      # Quest data table
├── Config/
│   ├── DefaultEngine.ini
│   ├── DefaultGame.ini
│   ├── DefaultInput.ini
│   └── DefaultScalability.ini
└── Plugins/                      # Third-party plugins
```

---

## Core Systems

### Game Framework Classes

#### OdysseyGameMode
- Controls game rules and flow
- Handles player spawning
- Manages game state transitions

#### OdysseyGameState
- Tracks global game data
- Quest state management
- World event coordination (for multiplayer)

#### OdysseyPlayerState
- Player-specific data (stats, inventory)
- Replicates to server (multiplayer)
- Persists to save game

#### OdysseyPlayerController
- Input handling
- Camera management
- UI ownership

---

## Character System

### BP_PlayerCharacter

**Components:**
- `SkeletalMeshComponent` - Visual representation
- `CapsuleComponent` - Collision
- `CharacterMovementComponent` - Movement (modified for isometric)
- `CombatComponent` - Combat logic
- `InventoryComponent` - Item management
- `StatsComponent` - Character statistics
- `AbilityComponent` - Skill management

**Key Variables:**
```
// Stats
CurrentHealth: float
MaxHealth: float
CurrentStamina: float
MaxStamina: float
Level: int32
Experience: int32

// Attributes
Strength: int32
Agility: int32
Intelligence: int32
Vitality: int32

// Combat State
bIsAttacking: bool
bIsDodging: bool
bIsStunned: bool
CurrentComboIndex: int32
```

### Movement System

Isometric movement requires input transformation:
```
// Pseudocode for isometric movement
WorldDirection = RotateVector(InputVector, 45 degrees around Z)
AddMovementInput(WorldDirection, Speed)
```

Camera remains fixed at isometric angle (45° pitch, -45° yaw).

---

## Combat System

### Damage Pipeline

```
1. Attack Initiated
   ↓
2. Hit Detection (Sphere/Box trace)
   ↓
3. Calculate Base Damage
   ↓
4. Apply Attacker Modifiers (stats, buffs)
   ↓
5. Apply Defender Modifiers (armor, resistances)
   ↓
6. Apply Elemental Multiplier
   ↓
7. Apply Critical Hit (if applicable)
   ↓
8. Apply Damage to Target
   ↓
9. Trigger Effects (knockback, status)
   ↓
10. Update UI/VFX
```

### Damage Types
| Type | Color | Strong Against | Weak Against |
|------|-------|----------------|--------------|
| Physical | White | - | Armored |
| Fire | Orange | Nature | Water |
| Water | Blue | Fire | Nature |
| Nature | Green | Water | Fire |
| Lightning | Yellow | Water | Nature |

### Status Effects
- **Burn:** DoT, reduced healing
- **Freeze:** Slowed movement, shatter bonus
- **Poison:** DoT, stacking
- **Stun:** Cannot act
- **Bleed:** DoT, increased on movement

---

## Inventory System

### Item Base Class

**Properties:**
```
ItemID: FName
DisplayName: FText
Description: FText
Icon: UTexture2D
ItemType: EItemType (Consumable, Equipment, Material, Quest)
Rarity: EItemRarity
MaxStackSize: int32
BaseValue: int32
```

### Equipment Subclass

**Additional Properties:**
```
EquipmentSlot: EEquipmentSlot
RequiredLevel: int32
StatModifiers: TMap<EStat, float>
SpecialEffects: TArray<FEquipmentEffect>
```

### Inventory Component

**Functions:**
- `AddItem(ItemID, Quantity) → bool`
- `RemoveItem(ItemID, Quantity) → bool`
- `HasItem(ItemID, Quantity) → bool`
- `GetItemCount(ItemID) → int32`
- `EquipItem(ItemID) → bool`
- `UnequipItem(Slot) → bool`

---

## Quest System

### Quest Data Structure

```
QuestID: FName
QuestName: FText
Description: FText
QuestGiver: FName (NPC ID)
Prerequisites: TArray<FName> (Quest IDs)
Objectives: TArray<FQuestObjective>
Rewards: FQuestRewards
```

### Quest Objective Types
- **Kill:** Defeat X enemies of type Y
- **Collect:** Gather X items of type Y
- **Location:** Reach location X
- **Interact:** Interact with object/NPC X
- **Escort:** Keep NPC alive to destination

### Quest Manager
- Tracks active quests
- Listens for objective events
- Handles completion and rewards
- Persists to save game

---

## Save System

### Save Game Structure

```
// Player Data
PlayerName: FString
Class: EPlayerClass
Level: int32
Experience: int32
Stats: FPlayerStats
Inventory: TArray<FInventorySlot>
Equipment: TMap<EEquipmentSlot, FName>
Skills: TArray<FName>
Position: FVector
Rotation: FRotator

// Progress Data
CompletedQuests: TArray<FName>
ActiveQuests: TArray<FActiveQuest>
DiscoveredAreas: TArray<FName>
UnlockedWaypoints: TArray<FName>

// Settings
AudioSettings: FAudioSettings
GraphicsSettings: FGraphicsSettings

// Timestamps
PlayTime: float
LastSaved: FDateTime
```

### Save Slots
- 3 manual save slots per device
- 1 auto-save slot (overwrites)
- Cloud sync for multiplayer (future)

---

## UI System

### Widget Hierarchy

```
WBP_GameHUD (Root)
├── WBP_VirtualJoystick
├── WBP_ActionButtons
│   ├── AttackButton
│   └── SkillButtons (1-4)
├── WBP_HealthBar
├── WBP_StaminaBar
├── WBP_Minimap
├── WBP_QuestTracker
└── WBP_QuickMenu (pause overlay)
```

### Input System (Enhanced Input)

**Input Mapping Contexts:**
- `IMC_Gameplay` - In-game controls
- `IMC_Menu` - Menu navigation
- `IMC_Dialogue` - Conversation controls

**Input Actions:**
- `IA_Move` - 2D axis for movement
- `IA_Attack` - Basic attack trigger
- `IA_Dodge` - Dodge trigger
- `IA_Skill1-4` - Skill triggers
- `IA_Interact` - Context interaction
- `IA_Pause` - Open pause menu

---

## Mobile Optimization

### Performance Budgets
- **Draw calls:** <200 per frame
- **Triangles:** <100K on screen
- **Texture memory:** <512MB
- **Target frame time:** 33ms (30 FPS)

### Optimization Strategies

**Rendering:**
- Forward shading (not deferred)
- Limited dynamic lights (1-2 max)
- Baked lighting where possible
- Simple post-processing (no SSR, SSAO)
- LOD system for all meshes

**Memory:**
- Texture streaming
- Asset streaming by area
- Object pooling for projectiles/effects
- Aggressive garbage collection

**CPU:**
- Navigation mesh baking
- Simplified physics (no complex ragdoll)
- AI LOD (distant enemies simplified)
- Event-driven over tick where possible

### Mobile-Specific Settings

```ini
# DefaultEngine.ini

[/Script/Engine.RendererSettings]
r.Mobile.EnableStaticAndCSMShadowReceivers=True
r.Mobile.AllowDistanceFieldShadows=False
r.Mobile.AllowMovableDirectionalLights=False
r.MobileHDR=False

[/Script/Engine.Engine]
bSmoothFrameRate=True
MinSmoothedFrameRate=24
MaxSmoothedFrameRate=30

[/Script/Engine.GarbageCollectionSettings]
gc.TimeBetweenPurgingPendingKillObjects=30
```

---

## Networking (Phase 3+)

### Architecture
- Dedicated server model (UE5 Dedicated Server)
- Client-server with server authority
- Replication for gameplay state

### Backend Services
```
Server/
├── api/                    # RESTful API
│   ├── src/
│   │   ├── auth/          # JWT authentication
│   │   ├── players/       # Player profiles
│   │   ├── matchmaking/   # Lobby system
│   │   └── leaderboards/  # Rankings
│   └── package.json
├── game-server/           # UE5 Dedicated Server
└── database/
    └── schema.sql         # PostgreSQL
```

### Replication Strategy
- **Server Authority:** Combat, inventory, quests
- **Client Predicted:** Movement, animations
- **Replicated:** Health, buffs, positions

---

## Build & Deployment

### Build Configurations
- **Development:** Debug symbols, logging
- **Testing:** Shipping code, test features
- **Shipping:** Full optimization, no debug

### Platform-Specific

**Android:**
- Target API: 34
- Min API: 28
- ABIs: arm64-v8a only (modern devices)
- Texture compression: ASTC

**iOS:**
- Deployment target: iOS 14.0
- Architecture: arm64 only
- Metal required

### CI/CD (Future)
- GitHub Actions for builds
- TestFlight for iOS beta
- Google Play Internal Testing for Android

---

## Testing Strategy

### Automated Tests
- Unit tests for core systems (C++)
- Blueprint functional tests
- Save/load verification

### Manual Testing
- Device matrix testing (low/mid/high end)
- Performance profiling sessions
- Playtest sessions

### Device Testing Matrix
| Tier | Example Devices | Target FPS |
|------|-----------------|------------|
| Low | iPhone 8, Pixel 3 | 30 |
| Mid | iPhone 11, Pixel 6 | 30-60 |
| High | iPhone 14+, Pixel 8 | 60 |

---

## Appendix: Naming Conventions

### Blueprints
- `BP_` - Blueprint class
- `WBP_` - Widget Blueprint
- `ABP_` - Animation Blueprint
- `BPI_` - Blueprint Interface

### Assets
- `SK_` - Skeletal Mesh
- `SM_` - Static Mesh
- `T_` - Texture
- `M_` - Material
- `MI_` - Material Instance
- `S_` - Sound
- `NS_` - Niagara System
- `DT_` - Data Table

### Code (C++)
- `A` prefix - Actor classes
- `U` prefix - UObject classes
- `F` prefix - Structs
- `E` prefix - Enums
- `I` prefix - Interfaces

---

*Document Version: 0.1*
*Last Updated: 2026-02-02*
