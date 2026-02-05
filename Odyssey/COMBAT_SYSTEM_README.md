# Odyssey Combat & Targeting System

A mobile-first combat system for Odyssey that provides touch-based targeting, automatic weapon firing, and rich visual feedback. This system is designed to integrate seamlessly with the existing action button and event systems.

## 🎯 Features

### Touch-Based Targeting
- **Touch to Target**: Tap enemy ships to select them as targets
- **Visual Indicators**: Red targeting circles around selected enemies
- **Auto-Targeting**: Automatically engage nearest hostile when enabled
- **Line of Sight**: Smart targeting that respects obstacles

### Automatic Weapon System
- **Auto-Fire**: Weapons fire automatically when target is in range
- **Multiple Weapon Types**: Laser, Plasma, Kinetic, and Missile weapons
- **Energy Management**: Integrates with existing energy system
- **Visual Effects**: Muzzle flash, projectile trails, and impact effects

### Mobile-Optimized UI
- **Target Indicators**: Dynamic circles around targeted enemies
- **Health Bars**: Floating health bars above damaged enemies  
- **Damage Numbers**: Animated floating damage numbers
- **Hit Markers**: Visual confirmation of successful hits
- **Weapon Status**: Charge, reload, and energy indicators

### Performance Features
- **Widget Pooling**: Efficient UI element reuse for mobile performance
- **LOD System**: Distance-based optimization for effects
- **Event-Driven**: Minimal CPU overhead through smart event handling
- **Configurable Quality**: Adjustable settings for different devices

## 🏗️ Architecture

The combat system consists of several interconnected components:

### Core Components

#### `UOdysseyCombatTargetingComponent`
- Handles touch input for target selection
- Manages automatic target prioritization
- Validates targets (range, line of sight, team)
- Provides screen-to-world coordinate conversion

#### `UOdysseyCombatWeaponComponent`
- Manages weapon firing and states
- Handles different weapon types and modes
- Integrates with energy system
- Provides visual and audio effects

#### `UOdysseyCombatUIComponent`
- Manages all combat-related UI elements
- Widget pooling for performance optimization
- Real-time position updates for moving targets
- Damage number animations

#### `UOdysseyCombatManager`
- Master coordinator for all combat systems
- Integrates with existing action button system
- Handles combat state management
- Provides unified configuration interface

### Integration Components

#### `UOdysseyCombatIntegration`
- Bridges combat system with existing Odyssey systems
- Registers combat actions with action button manager
- Handles touch interface integration
- Provides backwards compatibility

#### `UOdysseyCombatDemo`
- Example implementation and setup
- Demo scenarios for testing
- Configuration examples
- Usage patterns demonstration

## 🚀 Quick Start

### 1. Add Combat System to Your Ship

```cpp
// In your ship's constructor or BeginPlay
UOdysseyCombatManager* CombatManager = CreateDefaultSubobject<UOdysseyCombatManager>(TEXT("CombatManager"));
UOdysseyCombatIntegration* CombatIntegration = CreateDefaultSubobject<UOdysseyCombatIntegration>(TEXT("CombatIntegration"));
```

### 2. Configure Combat Settings

```cpp
// Set up combat configuration
FCombatConfiguration Config;
Config.bEnableAutoTargeting = true;
Config.bEnableAutoFiring = true;
Config.bShowTargetIndicators = true;
Config.bShowHealthBars = true;
Config.bShowDamageNumbers = true;
Config.TargetingRange = 2000.0f;
Config.WeaponRange = 1500.0f;

CombatManager->SetCombatConfiguration(Config);
```

### 3. Initialize the System

```cpp
// Initialize combat system
CombatManager->InitializeCombatSystem();
```

### 4. Handle Touch Input

The system automatically integrates with `UOdysseyTouchInterface`. Touch events in the upper 2/3 of the screen are treated as targeting input.

### 5. Add Combat Actions

Combat actions are automatically registered with the action button manager:
- **Attack Button**: Fire weapon at current target
- **Special Attack Button**: Charged weapon attack

## 📱 Mobile Optimization

### Touch Interface
- **Large Touch Targets**: Enemies are easy to select on small screens
- **Visual Feedback**: Clear indication of touch responses
- **Gesture Recognition**: Distinguishes between targeting and UI touches

### Performance
- **Widget Pooling**: Reuses UI elements to reduce memory allocation
- **Update Frequency Control**: Configurable update rates for different devices
- **Effect Scaling**: Quality settings for particles and effects
- **Distance Culling**: Objects beyond range are not processed

### Battery Efficiency
- **Selective Updates**: Only updates active combat elements
- **Event-Driven Logic**: Minimizes continuous processing
- **Smart Targeting**: Reduces raycast frequency through caching

## ⚙️ Configuration

### Targeting Settings
```cpp
// Configure targeting behavior
TargetingComponent->SetTargetingMode(ETargetingMode::Assisted);
TargetingComponent->SetMaxTargetingRange(2000.0f);
```

### Weapon Settings
```cpp
// Configure weapon behavior
FWeaponStats WeaponStats;
WeaponStats.Damage = 25.0f;
WeaponStats.RateOfFire = 3.0f;
WeaponStats.Range = 1500.0f;
WeaponStats.Accuracy = 0.95f;
WeaponComponent->SetWeaponStats(WeaponStats);
```

### UI Settings
```cpp
// Configure UI elements
UIComponent->SetUIElementEnabled(ECombatUIElement::TargetIndicator, true);
UIComponent->SetUIElementEnabled(ECombatUIElement::HealthBar, true);
UIComponent->SetUIElementEnabled(ECombatUIElement::DamageNumber, true);
```

