# NPC Ship System Usage Guide

## Overview

The NPC Ship system consists of two main components:

1. **ANPCShip** - The main NPC class extending AOdysseyCharacter
2. **UNPCBehaviorComponent** - AI state management component

## Basic Usage

### Creating an NPC Ship in C++

```cpp
// Create an NPC ship programmatically
ANPCShip* NewPirateShip = ANPCShip::CreateNPCShip(
    GetWorld(), 
    ENPCShipType::Pirate, 
    FVector(1000, 0, 100)
);

// Set up patrol route
TArray<FVector> PatrolPoints = {
    FVector(0, 0, 100),
    FVector(1000, 0, 100),
    FVector(1000, 1000, 100),
    FVector(0, 1000, 100)
};
NewPirateShip->SetPatrolRoute(PatrolPoints);
NewPirateShip->StartPatrol();
```

### Creating an NPC Ship in Blueprint

1. Create a new Blueprint based on `ANPCShip`
2. Set the `ShipConfig` in the defaults:
   - `ShipType`: Choose from Civilian, Pirate, Security, Escort
   - `MaxHealth`: Ship's maximum health
   - `AttackDamage`: Damage per attack
   - `MovementSpeed`: Movement speed
   - `bCanRespawn`: Whether ship respawns after death

3. Configure the `BehaviorComponent`:
   - `DetectionRadius`: How far the ship can detect players
   - `EngagementRange`: Combat range
   - `bIsHostile`: Whether to attack players

## State Machine

The NPC ships use a simple state machine:

- **Idle**: Default state, ship is stationary
- **Patrolling**: Moving along patrol routes
- **Engaging**: In combat with a target
- **Dead**: Ship is destroyed/disabled

## Events

The system provides several Blueprint-implementable events:

### Combat Events
- `OnHealthChanged(OldHealth, NewHealth)`
- `OnShieldChanged(OldShield, NewShield)` 
- `OnDamageTaken(DamageAmount, DamageSource)`
- `OnDeath()`
- `OnRespawned()`
- `OnAttackPerformed(Target, Damage)`

### AI Events
- `OnBehaviorStateChanged(OldState, NewState)`
- `OnTargetAcquired(Target)`
- `OnTargetLost(Target)`
- `OnEngagementStarted(Target)`
- `OnEngagementEnded(Target)`

### Patrol Events
- `OnPatrolPointReached(PatrolIndex, PatrolPoint)`

## Performance Considerations

The system is optimized for mobile:

- **Limited Update Frequency**: AI updates at 10Hz, detection at 2Hz
- **Component-Based**: Behavior is modular and can be disabled
- **Event-Driven**: Integrates with Odyssey's event system
- **Memory Efficient**: Uses object pooling where possible

## Integration with Action System

NPC ships integrate with Odyssey's action system through events:

```cpp
void ANPCShip::AttackTarget(AOdysseyCharacter* Target)
{
    // ... damage calculation ...
    
    // Broadcast attack event to action system
    TMap<FString, FString> EventData;
    EventData.Add(TEXT("Target"), Target->GetName());
    EventData.Add(TEXT("Damage"), FString::Printf(TEXT("%.2f"), Damage));
    BroadcastNPCEvent(TEXT("Attack"), EventData);
}
```

## Configuration Examples

### Civilian Ship
```cpp
FNPCShipConfig CivilianConfig;
CivilianConfig.ShipType = ENPCShipType::Civilian;
CivilianConfig.MaxHealth = 75.0f;
CivilianConfig.AttackDamage = 10.0f; // Low damage
CivilianConfig.MovementSpeed = 300.0f;
CivilianConfig.bCanRespawn = true;

// Set behavior to non-hostile
BehaviorComponent->SetHostile(false);
```

### Pirate Ship
```cpp
FNPCShipConfig PirateConfig;
PirateConfig.ShipType = ENPCShipType::Pirate;
PirateConfig.MaxHealth = 120.0f;
PirateConfig.AttackDamage = 35.0f; // High damage
PirateConfig.MovementSpeed = 450.0f; // Fast
PirateConfig.bCanRespawn = false; // Don't respawn

// Set aggressive behavior
BehaviorComponent->SetHostile(true);
```

## Debugging

Enable logging to see AI behavior:

```cpp
UE_LOG(LogTemp, Warning, TEXT("NPCShip %s changed state from %d to %d"), 
    *GetName(), (int32)OldState, (int32)NewState);
```

Set up console commands for testing:
- `showdebug AI` - Show AI debug info
- `stat game` - Show performance stats

## Next Steps

This base implementation provides:
- ✅ Core NPC ship architecture
- ✅ Component-based AI system  
- ✅ State machine implementation
- ✅ Event system integration
- ✅ Mobile-optimized performance

Future enhancements could include:
- 🔄 Integration with combat actions
- 🔄 Advanced patrol behaviors
- 🔄 Formation flying
- 🔄 Dynamic difficulty adjustment
- 🔄 Faction system integration
