# Odyssey - Space Exploration RPG Demo

**Odyssey** is a mobile-first sci-fi space exploration RPG built in Unreal Engine 5. This 10-minute preview demonstrates the core gameplay loop of exploration, resource extraction, crafting, trading, and character progression.

## Project Overview

### Core Concept
Players pilot a space exploration vessel, mining resources across alien worlds, refining materials into valuable goods, and trading for currency to upgrade their capabilities. The game features a satisfying progression loop designed to be immediately accessible yet strategically deep.

### Target Platform
- **Primary**: iOS and Android mobile devices
- **Secondary**: Desktop for development and testing
- **Performance Target**: 30+ FPS on mid-range mobile devices
- **Demo Duration**: 10-minute guided experience

## System Architecture

### Core Systems

#### 1. 2.5D Isometric Camera System (`OdysseyCameraActor`, `OdysseyCameraPawn`)
- Fixed 45° isometric perspective for consistent visual style
- Smooth following camera with configurable follow speed
- Orthographic projection optimized for mobile rendering
- Screen-to-world coordinate conversion for touch input

#### 2. Mobile Touch Controls (`OdysseyTouchInterface`, `OdysseyPlayerController`)
- Virtual joystick for smooth analog movement
- Touch-to-interact system for mining and UI interaction
- Platform-aware control visibility (mobile vs desktop)
- Configurable control positions and sensitivity

#### 3. Resource Mining System (`ResourceNode`, `OdysseyInventoryComponent`)
- Interactive resource nodes with regeneration mechanics
- Dynamic depletion and recovery states
- Stacking inventory system with configurable stack limits
- Mining difficulty and character skill progression

#### 4. Crafting System (`OdysseyCraftingComponent`, `CraftingStation`)
- Data-driven recipe system with ingredients and outputs
- Queue-based crafting with real-time progress tracking
- Station bonuses for improved efficiency and success rates
- Experience-based progression with skill improvements

#### 5. Trading Economy (`OdysseyTradingComponent`, `TradingStation`)
- Dynamic market pricing with volatility simulation
- OMEN currency system with buy/sell price spreads
- Station tiers offering improved trading rates
- Upgrade purchase system with immediate effect application

#### 6. Character Progression (`OdysseyCharacter` + Components)
- Mining power and speed upgrades
- Inventory capacity expansion
- Crafting efficiency improvements
- Integrated progression across all systems

#### 7. Mobile Optimization (`OdysseyMobileOptimizer`)
- Dynamic performance tier adjustment
- Real-time FPS monitoring and automatic quality scaling
- Device-specific configuration profiles
- Memory management and garbage collection optimization

#### 8. Tutorial System (`OdysseyTutorialManager`)
- Data-driven 8-step guided experience
- Action tracking and automatic progression
- UI highlighting and contextual guidance
- Integration with demo timing and completion metrics

## Technical Implementation

### Engine Configuration
- **Unreal Engine**: 5.4 with mobile-optimized rendering pipeline
- **Rendering**: Orthographic projection, disabled HDR, optimized shadows
- **Input**: Enhanced Input system with mobile touch interface
- **Platform**: Android (API 21+) and iOS (12.0+) targets

### Performance Features
- **Mobile HDR**: Disabled for performance and battery life
- **Shadow Quality**: Configurable cascade count and resolution
- **LOD System**: Distance-based level of detail for all assets
- **Memory Management**: Proactive garbage collection and asset cleanup

### Code Architecture
- **Component-Based**: Modular systems attached to characters and actors
- **Data-Driven**: JSON configuration for recipes, prices, upgrades, tutorials
- **Event-Driven**: Blueprint-compatible event system for UI integration
- **Platform-Agnostic**: Cross-platform compatibility with mobile optimizations

## Gameplay Flow

### 10-Minute Experience
1. **Minutes 1-2**: Tutorial introduction, basic movement, first mining operations
2. **Minutes 3-4**: Resource gathering, inventory management, crafting discovery
3. **Minutes 5-6**: Crafting refined materials, understanding value chains
4. **Minutes 7-8**: Trading refined goods, earning first OMEN currency
5. **Minutes 9-10**: Purchasing upgrades, demonstrating progression loop

### Core Loop
**Explore → Mine → Craft → Trade → Upgrade → Repeat**

Each cycle becomes more efficient as players upgrade their capabilities, creating a satisfying progression curve that encourages continued engagement.

## Content Configuration

### Resource Types
- **Raw Materials**: Silicate (basic), Carbon (organic)
- **Refined Materials**: Processed versions with 4x base value
- **Advanced Materials**: Composite materials with 10x raw value
- **Currency**: OMEN for purchasing upgrades and equipment