## 🎮 Integration with Existing Systems

### Action Button System
The combat system extends the existing `UOdysseyActionButtonManager` with new combat actions:
- Automatically registers Attack and Special Attack buttons
- Integrates with energy system for action costs
- Supports button cooldowns and visual feedback

### Event System
Fully integrated with `UOdysseyEventBus`:
- Publishes combat events for other systems to consume
- Subscribes to health, energy, and targeting events
- Maintains event-driven architecture consistency

### Touch Interface
Works seamlessly with `UOdysseyTouchInterface`:
- Extends touch handling for combat targeting
- Respects existing touch input priorities
- Maintains compatibility with other touch features

### Health System
Integrates with existing `UNPCHealthComponent`:
- Automatically shows health bars for damaged enemies
- Responds to health change events
- Handles target death notifications

## 🔧 Customization

### Custom Weapon Types
```cpp
// Create a custom weapon type
void SetupCustomWeapon()
{
    WeaponComponent->SetWeaponType(EWeaponType::Custom);
    
    FWeaponStats CustomStats;
    CustomStats.Damage = 50.0f;
    CustomStats.RateOfFire = 1.0f;
    CustomStats.Range = 2500.0f;
    WeaponComponent->SetWeaponStats(CustomStats);
}
```

### Custom UI Elements
```cpp
// Customize target indicator appearance
FTargetIndicatorConfig IndicatorConfig;
IndicatorConfig.IndicatorSize = 80.0f;
IndicatorConfig.IndicatorColor = FLinearColor::Blue;
IndicatorConfig.HostileColor = FLinearColor::Red;
UIComponent->SetTargetIndicatorConfig(IndicatorConfig);
```

### Custom Targeting Logic
```cpp
// Override target validation
ETargetValidation ValidateCustomTarget(AActor* Target)
{
    // Custom validation logic
    if (Target->ActorHasTag("Boss"))
    {
        return ETargetValidation::Valid;
    }
    
    return TargetingComponent->ValidateTarget(Target);
}
```

## 📊 Performance Metrics

The system tracks various performance and gameplay metrics:

### Combat Statistics
- **Shots Fired**: Total number of weapon discharges
- **Hit Accuracy**: Percentage of shots that hit targets
- **Critical Hit Rate**: Percentage of hits that were critical
- **Damage Dealt**: Total damage output
- **Enemies Destroyed**: Number of targets eliminated

### Performance Statistics
- **Average Frame Time**: Time spent in combat updates
- **UI Element Count**: Number of active UI widgets
- **Memory Usage**: Combat system memory footprint

## 🐛 Troubleshooting

### Common Issues

#### Targeting Not Working
- Ensure `UOdysseyCombatTargetingComponent` is added to your actor
- Check that targeting range is set appropriately
- Verify line of sight channels are configured correctly

#### Weapons Not Firing
- Check energy levels in action button manager
- Verify weapon component is enabled
- Ensure target is within weapon range

#### UI Elements Not Showing
- Confirm UI widget classes are set in the UI component
- Check that UI elements are enabled in configuration
- Verify widgets are being added to viewport

#### Performance Issues
- Reduce maximum UI element counts
- Increase update frequencies for better performance
- Disable non-essential visual effects

### Debug Commands

The system provides several debug features:

```cpp
// Enable debug drawing
CombatManager->SetDebugMode(true);

// Log combat statistics
CombatManager->LogCombatStats();

// Validate system integrity
CombatManager->ValidateSystem();
```

## 🔮 Future Enhancements

Potential areas for expansion:

### Combat Features
- **Formation Flying**: Group combat with wingmen
- **Electronic Warfare**: Jamming and countermeasures
- **Shield Systems**: Energy shields and penetration mechanics
- **Boarding Actions**: Close-quarters ship-to-ship combat

### AI Enhancements
- **Advanced Tactics**: Enemy AI that uses cover and formations
- **Dynamic Difficulty**: Combat difficulty that adapts to player skill
- **Faction Relations**: Complex alliance and hostility systems

### Visual Improvements
- **Damage Models**: Visual damage accumulation on ships
- **Environmental Effects**: Combat in nebulae, asteroid fields
- **Cinematic Mode**: Dramatic camera angles during combat

### Mobile Features
- **Haptic Feedback**: Controller vibration for hits and impacts
- **Voice Commands**: Voice-controlled targeting and commands
- **Gesture Controls**: Swipe-based combat maneuvers

## 📄 License

This combat system is part of the Odyssey project and follows the same licensing terms.

## 🤝 Contributing

When contributing to the combat system:

1. **Follow the Event-Driven Pattern**: Use the event bus for all inter-component communication
2. **Mobile-First Design**: Always consider mobile performance and touch interface
3. **Backwards Compatibility**: Ensure integration with existing Odyssey systems
4. **Performance Testing**: Test on target mobile devices
5. **Documentation**: Update this README with any new features or changes

## 📞 Support

For questions about the combat system:

- Check the troubleshooting section above
- Review the demo component (`UOdysseyCombatDemo`) for usage examples
- Examine the integration component (`UOdysseyCombatIntegration`) for system connections
- Look at the test scenarios in the demo for common use cases

The combat system is designed to be modular and extensible while maintaining the high performance requirements of mobile gaming. It provides a solid foundation for engaging space combat while preserving the accessibility and responsiveness that mobile players expect.
