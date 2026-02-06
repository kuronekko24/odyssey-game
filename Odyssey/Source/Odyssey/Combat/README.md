# Phase 3: Combat & Targeting System

Mobile-first arcade combat system for Odyssey. Touch a ship to target it, and weapons
engage automatically -- simple, responsive, and satisfying.

## Architecture

```
CombatSystemController          (master coordinator)
  |
  +-- UTouchTargetingSystem     (touch input -> target selection)
  |
  +-- UAutoWeaponSystem         (automatic firing when target in range)
  |
  +-- UCombatFeedbackSystem     (reticle, damage numbers, health bars, hit markers)
```

All three subsystems are created as sibling ActorComponents on the player ship.
They communicate through direct references and delegates -- no global singletons
or event bus overhead during the hot path.

## Quick Start

```cpp
// Add to your player ship actor:
UCombatSystemController* Combat = CreateDefaultSubobject<UCombatSystemController>(TEXT("CombatSystem"));
// That's it. The controller creates and wires all subsystems automatically.
```

## Gameplay Flow

1. **Player touches an enemy ship** on screen
2. `UTouchTargetingSystem` performs a sphere sweep from the touch point
3. If a valid target is found, it becomes the locked target
4. `UAutoWeaponSystem` detects the lock and enters `Firing` state
5. Weapons fire automatically at the configured rate
6. `UCombatFeedbackSystem` shows:
   - Targeting reticle on the locked enemy
   - Floating damage numbers on each hit
   - Health bar above the damaged enemy
   - Hit marker flashes on successful hits
7. When the enemy dies, target clears and auto-targeting scans for next hostile

## Subsystem Details

### TouchTargetingSystem (10 Hz tick)
- **Screen-to-world sphere sweep** with expanded touch radius for mobile comfort
- **Priority scoring** weights distance, hostility, and low-health bias
- **Auto-targeting** re-evaluates every 0.4s when no target is selected
- **Continuous tracking** refreshes target position, health, and LOS each tick
- **Reticle state machine**: Hidden -> Searching -> Locking -> Locked -> Firing

### AutoWeaponSystem (20 Hz tick)
- **State machine**: Idle -> Scanning -> Locked -> Firing -> Cooldown
- **Lead-target prediction** for projectile weapons (configurable speed)
- **Accuracy spread cone** for weapon-feel tuning
- **Critical hits** with configurable chance and multiplier
- **Energy integration** with existing action button energy pool
- **Session statistics**: shots, hits, crits, DPS, kills

### CombatFeedbackSystem (30 Hz tick)
- **Data-driven**: exposes arrays of display data; UI widgets poll it
- **Reticle display data**: position, color, state, pulse animation
- **Floating damage numbers**: world-anchored, drift upward, fade out
- **Health bar tracking**: appears on damage, fades after 4s, always shown on target
- **Hit markers**: brief screen-space flash on successful hit
- **Pool limits**: fixed max counts with oldest-eviction -- no unbounded growth

### CombatSystemController (10 Hz tick)
- **Creates and wires** all subsystems at BeginPlay
- **Integrates** with `UOdysseyTouchInterface` for touch routing
- **Registers** Attack and SpecialAttack with `UOdysseyActionButtonManager`
- **Pushes configuration** from editor-exposed structs to subsystems
- **Delegates** all heavy logic to subsystems -- near-zero own CPU cost

## Configuration Structs

| Struct               | Purpose                                          |
|----------------------|--------------------------------------------------|
| `FTargetingConfig`   | Max range, touch radius, auto-target interval    |
| `FAutoWeaponConfig`  | Damage, fire rate, range, accuracy, crit, energy |
| `FCombatFeedbackConfig` | Reticle size, damage number style, hit markers|

All structs are `UPROPERTY(EditAnywhere)` on the controller for designer tuning.

## Integration Points

| Existing System              | Integration                                          |
|------------------------------|------------------------------------------------------|
| `UOdysseyTouchInterface`    | `HandleCombatTouch()` called from touch handler      |
| `UOdysseyActionButtonManager` | Attack/SpecialAttack buttons auto-registered       |
| `UNPCHealthComponent`       | Damage applied through `TakeDamage()`                |
| `UNPCBehaviorComponent`     | Hostility queried for target prioritization          |
| `UOdysseyMobileOptimizer`   | Effect quality scales with performance tier          |

## Performance Budget

| Component          | Tick Rate | Target Budget |
|--------------------|-----------|---------------|
| Targeting          | 10 Hz     | < 0.1 ms      |
| Weapon             | 20 Hz     | < 0.1 ms      |
| Feedback           | 30 Hz     | < 0.15 ms     |
| Controller         | 10 Hz     | < 0.05 ms     |
| **Total**          |           | **< 0.5 ms**  |

## Files

| File                         | Lines | Purpose                          |
|------------------------------|-------|----------------------------------|
| `CombatTypes.h`              | ~280  | Shared enums, structs, configs   |
| `TouchTargetingSystem.h/cpp` | ~350  | Touch input + target management  |
| `AutoWeaponSystem.h/cpp`     | ~340  | Automatic weapon engagement      |
| `CombatFeedbackSystem.h/cpp` | ~380  | Visual feedback data management  |
| `CombatSystemController.h/cpp` | ~300 | Master coordinator + integration |
