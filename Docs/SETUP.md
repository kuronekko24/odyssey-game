# Odyssey - Developer Setup Guide

## Quick Start (Existing Developer)

```bash
# 1. Clone the repository
git clone https://github.com/kuronekko24/odyssey-game.git
cd odyssey-game

# 2. Open in Unreal Engine
open Odyssey/Odyssey.uproject

# 3. Build C++ code (in UE5 Editor)
# Build menu → Build Solution (or Ctrl+Shift+B / Cmd+Shift+B)

# 4. Run tests (in UE5 Editor)
# Tools → Session Frontend → Automation → Filter: "Odyssey" → Run All

# 5. Play in Editor
# Click Play button or Alt+P for desktop preview
# For mobile preview: Play → Mobile Preview (ES3.1)
```

---

## Prerequisites

### Required Software

1. **Epic Games Launcher**
   - Download: https://www.unrealengine.com/download
   - Create/login to Epic Games account

2. **Unreal Engine 5.4+**
   - Install via Epic Games Launcher
   - Select iOS and Android target platforms during install

3. **Xcode** (for iOS, Mac only)
   - Install from Mac App Store
   - After install: `sudo xcodebuild -license accept`

4. **Android Studio** (for Android)
   - Download: https://developer.android.com/studio
   - Install Android SDK and NDK via SDK Manager

5. **Git & Git LFS**
   ```bash
   brew install git git-lfs
   git lfs install
   ```

---

## Step 1: Clone & Set Up Repository

```bash
# Clone the repository
git clone https://github.com/kuronekko24/odyssey-game.git
cd odyssey-game

# Verify project structure
ls Odyssey/Odyssey.uproject    # UE5 project file
ls Odyssey/Source/Odyssey/     # C++ source code
ls Docs/                       # Design documentation
```

### Project Structure Overview
```
odyssey-game/
├── Odyssey/
│   ├── Odyssey.uproject           # Open this in UE5
│   ├── Config/                    # Engine & platform settings
│   ├── Source/Odyssey/            # All C++ game code
│   │   ├── Economy/               #   Dynamic economy simulation
│   │   ├── Procedural/            #   Planet generation
│   │   ├── Crafting/              #   Crafting & automation
│   │   ├── Social/                #   Guilds & reputation
│   │   ├── Combat/                #   Touch targeting & combat
│   │   ├── Tests/                 #   776 automation tests
│   │   │   ├── Combat/            #     167 combat tests
│   │   │   ├── Social/            #     156 social tests
│   │   │   ├── Procedural/        #     159 procedural tests
│   │   │   ├── Crafting/          #     148 crafting tests
│   │   │   └── Economy/           #     146 economy tests
│   │   ├── NPC*.h/cpp             #   NPC ship systems
│   │   ├── Odyssey*.h/cpp         #   Core game systems
│   │   └── Odyssey.Build.cs       #   Build configuration
│   └── Content/                   # Blueprints, data, assets
├── Docs/
│   ├── GDD.md                     # Game Design Document
│   ├── TDD.md                     # Technical Design Document
│   ├── Storyline.md               # Narrative & lore
│   ├── OMEN_Cosmic_Expansion.md   # OMEN consciousness system
│   └── SETUP.md                   # This file
└── README.md
```

---

## Step 2: Install Unreal Engine

1. Open Epic Games Launcher
2. Navigate to **Unreal Engine** tab
3. Click **Install Engine**
4. Select **UE 5.4** (or latest stable)
5. Check installation options:
   - [x] Editor Symbols for debugging
   - [x] Target Platforms → **iOS**
   - [x] Target Platforms → **Android**
   - [x] Starter Content (optional)
6. Click **Install** (~40-60 GB download)

---

## Step 2: Configure Android SDK

### Via Android Studio

1. Open Android Studio
2. Go to **Tools → SDK Manager**
3. In **SDK Platforms** tab, install:
   - Android 14.0 (API 34)
   - Android 13.0 (API 33)
4. In **SDK Tools** tab, install:
   - Android SDK Build-Tools 34.0.0
   - NDK (Side by side) - version 25.x or 26.x
   - Android SDK Command-line Tools
   - Android SDK Platform-Tools
5. Note the SDK location (usually `~/Library/Android/sdk`)

### Configure in Unreal Engine

1. Open any Unreal project (or create a blank one)
2. Go to **Edit → Project Settings → Platforms → Android**
3. Click **Configure Now** for Android SDK
4. Set paths if not auto-detected:
   - SDK: `~/Library/Android/sdk`
   - NDK: `~/Library/Android/sdk/ndk/<version>`
   - Java: `/Library/Java/JavaVirtualMachines/jdk-XX/Contents/Home`

