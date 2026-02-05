# Mobile Touch Controls System

This folder contains the mobile input system for Odyssey, including virtual joystick and touch interaction controls.

## Key Components

### C++ Classes
- **UOdysseyTouchInterface**: Component that handles all touch input and virtual controls
- **AOdysseyPlayerController**: Player controller with integrated touch support

### Touch Controls Configuration

#### Virtual Joystick (Movement)
- **Position**: Bottom-left corner (150, 700)
- **Size**: 200x200 pixels
- **Dead Zone**: 20% of radius
- **Sensitivity**: Configurable touch sensitivity multiplier

#### Interact Button
- **Position**: Bottom-right corner (1700, 700)
- **Size**: 120x120 pixels
- **Function**: Triggers interaction with nearby objects

## Implementation Features

### Touch Input Handling
- Multi-touch support with finger tracking
- Virtual joystick with dead zone and clamping
- Touch-to-interact for resource mining
- Automatic platform detection (Android/iOS)

### Isometric Movement Translation
- Touch input is transformed for diagonal isometric movement
- Forward/backward mapped to Y+Y movement vector
- Left/right mapped to X-Y movement vector
- Smooth analog movement support

### Visual Feedback
- Configurable opacity for touch controls
- Show/hide functionality for different platforms
- Touch control highlighting on press

## Blueprint Integration

### Creating Touch Interface Blueprint
1. Create Blueprint from UOdysseyTouchInterface
2. Configure control positions and sizes
3. Set up visual representations (UMG widgets)
4. Bind touch events to movement and interaction

### UMG Widget Setup
```cpp
// Example touch control widget binding
TouchInterface->OnMovementInputChanged.AddDynamic(this, &UWidget::OnMovementChanged);
TouchInterface->OnInteractPressed.AddDynamic(this, &UWidget::OnInteractPressed);
```

## Platform Configuration

### Mobile Optimizations
- Touch controls only visible on mobile platforms
- Automatic sensitivity scaling for different screen sizes
- Battery-efficient touch polling
- Memory-optimized control state management

### Desktop Fallback
- Touch controls hidden on desktop
- Mouse/keyboard input maintained through Enhanced Input
- Seamless platform switching

## Usage Notes

- Touch controls automatically initialize on mobile platforms
- Dead zone prevents accidental movement from small touches
- Joystick radius constrains maximum movement input
- All touch events are normalized for consistent behavior
- Support for customizable control layouts through data tables