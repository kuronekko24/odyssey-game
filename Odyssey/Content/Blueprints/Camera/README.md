# Isometric Camera System

This folder contains the isometric camera implementation for Odyssey's 2.5D perspective.

## Key Files

### Blueprint Templates (Create in UE5 Editor)
- **BP_OdysseyCameraPawn**: Main camera pawn with isometric view and follow functionality
- **BP_OdysseyCameraActor**: Alternative camera actor implementation

### C++ Classes
- **AOdysseyCameraPawn**: Primary isometric camera with spring arm and smooth following
- **AOdysseyCameraActor**: Simple camera actor for fixed isometric views

## Configuration

### Default Settings
- **Camera Distance**: 1500 units
- **Isometric Angle**: 35.26° (true isometric)
- **Camera Yaw**: -45° (diagonal view)
- **Orthographic Width**: 1920 units
- **Follow Speed**: 5.0 (for smooth camera following)

### Blueprint Creation Steps

1. Create Blueprint from AOdysseyCameraPawn class
2. Set as default view target in Game Mode
3. Configure follow target to player character
4. Adjust orthographic width for desired zoom level

## Usage Notes

- Camera automatically follows the player character
- Uses orthographic projection for true isometric look
- Spring arm prevents collision issues
- Smooth following can be toggled for different camera feels
- Screen-to-world coordinate conversion available for touch input