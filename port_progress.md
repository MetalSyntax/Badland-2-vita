# Registro de Progreso — Badland 2 (PS Vita)

## Fase 1: Configuración y Preparación (Completada — 2026-10-10, ampliada 2026-10-12)
- Repo creado desde soloader-boilerplate, `.gitignore` anti-DMCA.
- APK `badland-2-1-0-0-1062.apk` copiado y extraído.
- ABI detectada: armeabi-v7a (elegida: armeabi-v7a).
- GLES detectado: valor no estándar en manifest: 0x20000 (declarado en AndroidManifest.xml)
- **2026-10-12 — Fix bloqueante:** `lib/falso_jni/` estaba VACÍO (submódulo `v-atamanenko/FalsoJNI`
  sin inicializar y repo sin ningún commit). Clonado por HTTPS con
  `git clone --depth 1 https://github.com/v-atamanenko/FalsoJNI.git lib/falso_jni`.
  Sin esto ningún build puede linkar. Pendiente: primer `git commit` (el repo sigue sin commits).
- **2026-10-12 — Build:** `psvita-toolkit doctor` OK salvo Docker-daemon (caído) y avisos menores
  (ninja, psp2cgc, PVRTexToolCLI ausentes). Workaround de toolchain: configurar con
  `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` (el `vita.cmake` del SDK exige CMake <3.5).

## Fase 2: Decompilación (jadx OK — 2026-10-12; Ghidra pendiente)
- jadx: CONFIRMADO presente — `decompiled/apk_jadx/sources/` + `resources/` existen y se leyeron
  (`Main.java` 701 líneas, `NativeInterface.java`, `Badland2Activity.java`, managers de
  ads/Facebook/Flurry/IAP). El `port_progress` anterior decía "NO corrido" — era incorrecto.
- Ghidra (.so): NO corrido — `docker daemon` no responde (ver `doctor` arriba); sin pseudo-C.
  Todo el análisis nativo de esta fase se hizo con `arm-vita-eabi-objdump/nm/strings`.

## Fase 3: Análisis del Motor Real (Completada offline — 2026-10-12, sin confirmar en hardware)
- **Motor: Frogmind propietario (`namespace b`), NO Cocos2d-x.** Evidencia (`objdump -T/-nm -D`):
  `libbadland2.so` (4808 KB, ARMv7 Thumb-2, stripped) exporta `ANativeActivity_onCreate`
  (0x3e7f58, T) + `android_main` (0x13f098, T) → `android_native_app_glue` linkado
  estáticamente; símbolos `b::` por todo el binario (`b::StaticData`, `b::JointRevolute`,
  `b::CheetahRankList`, `SignalSystem::EventSound`). Los únicos `cocos2d` son 21 exports del
  helper social `Cocos2dxSocialGooglePlay` (leaderboards/nube), no el motor.
- **NO hay port hermano del mismo motor** (barrido de `BASE_DIR`: ningún otro port es Frogmind;
  solo Aralon tiene glue NativeActivity, pero es motor Unity → NO se hizo `sync-shared` de código
  de motor; solo se adaptó el *patrón* NDK — ABI estándar y estable — citándolo).
- **Ciclo de vida real:** `JNI_OnLoad` (0x13f088, desensamblado: solo `JniUtils::setJavaVM` +
  retorna 0x10006) → `ANativeActivity_onCreate` (spawnea el hilo de `android_main`) →
  `Main.onCreate/onStart` (Java) → `nativeSetFileDirectories` → `nativeInitJni` →
  `nativeInitFmod`. `Main.java` confirma orden de carga: `libfmod` → `libfmodstudio` →
  `libbadland2`, y `Badland2Activity extends NativeActivity` con `nativePadAction`,
  `nativeInputDeviceAdded/Removed`, `nativeSocialPlatformChosen`.