---

## Step 3: Configure iOS (Mac Only)

1. Install Xcode from App Store
2. Open Terminal and run:
   ```bash
   sudo xcodebuild -license accept
   xcode-select --install
   ```
3. Unreal Engine will auto-detect Xcode

### Apple Developer Account (for device testing)

1. Sign up at https://developer.apple.com
2. In Xcode: **Settings → Accounts → Add Apple ID**
3. Create Development Certificate and Provisioning Profile

---

## Step 4: Create Odyssey Project

### Option A: Via Unreal Editor

1. Open Epic Games Launcher → Launch UE5
2. Select **Games** category
3. Choose **Blank** template
4. Configure:
   - Project Type: **Blueprint**
   - Target Platform: **Mobile**
   - Quality Preset: **Scalable**
   - Starter Content: **No** (keep project clean)
5. Project Location: `~/dev/Odyssey`
6. Project Name: `Odyssey`
7. Click **Create**

### Option B: Via Command Line (Advanced)

```bash
# Create project using UnrealVersionSelector
/Users/Shared/Epic\ Games/UE_5.4/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor \
  ~/dev/Odyssey/Odyssey/Odyssey.uproject \
  -game -CreateProject
```

---

## Step 5: Initial Project Configuration

### Mobile Settings

1. Open Project Settings (**Edit → Project Settings**)

2. **Maps & Modes:**
   - Set default GameMode (create BP_OdysseyGameMode first)

3. **Platforms → iOS:**
   - Bundle Identifier: `com.yourname.odyssey`
   - Enable Touch Interface

4. **Platforms → Android:**
   - Package Name: `com.yourname.odyssey`
   - Minimum SDK Version: 28
   - Target SDK Version: 34
   - Enable Touch Interface

5. **Engine → Rendering:**
   - Mobile HDR: Off (better performance)
   - Auto Exposure: Off (more predictable lighting)

6. **Engine → Input:**
   - Configure default touch input

---

## Step 6: Set Up Git LFS

Large binary files (textures, meshes, audio) should use Git LFS.

```bash
cd ~/dev/Odyssey

# Install Git LFS (if not already)
brew install git-lfs

# Initialize LFS in repo
git lfs install

# Create .gitattributes for Unreal assets
cat > .gitattributes << 'EOF'
# Unreal Engine files
*.uasset filter=lfs diff=lfs merge=lfs -text
*.umap filter=lfs diff=lfs merge=lfs -text

# Binary assets
*.png filter=lfs diff=lfs merge=lfs -text
*.jpg filter=lfs diff=lfs merge=lfs -text
*.jpeg filter=lfs diff=lfs merge=lfs -text
*.tga filter=lfs diff=lfs merge=lfs -text
*.psd filter=lfs diff=lfs merge=lfs -text
*.wav filter=lfs diff=lfs merge=lfs -text
*.mp3 filter=lfs diff=lfs merge=lfs -text
*.ogg filter=lfs diff=lfs merge=lfs -text
*.fbx filter=lfs diff=lfs merge=lfs -text
*.obj filter=lfs diff=lfs merge=lfs -text
EOF

git add .gitattributes
```

---

## Step 7: First Build Test

### Test Editor Launch

1. Open `~/dev/Odyssey/Odyssey/Odyssey.uproject`
2. Verify editor opens without errors
3. Create a simple test level

### Test Mobile Preview

1. In Editor: **Play → Mobile Preview (ES3.1)**
2. Verify rendering looks correct

### Test Android Build

1. Connect Android device with USB debugging enabled
2. In Editor: **Platforms → Android → Launch**
3. Wait for build and deployment
4. Verify game runs on device

### Test iOS Build (Mac Only)

1. Connect iOS device
2. In Editor: **Platforms → iOS → Launch**
3. Wait for build and Xcode deployment
4. Verify game runs on device

---

## Running a Local Instance

### Option A: Play in Editor (Fastest)

1. Open `Odyssey/Odyssey.uproject` in Unreal Engine 5.4+
2. Wait for shaders to compile (first launch takes a few minutes)
3. Click **Play** button (or press `Alt+P`)
4. Game runs in the editor viewport with full debug capabilities

### Option B: Mobile Preview in Editor

