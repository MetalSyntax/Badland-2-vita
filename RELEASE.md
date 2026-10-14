# Badland 2 — PS Vita Port v1.0

First official playable release of **Badland 2** for the PlayStation Vita and PlayStation TV!

---

## Highlights & What's New in v1.0

### In-Game Milestone & 60 FPS Performance
- Badland 2 runs natively at 960x544 resolution at 60 FPS.
- Full physics simulation, particle effects, lighting, and ambient world shaders fully rendered with custom vitaGL integration.
- Native persistent shader cache enabled (`HAVE_SHADER_CACHE=1`) under `ux0:data/badland2/cache/`, preventing stutter during level transitions.

### Comprehensive Dual-Flap Controls
- Badland 2's signature two-finger / directional flapping is fully translated to PS Vita physical controls:
  - **L Trigger / D-Pad Left / Left Stick Left:** Flap towards the left.
  - **R Trigger / D-Pad Right / Left Stick Right:** Flap towards the right.
  - **Cross ($\times$) / Up:** Dual-flap (ascend straight up).
  - **Circle ($\bigcirc$):** Cancel / Back in menus.
  - **Start:** Pause game.
- Full native multi-touch touchscreen input with pointer tracking; touching the screen immediately takes priority.

### Full Audio System Fixed (FMOD Studio OpenSL ES)
- Resolved **FMOD Error 51** (`FMOD_ERR_OUTPUT_INIT`):
  - FMOD calls JNI `org.fmod.FMOD.getOutputSampleRate()` and `getOutputBlockSize()` to configure its output device.
  - Implemented these callbacks in `source/java.c` returning `44100` Hz and `512` frames per buffer, matching the VitaSDK OpenSL ES backend.
  - All music tracks, ambient atmospheric sounds, and sound effects now play clearly.

### Stability & Crash Triage Patches
- **Fixed Pause Crash (`badland2_013.log` / `0x985d0cc8`):**
  - When pausing, `b::StateGamePause::requestFacebookFriendsInfo` dereferenced a NULL global online manager pointer due to missing offline checks.
  - Root cause resolved: missing `JNI_IsNetConnected` and `JNI_getLoginState` in FalsoJNI previously defaulted to `1` (true), falsely signaling active internet and Facebook login.
  - Hooked `b::IAP::isNetConnected()` and `IAPAndroid::IsNetConnected()` to return `0` (offline).
  - Hooked `b::StateGamePause::requestFacebookFriendsInfo`, `requestChallengeRankInfo`, and `requestRushFriendsInfo` to safe no-ops.
  - Added complete `FacebookManager` JNI method bridges in `source/java.c` (`JNI_getUserId`, `JNI_getUserTokenString`, `JNI_getLoginState`, etc.).

---

## Installation Guide

1. Install `badland2.vpk` on your PS Vita using VitaShell.
2. Ensure you have `kubridge.skprx`, `fd_fix.skprx`, and `libshacccg.suprx` installed on your system.
3. Extract `libbadland2.so`, `libfmod.so`, and `libfmodstudio.so` from your Android `.apk` (`lib/armeabi-v7a/`).
4. Extract all asset folders from `assets/` in the APK.
5. Copy everything to `ux0:data/badland2/`:
   ```
   ux0:data/badland2/
   ├── libbadland2.so
   ├── libfmod.so
   ├── libfmodstudio.so
   ├── audio/
   ├── config/
   ├── level/
   ├── scene/
   └── textures/
   ```
6. Launch **Badland 2** from LiveArea and enjoy!

---

## Known Notes

- First boot will take a few extra seconds while the game compiles and caches its runtime shaders. Subsequent launches are much faster.
- Online leaderboards and cloud saves are unavailable due to server retirement.