### Crafting Recipes
- **Basic Refining**: 3 Silicate → 1 Refined Silicate
- **Carbon Processing**: 2 Carbon → 1 Refined Carbon
- **Advanced Composites**: 2 Refined Silicate + 1 Refined Carbon → 1 Composite Material

### Upgrade Progression
- **Mining Power**: Increases resources extracted per operation
- **Mining Speed**: Increases operations per second
- **Inventory Capacity**: Adds cargo storage slots
- **Crafting Efficiency**: Improves crafting speed and success rates

## Development Tools

### Debug and Testing
- **Performance Metrics**: Real-time FPS, frame time, memory usage
- **Tutorial Debug**: Step progression tracking and objective completion
- **Economic Balance**: Market price tracking and transaction logging
- **Mobile Testing**: Device-specific performance profiling

### Configuration Management
- **JSON Data Tables**: Easy modification of recipes, prices, tutorial content
- **Device Profiles**: Platform-specific performance optimizations
- **Scalable Settings**: Automatic quality adjustment based on hardware

## File Structure

```
Odyssey/
├── Config/                          # Engine and platform configuration
│   ├── DefaultEngine.ini           # Core engine settings
│   ├── DefaultGame.ini             # Project-specific settings
│   └── DefaultDeviceProfiles.ini   # Platform-specific optimizations
├── Content/
│   ├── Blueprints/                 # Blueprint template organization
│   │   ├── Camera/                 # Isometric camera system
│   │   ├── Characters/             # Player character blueprints
│   │   ├── Crafting/              # Crafting stations and UI
│   │   ├── Systems/               # Core gameplay systems
│   │   ├── Trading/               # Trading stations and economy
│   │   └── UI/                    # User interface elements
│   ├── Data/                      # Configuration data tables
│   │   ├── ResourceDataTable.json # Resource definitions
│   │   ├── CraftingRecipes.json   # Recipe configurations
│   │   ├── MarketPrices.json      # Economic pricing data
│   │   ├── Upgrades.json          # Character progression options
│   │   └── TutorialSteps.json     # Tutorial step definitions
│   ├── Input/                     # Mobile input configuration
│   ├── Optimization/              # Performance optimization data
│   └── Tutorial/                  # Tutorial system documentation
└── Source/
    └── Odyssey/                   # C++ source code
        ├── OdysseyGameModeBase.*  # Core game mode
        ├── OdysseyCharacter.*     # Player character
        ├── OdysseyPlayerController.* # Input handling
        ├── OdysseyCameraActor.*   # Isometric camera
        ├── OdysseyInventoryComponent.* # Resource management
        ├── OdysseyCraftingComponent.* # Crafting system
        ├── OdysseyTradingComponent.* # Economic system
        ├── OdysseyTouchInterface.* # Mobile input
        ├── OdysseyMobileOptimizer.* # Performance optimization
        ├── OdysseyTutorialManager.* # Tutorial guidance
        ├── ResourceNode.*         # Mineable resource nodes
        ├── CraftingStation.*      # Crafting interaction points
        └── TradingStation.*       # Economic interaction points
```

## Future Expansion

### Planned Features
- **Galaxy Exploration**: Procedurally generated star systems
- **Advanced Crafting**: Complex multi-stage manufacturing
- **Fleet Management**: Multiple ships and automated systems
- **Multiplayer Trading**: Player-to-player economic interaction
- **Story Campaign**: Narrative-driven exploration missions

### Technical Roadmap
- **Procedural Generation**: Infinite world exploration
- **Advanced AI**: Dynamic market simulation and NPC behavior
- **Cloud Integration**: Cross-device progression and leaderboards
- **Extended Platform Support**: Console and VR adaptations

## Build Instructions

### Prerequisites
- Unreal Engine 5.4 or later
- Android Studio (for Android builds)
- Xcode (for iOS builds)

### Configuration
1. Open `Odyssey.uproject` in Unreal Engine
2. Configure target platform (Android/iOS) in Project Settings
3. Build and package for target platform
4. Deploy to device for testing

### Performance Testing
- Target: 30+ FPS on iPhone X / Samsung Galaxy S10 equivalent
- Memory: <2GB RAM usage during gameplay
- Storage: <200MB total package size

---

**Odyssey** demonstrates the potential for deep, engaging gameplay experiences on mobile platforms, combining the accessibility of touch controls with the strategic depth of resource management and character progression. The modular, data-driven architecture ensures easy expansion and modification while maintaining stable performance across a wide range of devices.