# Odyssey Health & Damage Foundation System (Phase 1)

## Overview

Phase 1 implements the core health and damage systems that bridge existing combat events to actual damage application for NPC ships. The system provides dual-layer defense (shields + hull), per-type damage resistances, regeneration, damage-over-time effects, and visual health bar support -- all optimized for mobile.

## Architecture

```
AttackHit Event (OdysseyEventBus)
       |
       v
UOdysseyDamageProcessor (singleton)
  - Global & type multipliers
  - Critical hit roll
  - Distance falloff
  - Minimum damage floor
       |
       v
UNPCHealthComponent (per-actor)
  - Resistance reduction
  - Flat damage reduction
  - Shield absorption
  - Hull health application
  - Health state update
       |
       v
FHealthEventPayload (broadcast)
  - Local delegates (OnHealthChanged, OnActorDied, etc.)
  - Global OdysseyEventBus (DamageDealt event)
  - UI systems (health bars, damage numbers)
```

## Components

### UNPCHealthComponent

Component-based health tracking attached to any actor.

**Key features:**
- Dual-layer defense: shields absorb damage before hull
- Per-type damage resistances (0-100% reduction per FName)
- Flat damage reduction (subtracted after percentage resistance)
- True damage type bypasses all resistances
- Shield bleed-through ratio (configurable % of shield-absorbed damage leaks to hull)
- Health regeneration with combat-awareness (configurable delay and out-of-combat requirement)
- Shield regeneration with separate delay
- Damage-over-time effects (multiple concurrent, each with own tick interval)
- Health state tiers: Healthy > Damaged > Critical > Dying > Dead
- Immortality option (`bCanDie = false` clamps hull to 1 HP)
- Visual helpers: `GetHealthBarColor()`, `GetShieldBarColor()`, `ShouldShowHealthBar()`
- Mobile-optimized: 10 Hz tick rate for regen/DOT, minimal allocations

**Events:**
| Delegate | Signature | Trigger |
|---|---|---|
| `OnHealthChanged` | `FHealthEventPayload` | Any health/shield change |
| `OnHealthStateChanged` | `EHealthState` | State tier transition |
| `OnActorDied` | `AActor*` | Hull reaches 0 |
| `OnShieldBroken` | `AActor* Owner, AActor* Source` | Shields fully depleted |
| `OnShieldRestored` | `AActor* Owner, float Amount` | Shields regenerate to full |

### UOdysseyDamageProcessor

Centralized singleton that receives AttackHit events and routes calculated damage.

**Calculation pipeline:**
1. Base damage x Global multiplier
2. x Damage type multiplier
3. x Per-attack named modifiers
4. x Distance falloff (optional, configurable min/max range + exponent)
5. x Critical hit multiplier (if rolled)
6. Floor at minimum damage (default 1.0)

**Features:**
- Subscribes to `AttackHit` events on the OdysseyEventBus
- Publishes `DamageDealt` events after processing
- Lifetime combat statistics (events, crits, kills, avg processing time)
- Verbose logging mode for debugging
- Fallback to UE built-in damage for actors without health component

### ANPCShipEnhanced

Integration subclass that bridges existing `ANPCShip` to the new system.

**Features:**
- Overrides `TakeDamage()` to route through `UNPCHealthComponent`
- Auto-configures resistances, shields, and regen per `ENPCShipType`
- Synchronizes legacy variables (`CurrentHealth`, `CurrentShields`) for backward compatibility
- Forwards health events to existing Blueprint events (`OnHealthChanged`, `OnDamageTaken`, etc.)
- Maps health state changes to behavior modifications

## Ship Type Configuration

| Ship Type | Hull | Shields | Hull Regen | Shield Regen | Notable Resistances |
|-----------|------|---------|------------|--------------|---------------------|
| Civilian | 75 | 20 | 2/s | 4/s | 5% Energy |
| Pirate | 120 | 30 | None | 5/s | 15% Kinetic, +1 flat |
| Security | 150 | 60 | 4/s | 8/s | 20% Kinetic, 15% Energy, 10% Plasma, +2 flat |
| Escort | 100 | 80 | 6/s | 12/s | 25% Energy, 20% Plasma, 10% Kinetic, +1.5 flat |

## Integration Guide

### Adding to a new actor

