# Odyssey Unity — Thin Slice Setup

## Prerequisites
- Unity 2022.3 LTS or later (URP template)
- Node.js 18+

## Step 1: Create Unity Project

1. Open **Unity Hub** → New Project
2. Template: **3D (URP)**
3. Project name: `OdysseyUnity`
4. Location: `~/dev/Odyssey/`
5. Create

## Step 2: Copy Scripts

Copy the `Assets/Scripts/` folder from this directory into your Unity project's `Assets/` folder.

Or if you created the project at this exact path, the scripts are already in place.

## Step 3: Install Packages

Open `Packages/manifest.json` and add these to the `dependencies` block:

```json
"com.endel.nativewebsocket": "https://github.com/endel/NativeWebSocket.git#upm",
```

For MessagePack-CSharp, install via NuGet for Unity:
1. Download the latest `MessagePack.Unity.unitypackage` from https://github.com/MessagePack-CSharp/MessagePack-CSharp/releases
2. Import it into the project (Assets → Import Package → Custom Package)

## Step 4: Scene Setup

In your main scene (`MainScene`):

1. **Create empty GameObject** → name it `[Managers]`
   - Add component: `NetworkManager` (Odyssey.Network)
   - Add component: `GameManager` (Odyssey.Core)
   - Add component: `PlayerManager` (Odyssey.Player)
   - Add component: `PlayerInputHandler` (Odyssey.Player)

2. **Main Camera** (already in scene)
   - Add component: `IsometricCamera` (Odyssey.Camera)
   - Set Camera → Projection to **Orthographic**
   - Drag the `[Managers]` → GameManager → `Isometric Camera` reference to this camera
   - Set Orthographic Size to `10`

3. **Wire references in GameManager:**
   - `Isometric Camera` → drag Main Camera
   - `Player Manager` → drag [Managers]
   - `Input Handler` → drag [Managers]
   - `Server Url` → `ws://localhost:8765`

4. **Create a ground plane** for visual reference:
   - GameObject → 3D Object → Plane
   - Scale: (100, 1, 100)
   - Material: any dark color

5. **Save scene** as `Assets/Scenes/MainScene.unity`

## Step 5: Start Server

```bash
cd ~/dev/Odyssey/Server
npm run dev
```

You should see:
```
[Server] Listening on ws://localhost:8765
[Server] Tick rate: 20 Hz (50ms)
```

## Step 6: Play

Press Play in Unity Editor. You should see:
- `[Net] Connecting to ws://localhost:8765...`
- `[Net] Connected`
- `[Net] Welcome! id=1, spawn=(X, Y)`
- `[GameManager] Local ship spawned at (X, Y)`

Use **WASD/Arrow keys** to move the ship. The ship moves locally (prediction) and the server confirms the position.

## Step 7: Test Multiplayer

1. **Build & Run** a standalone player (File → Build & Run)
2. Press **Play** in the Editor
3. Both instances connect to the same server
4. Move one player — the other should see a cyan cube (remote proxy) moving smoothly

## Debug Visualization

In the Scene view (not Game view), enable **Gizmos**. You'll see:
- **Red wireframe sphere** = server-authoritative position
- **Green wireframe sphere** = client-predicted position

The gap between them shows prediction error. They should stay very close on localhost.

## Troubleshooting

**"NetworkManager not found"**
→ Make sure NetworkManager component is on a GameObject in the scene

**Connection refused**
→ Make sure the server is running (`npm run dev` in Server/)

**No ship spawns**
→ Check Console for errors. Common: missing MessagePack package

**Ship moves but no remote players visible**
→ Check both instances have different player IDs in the console logs
