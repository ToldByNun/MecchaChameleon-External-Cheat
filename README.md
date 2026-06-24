<div align="center">

<img src="https://capsule-render.vercel.app/api?type=soft&color=0:0d1117,100:21262d&height=200&section=header&text=MecchaChameleon&fontSize=44&fontColor=e6edf3&desc=external%20cheat%20%C2%B7%20UE5%20memory%20research&descSize=15&descAlignY=66"/>

![C++](https://img.shields.io/badge/C++-20-blue)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)
![Status](https://img.shields.io/badge/status-work%20in%20progress-orange)
![Engine](https://img.shields.io/badge/engine-Unreal%20Engine-black)

</div>

---

## Overview

External cheat for **MecchaChameleon** (UE5) — built for **memory research and reverse engineering**.

The tool runs out-of-process: no injection, no hooks. It attaches to the game's shipping executable, scans for `GWorld` / `GNames` via AOB patterns, and walks core Unreal structures (world chain, `GameState` player array, skeletal mesh bones, camera POV) to understand how runtime state is laid out in memory. A transparent DXGI overlay (DirectX 11 + ImGui) renders on top of the game window.

> Work in progress. AOB resolution, pointer chains, background polling, overlay, and menu are in place. Snaplines ESP is wired; box / skeleton / labels and aimbot remain placeholder UI only.

<br/>

## Contents

- [Current scope](#current-scope)
- [Planned features](#planned-features)
- [Architecture](#architecture)
- [Project layout](#project-layout)
- [Requirements](#requirements)
- [Build & run](#build--run)
- [Controls](#controls)
- [Offsets & patterns](#offsets--patterns)
- [Roadmap](#roadmap)
- [Disclaimer](#disclaimer)

---

## Current scope

| Area | Description | Status |
|:-----|:------------|:------:|
| **Process I/O** | External attach via `ReadProcessMemory` | done |
| **AOB scanning** | Runtime pattern scan for `GWorld` and `GNames` globals | done |
| **FName pool** | Decode entries from AOB-resolved `GNames` | done |
| **World chain** | Resolve `UWorld → PersistentLevel`, `GameInstance`, `GameState` | done |
| **Camera chain** | `GameInstance → LocalPlayer → PlayerController → CameraManager` | done |
| **View info** | Live `FMinimalViewInfo` from `PlayerCameraManager` | done |
| **Player tracking** | `GameState → PlayerArray → Pawn → RootComponent` positions | done |
| **Mesh / bones** | Init-time validation of skeletal mesh & bone transform arrays | done |
| **Projection** | `WorldToScreen` (`FMinimalViewInfo → FVector2D`) | done |
| **Background poll** | Mutex-protected actor/camera snapshot on a worker thread | done |
| **Overlay** | Transparent Win32 window, DXGI 11, ImGui render loop | done |
| **Menu** | ImGui tabs (ESP / Aimbot / Misc), `INSERT` toggle | done |
| **Snaplines ESP** | Lines from screen bottom to projected actor positions | WIP |
| **Box / skeleton / labels** | Menu toggles present, rendering not implemented | planned |
| **Aimbot** | Menu toggles present, no aim logic yet | planned |

---

## Planned features

### ESP

| Feature | Description |
|:--------|:------------|
| **Box ESP** | 2D bounding boxes around actors via world-to-screen |
| **Skeleton ESP** | Bone chain overlay for humanoid meshes |
| **Name / distance** | Actor class name and distance from local player |
| **Snaplines** | Lines from screen center or bottom to target |
| **Health / state** | Optional bars or flags when offsets are known |
| **Chinese hat** | RGB hat above players |

### Aimbot *(maybe)*

| Feature | Description |
|:--------|:------------|
| **Target selection** | Closest to crosshair, lowest HP, etc. |
| **Bone aim** | Head / chest / configurable bone index |
| **FOV limit** | Only acquire targets inside a radius |
| **Smoothing** | Interpolated aim delta instead of snap |
| **Visibility check** | Skip actors behind geometry when trace data exists |

> Aimbot is not committed yet — listed as a possible research extension once actor filtering and view matrices are stable.

---

## Architecture

```mermaid
flowchart LR
    A["main.cpp"] --> B["MecchaChameleon core"]
    A --> O["Overlay"]
    A --> M["Menu"]
    A --> E["ESP"]

    B --> C["Memory"]
    B --> D["Helpers"]
    B --> U["Unreal"]

    C -->|"attachToProcess()"| F["PenguinHotel-Win64-Shipping.exe"]
    C -->|"findAob() / resolveAob()"| F
    C -->|"readMemory&lt;T&gt;()"| F

    B -->|"init: AOB scan"| G["GWorld / GNames"]
    G --> H["UWorld chain"]
    H --> I["GameState → PlayerArray"]
    I --> J["Pawn → RootComponent → FVector"]
    I --> K["Mesh → Bone transforms"]

    H --> L["GameInstance → CameraManager"]
    L --> M2["FMinimalViewInfo"]

    B -->|"background thread"| B
    B -->|"getSnapshot()"| A
    U -->|"WorldToScreen()"| E
    O -->|"DX11 + ImGui"| N["Transparent overlay HWND"]
    M --> S["settings::esp / aimbot"]

    style A fill:#161b22,color:#e6edf3,stroke:#30363d
    style B fill:#21262d,color:#e6edf3,stroke:#30363d
    style F fill:#0d1117,color:#8b949e,stroke:#30363d
```

**Init flow:**

1. Attach to `PenguinHotel-Win64-Shipping.exe` and read module base/size.
2. Scan the module image for AOB patterns → resolve `GWorld` and `GNames` globals.
3. Walk and validate the full pointer chain (world, camera, `GameState`, player meshes).

**Pointer chains:**

```
Module scan (AOB)
    ├── GNames  → FName pool (resolveName)
    └── GWorld  → UWorld*
            ├── PersistentLevel
            ├── OwningGameInstance
            │       └── LocalPlayers → PlayerController → PlayerCameraManager
            │               └── FMinimalViewInfo (CameraInfo)
            └── GameState
                    └── PlayerArray (TArray)
                            └── PlayerState → Pawn
                                    ├── Mesh → SkeletalMesh / BoneSpaceTransforms / ComponentSpaceTransforms
                                    └── RootComponent → RelativeLocation
```

**Runtime loop (`main.cpp`):** sync overlay to game window → poll input → read snapshot → ImGui frame → optional ESP draw → present. Settings live in `settings` (`Menu.hpp` / `Menu.cpp`) as a single shared instance (`extern` in header, definition in `Menu.cpp`).

---

## Project layout

```
MecchaChameleon/                          # repo / solution root
├── README.md
├── MecchaChameleon.slnx
├── Research (ignore this)/               # local RE notes, patterns, IDA helpers
└── MecchaChameleon/
    ├── MecchaChameleon.vcxproj
    └── MecchaChameleon/
        ├── main.cpp
        ├── Engine/
        │   ├── offsets.hpp               # struct offsets + AOB patterns
        │   ├── types.hpp
        │   ├── helpers.hpp               # FName resolution
        │   ├── Memory/                   # attach, read, AOB scan
        │   ├── MecchaChameleon/          # core module (init, update, snapshot)
        │   ├── Unreal/                   # WorldToScreen
        │   └── ImGui/                    # vendored Dear ImGui + DX11/Win32 backends
        └── Modules/
            ├── Overlay/                  # transparent DXGI overlay window
            ├── Menu/                     # ImGui menu + shared settings
            └── Esp/                      # ESP draw helpers
```

---

## Requirements

| | |
|:--|:--|
| OS | Windows 10 / 11 (x64) |
| IDE | Visual Studio 2026, MSVC v145 |
| Language | C++20 |
| Target | MecchaChameleon UE5 shipping build (`PenguinHotel-Win64-Shipping.exe`) |

---

## Build & run

```bash
git clone https://github.com/ToldByNun/MecchaChameleon-External-Cheat.git
cd MecchaChameleon-External-Cheat
```

Open `MecchaChameleon/MecchaChameleon.slnx`, set **Release · x64**, then build.

The game must be running before launch. The tool attaches to `PenguinHotel-Win64-Shipping.exe`, scans for `GWorld` / `GNames`, validates the pointer chain, and opens a click-through overlay aligned to the game window.

**Expected console output (init):**

```text
[+] BaseAddress                    : 0x7FF6A0000000
[+] GWorld                         : 0x7FF6A6737A14
[+] GNames                         : 0x7FF6A13AE2F4
[+] PersistentLevel                : 0x...
[+] GameInstance                   : 0x...
[+] LocalPlayers                   : SUCCESS [Count: 1] at 0x...
[+] LocalPlayer                    : 0x...
[+] PlayerController               : 0x...
[+] CameraManager                  : 0x...
[+] ViewInfo                       : SUCCESS X: 1200 Y: 340 Z: 90
[+] GameState                      : 0x...
[+] PlayerArray                    : SUCCESS [Count: 8] at 0x...
mesh=SkeletalMeshComponent class=... skeletal=... bones=65 comp=65
[+] Overlay running. Press INSERT to toggle menu.
```

If an AOB pattern fails to match after a game update, update the patterns in `offsets.hpp` (see `Research (ignore this)/` for notes).

---

## Controls

| Key | Action |
|:----|:-------|
| **INSERT** | Toggle ImGui menu (overlay becomes interactive while open) |

Menu tabs: **ESP** (box, skeleton, name/distance, snaplines), **Aimbot** (enabled, FOV, smoothing — UI only), **Misc**.

---

## Offsets & patterns

Defined in `offsets.hpp`. Struct offsets are version-specific — re-derive after patches. `GWorld` and `GNames` are resolved at runtime via AOB scan instead of hardcoded RVAs.

### AOB patterns (current build)

| Symbol | Pattern | Instruction offset |
|:-------|:--------|:-----------------|
| `GNames` | `80 3D ?? ?? ?? ?? 00 0F 84 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? E9 ?? ?? ?? ??` | `+0xD` |
| `GWorld` | `44 38 2D ?? ?? ?? ?? 48 8B 1D ?? ?? ?? ?? 74 ?? 48 85 DB 74 ?? 48 8B CB E8 ?? ?? ?? ??` | `+0x7` |

`Memory::resolveAob()` locates the pattern in the module image, reads the `rip+rel32` displacement at the instruction offset, and returns the resolved global address.

### Struct offsets

| Symbol | Value | Role |
|:-------|:------|:-----|
| `PersistentLevel` | `+0x30` | `UWorld` → active level |
| `OwningGameInstance` | `+0x228` | `UWorld` → game instance |
| `GameState` | `+0x1B0` | `UWorld` → game state |
| `Actors` | `+0xA0` | Level actor array |
| `RootComponent` | `+0x1B8` | Actor scene root |
| `RelativeLocation` | `+0x140` | Component translation |
| `LocalPlayers` | `+0x38` | `UGameInstance` → local player array |
| `PlayerController` | `+0x30` | `ULocalPlayer` → controller |
| `PlayerCameraManager` | `+0x360` | `APlayerController` → camera manager |
| `CameraInfo` | `+0x1540` | `FMinimalViewInfo` in camera manager |
| `PlayerArray` | `+0x2C0` | `AGameState` → player state array |
| `Pawn` | `+0x320` | `APlayerState` → possessed pawn |
| `Mesh` | `+0x418` | `APawn` → skeletal mesh component |
| `SkeletalMesh` | `+0x578` | `USkeletalMeshComponent` → mesh asset |
| `BoneSpaceTransforms` | `+0x9A8` | Bone transform array (bone space) |
| `ComponentSpaceTransforms` | `+0x9B8` | Bone transform array (component space) |

---

## Roadmap

**Foundation**

- [x] External process attach & typed memory reads
- [x] AOB pattern scan for `GWorld` / `GNames`
- [x] `GNames` / FName decoding
- [x] `UWorld` pointer chain resolution
- [x] `GameState` player array parsing
- [x] Skeletal mesh & bone transform validation
- [x] Root-component transform reads
- [x] World-to-screen projection math
- [x] Live `FMinimalViewInfo` extraction
- [x] Background update thread + thread-safe snapshot
- [x] Overlay render loop (DirectX 11 / ImGui)
- [x] ImGui menu & shared settings

**ESP**

- [ ] Box ESP
- [ ] Skeleton ESP
- [ ] Name / distance labels
- [x] Snaplines (first pass — toggle in menu)
- [ ] Chinese hat

**Aimbot** *(TBD)*

- [ ] Target selection & FOV filter
- [ ] Bone-based aim
- [ ] Smoothing

---

## Disclaimer

Educational and research use only — reverse engineering and external memory layout analysis.

Do not use against live multiplayer services or in violation of any terms of service. The author accepts no liability for misuse.

---

<div align="center">

<img src="https://capsule-render.vercel.app/api?type=soft&color=0:0d1117,100:21262d&height=60&section=footer&text=&fontSize=1"/>

<sub><a href="https://github.com/ToldByNun">ToldByNun</a></sub>

</div>