1. In UE5 Editor toolbar: **Play → Mobile Preview (ES3.1)**
2. Simulates mobile rendering pipeline on desktop
3. Touch controls simulated with mouse clicks
4. Good for testing mobile optimization without a device

### Option C: Standalone Game Window

1. In UE5 Editor: **Play → Standalone Game**
2. Launches in a separate window at target resolution
3. Better performance testing than in-editor play
4. Close window to stop (or press `Esc`)

### Option D: Package for Device

**Android:**
```bash
# In UE5 Editor: Platforms → Android → Package Project
# Or via command line:
/path/to/UE5/Engine/Build/BatchFiles/RunUAT.sh BuildCookRun \
  -project="$HOME/dev/Odyssey/Odyssey/Odyssey.uproject" \
  -platform=Android -clientconfig=Development -cook -build -stage -pak -package
```

**iOS (Mac only):**
```bash
# In UE5 Editor: Platforms → iOS → Package Project
# Requires Apple Developer account and provisioning profile
```

---

## Building the C++ Code

### From UE5 Editor
1. Open `Odyssey.uproject`
2. If prompted to rebuild, click **Yes**
3. Manual rebuild: **Build → Build Solution** or `Ctrl+Shift+B` (Win) / `Cmd+Shift+B` (Mac)
4. Check **Output Log** for compilation errors

### From Command Line (Advanced)

**Mac:**
```bash
# Generate Xcode project files
/path/to/UE5/Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh \
  "$HOME/dev/Odyssey/Odyssey/Odyssey.uproject" -game

# Build via Xcode
xcodebuild -project Odyssey.xcodeproj -scheme OdysseyEditor -configuration Development build
```

**Windows:**
```bash
# Generate Visual Studio project files
"C:\Program Files\Epic Games\UE_5.4\Engine\Build\BatchFiles\GenerateProjectFiles.bat" ^
  "C:\dev\Odyssey\Odyssey\Odyssey.uproject" -game

# Build via MSBuild
MSBuild.exe Odyssey.sln /t:Build /p:Configuration="Development Editor"
```

### Build Configurations

| Config | Use Case | Performance | Debugging |
|--------|----------|-------------|-----------|
| **Development** | Day-to-day coding | Good | Full debug |
| **DebugGame** | Deep debugging | Slow | Maximum info |
| **Shipping** | Release builds | Maximum | None |

---

## Running the Test Suite

### From UE5 Editor (Recommended)

1. Open **Tools → Session Frontend** (or `Window → Developer Tools → Session Frontend`)
2. Click the **Automation** tab
3. In the test filter, type `Odyssey` to see all 776 tests
4. Filter by system:
   - `Odyssey.Combat` → 167 combat tests
   - `Odyssey.Social` → 156 social tests
   - `Odyssey.Economy` → 146 economy tests
   - `Odyssey.Procedural` → 159 procedural tests
   - `Odyssey.Crafting` → 148 crafting tests
5. Check the tests you want to run and click **Start Tests**
6. Results appear in the panel with pass/fail indicators

### From Command Line (CI/Headless)

```bash
# Run all Odyssey tests headless
/path/to/UE5/Engine/Binaries/Mac/UnrealEditor-Cmd \
  "$HOME/dev/Odyssey/Odyssey/Odyssey.uproject" \
  -ExecCmds="Automation RunTests Odyssey" \
  -Unattended -NullRHI -NoSound -NoSplash \
  -log -ReportOutputPath="$HOME/dev/Odyssey/TestResults"

# Run specific system tests
/path/to/UE5/Engine/Binaries/Mac/UnrealEditor-Cmd \
  "$HOME/dev/Odyssey/Odyssey/Odyssey.uproject" \
  -ExecCmds="Automation RunTests Odyssey.Combat" \
  -Unattended -NullRHI -NoSound -NoSplash

# Run with JUnit XML output (for CI integration)
/path/to/UE5/Engine/Binaries/Mac/UnrealEditor-Cmd \
  "$HOME/dev/Odyssey/Odyssey/Odyssey.uproject" \
  -ExecCmds="Automation RunTests Odyssey; Quit" \
  -Unattended -NullRHI -TestExit="Automation Test Queue Empty" \
  -ReportExportPath="$HOME/dev/Odyssey/TestResults"
```

### Test Organization