```cpp
// In your actor header
UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
UNPCHealthComponent* HealthComponent;

// In constructor
HealthComponent = CreateDefaultSubobject<UNPCHealthComponent>(TEXT("HealthComponent"));

// In BeginPlay
HealthComponent->SetMaxHealth(200.0f);
HealthComponent->SetMaxShields(50.0f);
HealthComponent->SetShields(50.0f);
HealthComponent->SetDamageResistance(TEXT("Energy"), 0.15f);
HealthComponent->SetHealthRegenEnabled(true);
HealthComponent->SetHealthRegenRate(3.0f);

HealthComponent->OnHealthChanged.AddDynamic(this, &AMyActor::OnHealthChanged);
HealthComponent->OnActorDied.AddDynamic(this, &AMyActor::OnDied);
```

### Dealing damage via event bus (recommended)

```cpp
FCombatEventPayload AttackEvent;
AttackEvent.Initialize(EOdysseyEventType::AttackHit, AttackerShip);
AttackEvent.Attacker = AttackerShip;
AttackEvent.Target = TargetShip;
AttackEvent.DamageAmount = 75.0f;
AttackEvent.DamageType = TEXT("Plasma");
AttackEvent.HitLocation = HitResult.Location;

UOdysseyEventBus::Get()->PublishEvent(MakeShared<FCombatEventPayload>(AttackEvent));
// DamageProcessor picks it up automatically
```

### Dealing damage directly

```cpp
// Through the processor (applies global modifiers, crits, falloff)
UOdysseyDamageProcessor::Get()->DealDamage(TargetShip, 50.0f, TEXT("Laser"), AttackerShip);

// Through the health component directly (raw damage, only resistances apply)
UNPCHealthComponent* HC = Target->FindComponentByClass<UNPCHealthComponent>();
HC->TakeDamage(50.0f, AttackerShip, TEXT("Missile"));
```

### Applying damage-over-time

```cpp
UNPCHealthComponent* HC = Target->FindComponentByClass<UNPCHealthComponent>();
// 5 damage every 0.5s for 4 seconds = 40 total damage
HC->ApplyDamageOverTime(5.0f, 0.5f, 4.0f, TEXT("Plasma"), AttackerShip);
```

### Querying health for UI

```cpp
float HullPercent = HC->GetHealthPercentage();
float ShieldPercent = HC->GetShieldPercentage();
FLinearColor BarColor = HC->GetHealthBarColor();
bool bShowBar = HC->ShouldShowHealthBar();
EHealthState State = HC->GetHealthState();
```

## Event Flow

```
1. Weapon fires       -> publishes AttackHit event
2. DamageProcessor    -> subscribes, receives AttackHit
3. CalculateDamage()  -> global mult, type mult, crits, falloff
4. ApplyDamageToTarget()
   |-> FindHealthComponent()
   |-> HealthComponent->TakeDamageEx()
       |-> CalculateActualDamage()  (resistance, flat reduction)
       |-> ApplyDamageToShieldsAndHealth()  (shields first, then hull)
       |-> UpdateHealthState()  (tier transitions)
       |-> BroadcastHealthChangeEvent()  (local delegates + event bus)
5. DamageProcessor    -> publishes DamageDealt event
6. UI systems         -> subscribe to DamageDealt for floating numbers, health bars
```

## Performance Considerations

- **Health component ticks at 10 Hz** (0.1s interval) -- sufficient for regen and DOT
- **Damage calculations are O(n)** where n = number of modifiers (typically < 5)
- **No heap allocations during normal damage processing** (stack-allocated payloads)
- **Event bus broadcasts use shared pointers** only for the global bus path
- **DOT effects use RemoveAtSwap** for O(1) removal
- **Distance falloff is a simple power curve** -- no trig functions

## Files

| File | Purpose |
|------|---------|
| `NPCHealthComponent.h/cpp` | Core health component |
| `OdysseyDamageProcessor.h/cpp` | Central damage routing singleton |
| `NPCShipHealthIntegration.h/cpp` | ANPCShip integration subclass |

## Dependencies

- `OdysseyEventBus` -- event-driven architecture
- `OdysseyActionEvent` -- event payload definitions (FCombatEventPayload)
- `AOdysseyCharacter` -- base character class (via ANPCShip)
- `UNPCBehaviorComponent` -- NPC AI state machine (for health-driven behavior)

## Future (Phase 2+)

- Armor/blocking system in DamageProcessor
- Damage vulnerability multipliers (opposite of resistances)
- AOE damage with falloff per-target
- Healing-over-time effects
- Shield types (kinetic shield vs energy shield)
- Hit location multipliers (weak points)
- Damage log / combat replay
