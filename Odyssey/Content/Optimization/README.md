# Mobile Performance Optimization

This folder contains the complete mobile optimization system designed to ensure Odyssey runs smoothly at 30+ FPS on iOS and Android devices.

## Core Components

### C++ Classes
- **UOdysseyMobileOptimizer**: Dynamic performance monitoring and optimization component
- **FMobilePerformanceSettings**: Configurable performance tier settings
- **FPerformanceMetrics**: Real-time performance monitoring data

### Performance Tiers

#### High Performance (Desktop/Premium Mobile)
- **View Distance**: 100% (full range)
- **Shadow Quality**: 1024px resolution
- **Effects**: Full quality bloom and particles
- **Anti-Aliasing**: Enabled (MSAA x2 on iOS)
- **Render Scale**: 100%
- **Target Devices**: iPhone 12+, Samsung Galaxy S20+, Desktop

#### Medium Performance (Standard Mobile)
- **View Distance**: 80% (reduced LOD distance)
- **Shadow Quality**: 512px resolution
- **Effects**: Reduced particle density
- **Anti-Aliasing**: Disabled
- **Render Scale**: 90%
- **Target Devices**: iPhone X+, Samsung Galaxy S10+, mid-range Android

#### Low Performance (Budget Mobile)
- **View Distance**: 60% (aggressive LOD)
- **Shadow Quality**: 256px resolution
- **Effects**: Minimal particles, no bloom
- **Anti-Aliasing**: Disabled
- **Render Scale**: 80%
- **Target Devices**: iPhone 8, Samsung Galaxy A-series, budget Android

## Dynamic Optimization Features

### Automatic Performance Scaling
- **Real-time FPS monitoring**: Tracks performance over 2-second windows
- **Threshold-based adjustments**: Automatically downgrades/upgrades settings
- **Target FPS**: 30 FPS baseline with 25-35 FPS adjustment thresholds
- **Graceful degradation**: Maintains playability on all devices

### Device Detection
```cpp
// Automatic device classification
bool IsLowEndDevice = (RAM < 4GB || DetectedBenchmarkScore < Threshold);
EPerformanceTier OptimalTier = DetermineOptimalTier(DeviceCapabilities);
```

### Dynamic LOD System
- **Distance-based scaling**: Adjusts object detail based on performance
- **Real-time adjustment**: Responds to frame rate drops
- **Smooth transitions**: Prevents visual popping between LOD levels
- **Memory optimization**: Reduces texture streaming load

## Engine Configuration

### Mobile Rendering Settings
```ini
[Mobile Optimizations]
r.MobileHDR=False                    # Disable HDR for performance
r.MobileMSAA=1                       # Minimal anti-aliasing
r.Mobile.DisableVertexFog=True       # Remove expensive fog
r.Mobile.VirtualTextures=False       # Disable for memory savings
r.Shadow.CSM.MaxCascades=2          # Limit shadow cascades
r.ViewDistanceScale=0.8             # Reduce draw distance
```

### Device-Specific Profiles
- **Android Low-End**: Aggressive optimizations for <4GB RAM devices
- **Android Mid-Range**: Balanced settings for mainstream devices
- **iOS**: Optimized for Metal rendering with Apple-specific features
- **Auto-Detection**: Automatic profile selection based on device specs

## Performance Monitoring

### Real-Time Metrics
- **FPS Tracking**: Current, average, minimum, maximum frame rates
- **Frame Time**: Millisecond timing for performance analysis
- **Memory Usage**: RAM and VRAM consumption monitoring
- **Draw Call Counting**: Rendering efficiency metrics

### Performance Events
```cpp
OnPerformanceTierChanged(OldTier, NewTier);     // Tier adjustment
OnPerformanceThresholdReached(BelowThreshold); // FPS warning
OnMemoryWarning();                              // Low memory alert
```

### Adaptive Optimization
- **Proactive Scaling**: Adjusts before severe performance drops
- **Recovery Modes**: Attempts to restore quality when performance improves
- **Emergency Optimization**: Ultra-low settings for critical situations

## Memory Management

### Asset Optimization
- **Texture Streaming**: Limited pool size for mobile VRAM
- **Garbage Collection**: Proactive cleanup of unused assets
- **Asset Bundling**: Efficient loading of only necessary resources
- **Memory Pooling**: Reuse of common objects to reduce allocation

### Runtime Optimization
```cpp
// Memory optimization operations
OptimizeMemoryUsage();              // Force GC and asset cleanup
ClearUnusedAssets();               // Remove cached but unused assets
TrimTexturePool();                 // Reduce texture memory usage
```

## Mobile-Specific Features

### Touch Performance
- **Efficient Input Processing**: Optimized touch event handling
- **Minimal UI Overdraw**: Reduced transparency and complex UI
- **Battery Optimization**: Power-efficient rendering techniques

### Platform Integration
- **iOS Metal**: Apple-optimized rendering pipeline
- **Android Vulkan/OpenGL**: Flexible API selection based on device
- **Background Modes**: Proper app lifecycle management

## Configuration Files

### DeviceProfiles.ini
Defines platform-specific optimizations:
- Hardware capability detection
- Console variable overrides
- Performance tier mapping
- Memory allocation limits

### Engine.ini Mobile Settings
Core engine optimizations:
- Rendering pipeline configuration
- Physics simulation limits
- Audio quality settings
- Network optimization

## Demo Integration

### 10-Minute Performance Target
- **Consistent 30+ FPS**: Stable frame rate throughout demo
- **2-second startup**: Fast loading on mobile devices
- **<200MB RAM**: Memory efficient for background apps
- **Battery Friendly**: Minimal power consumption

### Progressive Loading
- **Essential Assets First**: Core gameplay assets loaded immediately
- **Streaming Content**: Non-critical assets loaded during play
- **Precompiled Shaders**: No runtime shader compilation hitches

## Optimization Strategies

### Rendering Optimizations
1. **Simplified Shaders**: Mobile-optimized materials without expensive operations
2. **Batching**: Reduced draw calls through instancing and batching
3. **Occlusion Culling**: Efficient hiding of non-visible geometry
4. **Distance Culling**: Aggressive LOD and distance-based hiding

### Gameplay Optimizations
1. **Tick Optimization**: Reduced component update frequency
2. **Physics Simplification**: Simplified collision shapes
3. **Effect Pooling**: Reused particle systems and effects
4. **Audio Compression**: Optimized sound file sizes and quality

## Performance Testing

### Automated Testing
- **Device Farm Integration**: Testing across multiple devices
- **Performance Regression Detection**: Automated benchmark comparisons
- **Memory Leak Detection**: Long-running stability tests

### Manual Testing Targets
- **Entry-Level Devices**: iPhone 8, Samsung Galaxy A52
- **Mid-Range Devices**: iPhone XR, Samsung Galaxy S10
- **Thermal Testing**: Extended play sessions for heat management

## Troubleshooting

### Common Issues
1. **Frame Rate Drops**: Check dynamic LOD system and shadow quality
2. **Memory Warnings**: Verify asset cleanup and texture streaming
3. **Startup Hitches**: Review precompiled shader cache
4. **Battery Drain**: Investigate unnecessary background processing

### Debug Tools
- **Performance HUD**: Real-time metrics overlay
- **Memory Profiler**: Asset usage and leak detection
- **Frame Analyzer**: Detailed rendering cost analysis

This optimization system ensures Odyssey delivers a consistent, high-quality experience across the full spectrum of mobile devices while maintaining the core gameplay vision.