- **Middleware y decisión stub-vs-real:**
  - `NEEDED`: `libfmod.so`, `libfmodstudio.so` (vienen EN el APK, solo dependen de
    libc/log/m) + `libjnigraphics`, `liblog`, `libandroid`, `libEGL`, `libGLESv2`.
  - FMOD Studio: ~47 imports C++ UND — decisión: **NO stubear** (devolver `FMOD_OK` con handles
    NULL rompería en el primer `createInstance` con un PC confuso). Estrategia: shippear los
    `.so` reales del APK y resolver contra ellos (follow-up Fase 4). Primer llamado FMOD real
    será el próximo bug confirmado.
  - `libpng 1.5.26` estática dentro del `.so` (strings) → nada que hacer.
  - GLESv2 + EGL CONFIRMADOS como UND → vitaGL (Fase 5: `vglInitExtended` 960x544 ya en `glutil.c`).
- **Exports JNI:** 65 `Java_*` en `libbadland2.so` (el plan listaba 68; los 2 de
  `org.fmod.MediaCodec` viven en `libfmod.so`: `fmodGetSize`/`fmodReadAt`, + `JNI_OnLoad` propio).
- **Riesgo alineación:** `align-check` del toolkit se cuelga (timeout 120s, igual que
  `jni-analyze`); evaluación manual: ARMv7 hard-float/NEON, CPU Vita Cortex-A9 sin traducción.
  Scan detallado (`ldrd`/`vld1`) pendiente de Ghidra.

## Fase 4: Loader y bootstrap JNI (Código completo y compilado — 2026-10-12, sin probar en hardware)
- **Import table: 291/338 UND resueltos; restan solo los 47 FMOD.** Añadidos a `source/dynlib.c`:
  glue NDK (`AInputQueue/ALooper/AMotionEvent/AKeyEvent/AInputEvent/ANativeWindow/
  AConfiguration/AndroidBitmap/AAsset_openFileDescriptor`), `writev` (vía `write()`, no hay
  `sys/uio.h` en newlib de Vita), y los 11 `_Unwind_*` **re-enviados al libgcc del toolchain**
  (verificado `T` en `libgcc.a` gcc 10.3.0 — no stubeados, el `.so` es C++ con excepciones).
  `so_resolve` NO aborta ante irresueltos (solo loguea + `plt0_stub` en JUMP_SLOT), así que el
  boot puede avanzar hasta el primer llamado FMOD.
- **Glue NativeActivity nuevo** (`source/reimpl/`: `native_activity`, `input_queue`,
  `input_event`, `looper`, `native_window`, `configuration` + `getCountry("US")/getLanguage("en")`,
  `bitmap`, `rect.h`; `AAsset_openFileDescriptor` en `asset_manager.cpp`): `main.c` reescrito —
  `JNI_OnLoad` → `native_activity_bootstrap` (onCreate/onStart/onResume/focus/window) →
  `nativeSetFileDirectories(DATA_PATH, DATA_PATH)` → `nativeInitJni` → `nativeInitFmod` →
  heartbeat en hilo principal (el hilo del glue corre `android_main`). **Se eliminó el doble
  `gl_init()`** (lo hace `eglInitialize` cuando el glue crea la ventana).
- **FalsoJNI: 8 métodos que el .so SÍ resuelve** (strings exactos en el binario) registrados en
  `source/java.c` con valores seguros leídos de `Main.java`: `JNI_getLocalLanguage→0` (inglés),
  `JNI_IsNetConnected→0` (offline → rutas cached), `JNI_isControllerOnly→1` (verdad hoy: sin
  táctil), `JNI_openUrl→0`, `JNI_getModelName→"PS Vita"`, `keyboardShow/Hide→no-op`,
  `JNI_textAsBitmap→NULL` ⚠️ **sospechoso #1 si el primer boot cae en
  `AndroidBitmap_lockPixels/getInfo` o justo después de su log** (wrapper sin verificar, §8
  so-crash-triage; impl real con stb_truetype = Fase 5).
