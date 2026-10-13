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
- **NO probado en hardware:** la consola (`192.168.3.15`) no responde al ping → sin deploy, sin
  log real, sin `analyze`. Primer boot pendiente. **Logger NO cumple §6 del plan**
  (`logger.c` solo imprime por consola, no escribe `logs/badland2_NNN.log`) — TODO antes del
  primer deploy con debug real.
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

## Fase 10: Assets/datos/idioma/saves (Parcial)
- `nativeSetFileDirectories(DATA_PATH, DATA_PATH)` cableado; `AAssetManager_open` mapea a
  `DATA_PATH/assets/`. Idioma: `"en"` hardcodeado en glue + `JNI_getLocalLanguage→0`; TODO leer
  `sceAppUtilSystemParamGetById` con fallback inglés. Boot checker de `SO_PATH` ya existe en
  `init.c`. `datafiles.zip`/saves/quick-save pendientes de boot real.

## Fase 11: Rendimiento (Pendiente hardware)
- Clocks 444/222/222/166 ya fijados en `init.c`. Sin telemetría hasta primer boot.

## Fase 12: Configurador (Pendiente)
- `settings.bin`/`-config` redirect del boilerplate intactos; sin opciones reales todavía.

## Fase 13: Trofeos (Pendiente)
- Sin lista, sin hook a eventos. Después del primer nivel jugable.

## Fase 14: LiveArea y VPK (Boilerplate; sin validar)
- `extras/livearea/` trae los placeholders del boilerplate + `template.xml`; `param.sfo`
  `PSVBL0002`. El VPK arma bien pero **no instalado en consola** (inaccesible) → `0x8010113D`
  sin verificar; correr `livearea --validate` + revisar `icon0/bg/startup` 8-bit antes del release.

## Fase 15: Testing y release (Pendiente)
- Bloqueado por hardware. Siguiente paso físico: `deploy --eboot`, leer `logs/`, y ante el primer
  crash aplicar skill `so-crash-triage` (con Ghidra si Docker revive).

## Cómo seguir (una cosa a la vez)
1. Revivir Docker Desktop → Ghidra pseudo-C de `libbadland2.so` (verificar wrapper de
   `JNI_textAsBitmap` y direcciones FMOD-init).
2. `psvita-toolkit log-standard --fix-dirs` + file-logger (§6) ANTES del primer deploy.
3. Primer `git commit` (hay trabajo sin commitear desde Fase 1).
4. Deploy a consola real → log → triage (esperado: FMOD o textAsBitmap).