```
Odyssey/Source/Odyssey/Tests/
├── Combat/          # 9 files, 167 tests
│   ├── NPCHealthComponentTests.cpp
│   ├── OdysseyDamageProcessorTests.cpp
│   ├── NPCShipTests.cpp
│   ├── NPCBehaviorComponentTests.cpp
│   ├── TouchTargetingSystemTests.cpp
│   ├── AutoWeaponSystemTests.cpp
│   ├── NPCSpawnManagerTests.cpp
│   ├── CombatFeedbackSystemTests.cpp
│   └── CombatIntegrationTests.cpp
├── Social/          # 6 files, 156 tests
│   ├── GuildManagerTests.cpp
│   ├── ReputationSystemTests.cpp
│   ├── ContractSystemTests.cpp
│   ├── GuildEconomyTests.cpp
│   ├── CooperativeProjectTests.cpp
│   └── SocialIntegrationTests.cpp
├── Economy/         # 8 files, 146 tests
│   ├── MarketDataComponentTest.cpp
│   ├── PriceFluctuationSystemTest.cpp
│   ├── TradeRouteAnalyzerTest.cpp
│   ├── EconomicEventSystemTest.cpp
│   ├── EconomySaveSystemTest.cpp
│   ├── EconomyRippleEffectTest.cpp
│   ├── EconomyPerformanceTest.cpp
│   └── EconomyIntegrationTest.cpp
├── Procedural/      # 7 files, 159 tests
│   ├── PlanetGenerationTests.cpp
│   ├── ResourceDistributionTests.cpp
│   ├── BiomeDefinitionTests.cpp
│   ├── ExplorationRewardTests.cpp
│   ├── PlanetaryEconomyTests.cpp
│   ├── PerformanceTests.cpp
│   └── ProceduralIntegrationTests.cpp
└── Crafting/        # 7 files, 148 tests
    ├── OdysseyRecipeSystemTests.cpp
    ├── OdysseyCraftingManagerTests.cpp
    ├── OdysseyQualityControlTests.cpp
    ├── OdysseyAutomationNetworkTests.cpp
    ├── OdysseyCraftingSkillTests.cpp
    ├── OdysseyProductionChainPlannerTests.cpp
    └── OdysseyCraftingIntegrationTests.cpp
```

---

## Development Workflow

### Daily Development Cycle

```bash
# 1. Pull latest changes
cd ~/dev/Odyssey
git pull

# 2. Open project in UE5
open Odyssey/Odyssey.uproject

# 3. Make changes in C++ source or Blueprints

# 4. Build and test
# Build: Ctrl+Shift+B (Editor)
# Test: Tools → Session Frontend → Automation → Run

# 5. Commit and push after every feature
git add .
git commit -m "Brief description of changes"
git push
```

### Adding New Systems

1. Create `.h` and `.cpp` files in appropriate subdirectory under `Source/Odyssey/`
2. Add corresponding test file in `Tests/<SystemName>/`
3. Follow existing patterns (component-based, event-driven)
4. Run full test suite before committing
5. Update documentation if adding major features

### Code Architecture Patterns

**Component-Based Design:**
- All gameplay systems are UActorComponents
- Attach to actors for modular functionality
- Communicate through UE5 delegates and the OdysseyEventBus

**Event-Driven Architecture:**
- Systems communicate through events, not direct references
- Use OdysseyEventBus for cross-system communication
- Keeps systems decoupled and testable

**Mobile-First Optimization:**
- Performance tier system (High/Medium/Low)
- Object pooling for frequently spawned actors
- Distance-based LOD for NPC behavior
- Staggered updates to prevent frame spikes

---

## Troubleshooting

### Android Build Fails

1. Check SDK/NDK paths in Project Settings
2. Verify Java JDK is installed
3. Try: **File → Package Project → Android** for detailed errors
4. Check `Saved/Logs/` for build logs

### iOS Build Fails

1. Verify Xcode is installed and license accepted
2. Check provisioning profile in Xcode
3. For "code signing" errors, configure in Xcode directly

### Editor Performance

1. Lower editor viewport quality for better performance
2. Disable real-time thumbnails in Content Browser
3. Use **Edit → Editor Preferences → Performance**

---

## Recommended Learning Path

1. **UE5 Your First Hour** - Official Epic tutorial
2. **Blueprint Introduction** - Epic's Blueprint docs
3. **Mobile Development in UE5** - Platform-specific guide
4. **Isometric Camera Tutorial** - YouTube search for setup

---

## Next Steps

After setup is complete:

1. [ ] Create `BP_OdysseyGameMode` Blueprint
2. [ ] Set up isometric camera
3. [ ] Create `BP_PlayerCharacter` with basic movement
4. [ ] Implement virtual joystick
5. [ ] Test on physical device

---

*Last Updated: 2026-02-02*
