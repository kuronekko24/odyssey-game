# Odyssey Health & Damage Foundation System

## Overview

This implementation provides a robust, event-driven health and damage system for Odyssey's NPC Ship Combat. The system consists of two main components:

1. **UNPCHealthComponent** - Advanced health management for ships
2. **UOdysseyDamageProcessor** - Centralized damage calculation and routing

## Key Features

### NPCHealthComponent
- Event-driven health management with OdysseyEventBus integration
- Health state system (Healthy, Damaged, Critical, Dying, Dead)
- Configurable health regeneration with combat awareness
- Damage type resistances
- Mobile-optimized performance
- Blueprint-friendly interface
- Visual health bar support through events

### OdysseyDamageProcessor  
- Centralized damage processing from combat events
- Advanced damage calculation with modifiers and critical hits
- Bridges AttackHit events to actual damage application
- Performance metrics and debugging support
- Singleton pattern for global access
- Support for custom damage types and calculations

## Integration Guide

### Step 1: Adding Health Component to NPCShip

The existing NPCShip class has a basic health system. You can either:

**Option A: Replace existing health system (Recommended)**
```cpp
// In NPCShip.h, replace existing health variables with:
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
UNPCHealthComponent* HealthComponent;

// Remove these existing properties:
// float CurrentHealth;
// float MaxShields; 
// float CurrentShields;
// etc.
```

**Option B: Add alongside existing system**
```cpp
// In NPCShip.h, add the component:
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
UNPCHealthComponent* HealthComponent;

// Keep existing health system for backward compatibility
```

### Step 2: Initialize Components

In NPCShip constructor:
```cpp
// Create health component
HealthComponent = CreateDefaultSubobject<UNPCHealthComponent>(TEXT("HealthComponent"));
```

In NPCShip::BeginPlay():
```cpp
// Configure health component
if (HealthComponent)
{
    HealthComponent->SetMaxHealth(ShipConfig.MaxHealth);
    HealthComponent->SetHealthRegenEnabled(true);
    HealthComponent->SetHealthRegenRate(2.0f);
    HealthComponent->SetDamageResistance(TEXT("Energy"), 0.1f); // 10% energy resistance
}

// Initialize damage processor (once per game)
UOdysseyDamageProcessor* DamageProcessor = UOdysseyDamageProcessor::Get();
if (DamageProcessor && !DamageProcessor->IsInitialized())
{
    DamageProcessor->Initialize();
}
```

### Step 3: Replace Damage Methods

Replace existing NPCShip::TakeDamage with:
```cpp
void ANPCShip::TakeDamage(float DamageAmount, AActor* DamageSource)
{
    if (HealthComponent)
    {
        HealthComponent->TakeDamage(DamageAmount, DamageSource, TEXT("Combat"));
    }
}
```

### Step 4: Event Integration

Subscribe to health events for UI updates:
```cpp
// In BeginPlay()
if (HealthComponent)
{
    HealthComponent->OnHealthChanged.AddDynamic(this, &ANPCShip::OnNPCHealthChanged);
    HealthComponent->OnHealthStateChanged.AddDynamic(this, &ANPCShip::OnNPCHealthStateChanged);
    HealthComponent->OnActorDied.AddDynamic(this, &ANPCShip::OnNPCDied);
}
```

Add corresponding methods:
```cpp
UFUNCTION()
void OnNPCHealthChanged(const FHealthEventPayload& HealthData);

UFUNCTION()
void OnNPCHealthStateChanged(EHealthState NewState);

UFUNCTION()
void OnNPCDied(AActor* DiedActor);
```

## Usage Examples

### Basic Damage Application
```cpp
// Direct damage
UOdysseyDamageProcessor::Get()->DealDamage(TargetShip, 50.0f, TEXT("Laser"), AttackerShip);

// Through health component
if (UNPCHealthComponent* Health = TargetShip->FindComponentByClass<UNPCHealthComponent>())
{
    Health->TakeDamage(50.0f, AttackerShip, TEXT("Missile"));
}
```

