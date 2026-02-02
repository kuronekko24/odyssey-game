# Odyssey - Setup Guide

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

---

## Step 1: Install Unreal Engine

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