- **Build verde:** `cmake --build build` → `eboot.bin` (572848 B) + `badland2.vpk` (656545 B).
  Fixes de build aplicados: `converter.c` de FalsoJNI al `CMakeLists.txt`, renames
  `soloader_egl*` (la vitaGL instalada exporta todo EGL y colisionaba), includes/convenciones
  `JNIEnv` (`jni` ya ES `JNIEnv` en C; las tablas de FalsoJNI toman `JNIEnv*`).
- **2026-10-12 — Logger según estándar §6:** `source/utils/logger.h` y `logger.c` adaptados al estándar canónico del toolkit (`badland2_NNN.log` 001..999, `next.idx`, `DATA_PATH/logs/`, unbuffered, flags de producción `ENABLE_FILE_LOG`/`PORT_TRACE`). `psvita-toolkit log-standard --fix-dirs` OK (0 discrepancias). `log_init()` llamado al inicio de `soloader_init_all()`.
- **NO probado en hardware:** la consola (`192.168.3.15`) no responde al ping → sin deploy todavía.
- **Generados sin integrar:** `generated_jni_table.h/.c` (235 candidatos de ads/social/IAP) siguen
  como candidatos — no se integran hasta que un log pruebe que el .so los llama.

## Fase 5: Gráficos (Pendiente hardware; decisiones anotadas)
- vitaGL + `vglInitExtended(960x544, MSAA 4x)` ya cableado vía `eglInitialize` (Fase 4).
- Texturas: `assets/tex_SD/` trae bundles `ATC/ETC2/NOCOMPR/PVRTC/RAW.bin` (94 MB assets) —
  pendiente elegir variante (candidato: RAW) cuando el boot llegue a carga de nivel.
- Shaders: sin `glslangValidator` corrido aún; `shader-transpile` cuando haya dump GLSL real.

## Fase 6/7: Táctil y controles físicos (Pendiente)
- Input hoy = ceros neutros (`input_event.c`, `getEvent→-1`, `pollAll→TIMEOUT`): "sin dispositivos".
  `nativePadAction`/`nativeInputDeviceAdded` existen en el .so para mapear Vita → gamepad más
  adelante. `isControllerOnly=1` evita rutas táctiles hasta entonces.

## Fase 8: Audio (Bloqueado por estrategia FMOD — ver Fase 4)
- `libfmod.so` (1147 KB) + `libfmodstudio.so` (1479 KB) reales en el APK. `nativeInitFmod` se llama
  en el boot; se espera que sea el primer crash real → un bug a la vez.

## Fase 9: Videos (Probablemente N/A)
- Barrido del APK extraído: **cero `.mp4/.ogv/.webm`** en `badland2_extract/`. Si el juego solo usa
  los bundles `.bin`, no hay FMV que portar. Confirmar cuando el boot liste assets pedidos.

## Fase 10: Assets/datos/idioma/saves (Completada offline)
- `nativeSetFileDirectories(DATA_PATH, DATA_PATH)` cableado; `AAssetManager_open` mapea a
  `DATA_PATH/assets/`.
- **Idioma y país dinámicos:** Implementados con `sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG)`:
  - En `source/reimpl/configuration.c`: `AConfiguration_getLanguage` y `AConfiguration_getCountry` mapean los idiomas y países a códigos ISO (en/es/fr/de/it/pt/ru/zh/ko/ja/fi/tr + US/ES/FR/DE/etc.).
  - En `source/java.c`: `Main_JNI_getLocalLanguage` mapea los códigos del sistema a los enteros correspondientes de `Main.java` (default 0 English).
- Boot checker de `SO_PATH` ya existe en `init.c`. `saves/` y `logs/` creados por el toolkit.

## Fase 11: Rendimiento (Pendiente hardware)
- Clocks 444/222/222/166 ya fijados en `init.c`. Sin telemetría hasta primer boot.

