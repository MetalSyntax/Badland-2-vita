# Badland 2 — PS Vita Port

<p align="center">
  <img src="extras/livearea/icon0.png" alt="Badland 2 Vita Icon" width="128" height="128">
</p>

An enhanced, native port of **Badland 2** (Android) for the **PlayStation Vita** and **PlayStation TV**, powered by an ARMv7 dynamic shared library loader (`so-loader`), custom `FalsoJNI` bridge, and `vitaGL`.

---

## Features

- **Native Resolution & 60 FPS:** Runs at full Vita native resolution (960x544) at smooth 60 frames per second.
- **Physical Controls & Front Multi-Touch:** Full integration with PS Vita hardware controls:
  - Dual-flap physics controls mapped to physical **L / R Triggers**, **Left Analog Stick**, and **Face Buttons**.
  - Full front touchscreen support with dynamic touch tracking.
- **Complete Audio Support:** FMOD Studio's AudioTrack output is bridged to native `SceAudioOut` at 44.1 kHz (the VitaSDK OpenSL ES backend is avoided because it deadlocks).
- **Persistent Shader Cache:** Utilizes vitaGL runtime shader binary caching (`ux0:data/badland2/cache/`) to eliminate shader compilation stutter during gameplay.
- **Offline Soloader Patches:**
  - Bypasses nonexistent Facebook and Cheetah Mobile online servers.
  - Social friend requests and online challenge leaderboards safely stubbed to prevent NULL-pointer crashes when pausing or entering menus.
  - In-app purchase verification stubs ensuring offline playability.
- **Thread-Safe Input & Robust Memory Architecture:** Uses `SceLibcBridge` and POSIX mutexes for crash-resilient multi-threaded input event dispatching.

---

## Controls

| Button / Input | Action |
| :--- | :--- |
| **L Trigger** | Flap Left (fly towards left) |
| **R Trigger** | Flap Right (fly towards right) |
| **Cross ($\times$)** | Dual Flap (ascend / forward) / Menu Confirm |
| **Left Stick** | Directional Flap (Tilt Left/Right to flap in that direction) |
| **D-Pad Left / Right** | Flap Left / Flap Right |
| **Start** | Pause Game / Resume |
| **Touchscreen** | Native Touch Controls (Tap left/right side of screen to flap), up to 5 fingers |

> **Note:** Physical flap buttons (L / R / D-Pad / Stick / Cross) are still weak in gameplay — the touchscreen is the reliable control for now. Circle is intentionally unmapped.

---

## Prerequisites

Make sure your PS Vita is running Henkaku / Enso on firmware 3.60 or 3.65 and has the following plugins installed:

1. **[kubridge.skprx](https://github.com/TheOfficialFloW/kubridge/releases)** (v0.3.1 or higher)
2. **[fd_fix.skprx](https://github.com/TheOfficialFloW/FdFix/releases)** (or repatch / rePatch reLoaded)
3. **libshacccg.suprx** runtime shader compiler (can be easily extracted using **[ShaRKBR33D](https://github.com/Rinnegatamante/ShaRKBR33D)**).

---

## Setup & Installation

1. Install the latest **`badland2.vpk`** on your PS Vita using VitaShell.
2. Obtain a legally purchased copy of **Badland 2** (v1.0.0.1062 or compatible ARMv7 build) in `.apk` format.
3. Open the `.apk` file using an archive manager (such as 7-Zip, WinRAR, or unzip).
4. Extract the following native libraries from `lib/armeabi-v7a/`:
   - `libbadland2.so`
   - `libfmod.so`
   - `libfmodstudio.so`
5. Extract the game asset files:
   - Extract the entire contents of the `assets/` directory from inside the APK.
6. Copy the extracted files to your PS Vita storage under `ux0:data/badland2/`:
   ```
   ux0:data/badland2/
   ├── libbadland2.so
   ├── libfmod.so
   ├── libfmodstudio.so
   └── (assets extracted from the APK: audio/, config/, level/, scene/, textures/, etc.)
   ```
   > Keep the original filename `libbadland2.so`. v1.0 only found the game if it was renamed to `main.so`; since v1.1 both names work.
7. Launch **Badland 2** from your LiveArea!

---

## Building from Source

To compile the port using the official **[vitaSDK](https://vitasdk.org/)**:

```bash
git clone https://github.com/your-username/Badland-2-vita.git
cd Badland-2-vita

export VITASDK=/path/to/vitasdk
export PATH=$VITASDK/bin:$PATH

cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

The resulting `badland2.vpk` and `eboot.bin` will be located in the `build/` directory.

---

## Credits & Acknowledgements

- **Frogmind** for developing the award-winning *Badland* series.
- **TheFloW** for the original `so-loader` concept, `kubridge`, and `fd_fix`.
- **Rinnegatamante** for [vitaGL](https://github.com/Rinnegatamante/vitaGL) and continuous support across the PS Vita homebrew ecosystem.
- **Volodymyr Atamanenko** for the soloader boilerplate architecture and foundational tooling.