### Event-Driven Combat
```cpp
// Publish attack event - damage processor will handle it automatically
FCombatEventPayload AttackEvent;
AttackEvent.Initialize(EOdysseyEventType::AttackHit, AttackerShip);
AttackEvent.Attacker = AttackerShip;
AttackEvent.Target = TargetShip;
AttackEvent.DamageAmount = 75.0f;
AttackEvent.DamageType = TEXT("Plasma");
AttackEvent.HitLocation = HitResult.Location;

UOdysseyEventBus::Get()->PublishEvent(MakeShared<FCombatEventPayload>(AttackEvent));
```

### Health State Reactions
```cpp
void ANPCShip::OnNPCHealthStateChanged(EHealthState NewState)
{
    switch (NewState)
    {
        case EHealthState::Critical:
            // Start emergency behavior, smoke effects
            BehaviorComponent->SetState(ENPCState::Retreating);
            break;
            
        case EHealthState::Dying:
            // Disable weapons, prepare for death
            BehaviorComponent->SetState(ENPCState::Disabled);
            break;
            
        case EHealthState::Dead:
            // Death animation, loot drop, etc.
            Die();
            break;
    }
}
```

### Configuration Examples
```cpp
// Damage processor configuration
UOdysseyDamageProcessor* DP = UOdysseyDamageProcessor::Get();
DP->SetGlobalDamageMultiplier(1.5f);  // 50% more damage globally
DP->SetDamageTypeMultiplier(TEXT("Kinetic"), 0.8f);  // Kinetic weapons do 20% less damage
DP->SetGlobalCriticalChance(0.15f);   // 15% critical chance
DP->SetCriticalHitsEnabled(true);

// Health component configuration
HealthComponent->SetHealthRegenEnabled(true);
HealthComponent->SetHealthRegenRate(5.0f);           // 5 HP/second
HealthComponent->SetHealthRegenDelay(3.0f);          // 3 second delay after damage
HealthComponent->SetDamageResistance(TEXT("Energy"), 0.25f);  // 25% energy resistance
HealthComponent->SetDamageResistance(TEXT("Kinetic"), 0.10f); // 10% kinetic resistance
```

## Performance Considerations

### Mobile Optimization
- Components use efficient tick intervals (0.1s for health regen)
- Event system uses object pooling to minimize allocations
- Damage calculations are optimized for mobile CPUs
- Health state changes are cached to avoid redundant updates

### Memory Management
- Health events use shared pointers for efficient memory usage
- Event subscriptions are automatically cleaned up
- Components properly unregister from event bus on destruction

## Debugging and Monitoring

### Statistics
```cpp
// Get damage processor statistics
FDamageProcessorStats Stats = UOdysseyDamageProcessor::Get()->GetStatistics();
UE_LOG(LogTemp, Log, TEXT("Total Damage Events: %lld, Average Processing: %.2fms"), 
    Stats.TotalDamageEventsProcessed, Stats.AverageProcessingTimeMs);

// Enable verbose logging for debugging
UOdysseyDamageProcessor::Get()->bVerboseLogging = true;
```

### Health Component Queries
```cpp
// Check health status
if (HealthComponent->GetHealthState() == EHealthState::Critical)
{
    // Ship is in critical condition
}

// Get health percentage for UI
float HealthPercent = HealthComponent->GetHealthPercentage();
HealthBar->SetPercent(HealthPercent);
```

## Event Flow

1. **Attack Initiated** → AttackHit event published
2. **Damage Processor** → Receives event, calculates damage
3. **Health Component** → Receives calculated damage
4. **Health Events** → Published for UI/gameplay systems
5. **State Changes** → Health state updates trigger behavior changes

## Future Enhancements

- Shield system integration
- Armor/damage reduction systems
- Damage over time effects
- Healing abilities and items
- Critical hit locations and damage multipliers
- Advanced blocking/dodging mechanics

## Files Created

- `/Source/Odyssey/NPCHealthComponent.h`
- `/Source/Odyssey/NPCHealthComponent.cpp`
- `/Source/Odyssey/OdysseyDamageProcessor.h`
- `/Source/Odyssey/OdysseyDamageProcessor.cpp`

## Dependencies

The system integrates with existing Odyssey components:
- OdysseyEventBus (for event-driven architecture)
- OdysseyActionEvent (for event payloads)
- AOdysseyCharacter (base character class)
- Existing mobile optimization systems

This system provides a solid foundation for expanding Odyssey's combat mechanics while maintaining performance and architectural consistency.