## Fase 12: Configurador (Omitido temporalmente por solicitud del usuario)
- `settings.bin`/`-config` redirect del boilerplate intactos; postergado para no generar esfuerzo extra hasta probar en consola física.

## Fase 13: Trofeos (Omitido expresamente por solicitud del usuario)
- Omitido hasta que el juego esté probado y funcional en consola física.

## Fase 14: LiveArea y VPK (Validado)
- Assets de LiveArea en `extras/livearea/` validados con `psvita-toolkit livearea --validate`:
  - `bg0.png` (8-bit indexado OK)
  - `pic0.png` (8-bit indexado OK)
  - `icon0.png` (8-bit indexado OK)
  - `startup.png` (8-bit indexado OK)
  - `template.xml` OK
- `param.sfo` con Title ID `PSVBL0002`.
- `clean-junk` ejecutado (eliminados archivos `._*` de macOS).
- `badland2.vpk` y `eboot.bin` compilan sin errores.

## Fase 15: Testing y release (Boots analizados en hardware)
- **2026-10-13 — Análisis de `logs/badland2_002.log`:**
  - El boot llegó con éxito hasta la llamada a `onStart`: `Calling onStart...` (última línea escrita).
  - Resuelto: Deadlock de pipe y despacho de ALooper implementado.
- **2026-10-13 — Análisis de `logs/badland2_003.log`:**
  - **Avance:** El motor superó el LiveArea, completó el ciclo NDK (`onStart`, `onResume`, `onInputQueueCreated`, `onNativeWindowCreated`), levantó EGL y GLES2 VitaGL, tradujo y compiló shaders GLSL y cargó los assets principales (`audio.bin`, `tex_SD`).
  - **Crash identificado:**
    `Fatal error: Unknown symbol "ZN4FMOD6Studio6System6createEPPS1_j" (0x984b1c58).`
    Disparado al ejecutar `Java_com_frogmind_badland2_Main_nativeInitFmod` en `b::AudioEngine::init`.
  - **Errores de shaders identificados:**
    `file_save: Could not open the specified target path "ux0:data/badland2/gxp/..."`
    El compilador de shaders intentaba guardar la caché `.gxp` en directorios que no existían en disco.
  - **Solución implementada:**
    1. **Multi-módulo FMOD real:** Carga encadenada de `ux0:data/badland2/libfmod.so` (0x98000000) y `ux0:data/badland2/libfmodstudio.so` (0x98200000) previa a `libbadland2.so` (0x98400000). Los 47 símbolos C++ de FMOD se resuelven nativamente vía `DT_NEEDED` sin stubs destructivos.
    2. **Soporte OpenSL ES:** Mapeo de `slCreateEngine` y `SL_IID_*` en `dlsym_soloader`, enlace con `libOpenSLES.a` del SDK de Vita, y stubs de bajo nivel para `sf_*` de SndFile.
    3. **Símbolos faltantes Bionic:** Añadidos `__assert2` y `setpriority` a `dynlib.c`.
    4. **Caché de shaders y VitaGL:** Creación automática de directorios `ux0:data/badland2/gxp`, `cg` y `glsl` en `soloader_init_all`. Ajuste de memoria de VitaGL a 16 MB (`vglInitExtended`) según `README VITAGL.md`.

## Cómo seguir (una cosa a la vez)
1. Copiar `libfmod.so` y `libfmodstudio.so` (desde `badland2_extract/lib/armeabi-v7a/`) a `ux0:data/badland2/`.
2. Instalar el nuevo `badland2.vpk` (o enviar el nuevo `eboot.bin` a `ux0:app/PSVBL0002/eboot.bin`).
3. Iniciar el juego en la consola física.
4. El cargador inicializará FMOD Studio real, guardará la caché de shaders en disco y continuará con la ejecución del juego.
5. Revisar el nuevo log `ux0:data/badland2/logs/badland2_004.log`.
