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
    4. **Caché de shaders y VitaGL:** Creación automática de directorios `ux0:data/badland2/gxp`, `cg` y `glsl` en `soloader_init_all`. Llamada a `file_mkpath()` dentro de `glCompileShader_soloader` previo a guardar el binario compilado. Ajuste de memoria de VitaGL a 16 MB (`vglInitExtended`) según `README VITAGL.md`.
    5. **Stubs de Login y Android ID JNI:** Mapeados `JNI_getAndroidID` (`LoginHandler`), `JNI_isLoggedIn` y `JNI_startupLogin` (`FacebookManager`) en `source/java.c` para evitar fallos de resolución de IDs de métodos.

- **2026-10-13 — Análisis de `logs/badland2_004.log` + `badland2-psp2core-1791921147-...psp2dmp`:**
  - **Log vacío** (solo cabecera): el proceso murió sin `sceIoClose` y exFAT no comprometió el tamaño del archivo. Fix: `sceIoSyncByFd` tras cada línea en `logger.c`.
  - **Ojo con `psvita-toolkit analyze`:** auto-detectó la base de `libbadland2.so` en 0x97e6b000 (incorrecto) y simbolizó el PC como `ComponentBot::load`. Con 3 módulos la base real es libfmod=0x98000000 → el PC 0x980cadfc es **libfmod.so+0xcadfc** (código ARM, no Thumb).
  - **Crash:** Data abort en función interna de libfmod (llamada desde `SystemI` vía selección de output) que hace `(*gGlobal->vm)->GetEnv(vm, &env, 0x10006)` con `vm == NULL` (R0=0). El `JavaVM*` en `[gGlobal]+0x68` lo escribe **el `JNI_OnLoad` de libfmod**, que el loader nunca llamaba (solo el de libbadland2).
  - **Fix:**
    1. `main.c` llama `JNI_OnLoad` de libfmod antes que el de libbadland2 (orden de `Main.java`: fmod → fmodstudio → badland2; fmodstudio no tiene `JNI_OnLoad`).
    2. Esa misma función luego llama `org.fmod.FMOD.checkInit()Z` y `supportsLowLatency()Z`: ambas `true` ⇒ output 16 (OpenSL, respaldado por libOpenSLES del SDK); si no, 15 (AudioTrack vía Java, inexistente). Registradas en `java.c` devolviendo `true`, y `getOutputSampleRate`/`getOutputBlockSize` devolviendo `0` (lo que devuelve el Java real sin la propiedad; el `-1` por defecto de FalsoJNI no es válido).

- **2026-10-13 — Análisis de `logs/badland2_005.log` + `badland2-psp2core-1791923619-...psp2dmp`:**
  - **Avance:** FMOD inicializa (`libfmod JNI_OnLoad returned 0x10006`), GL/VitaGL arriba, carga de `data.bin`/`tex_SD`/fuente; corte en `Init Shaders`.
  - **Crash:** thread `pthread`, PC en `SceLibKernel` (memcpy), R0=0, stack con `unserialize_shader` (vitaGL) ← `load_shader` (`glutil.c`). `load_shader` ignoraba el retorno de `file_load`: un `.gxp` cacheado vacío/truncado (p.ej. quedó en 0 bytes al morir el proceso a mitad de `file_save`, mismo efecto exFAT que el log 004) → `glShaderBinary` con buffer/size basura → `vglMalloc` NULL → memcpy a 0.
  - **Fix (`glutil.c`):** si el `.gxp` no carga o mide ≤ 8 bytes se borra y se recompila desde el GLSL. Al guardar: solo si `GL_COMPILE_STATUS` OK y `len > 0`, vía `.tmp` + `sceIoRename` (escritura atómica).
  - **Extra (`dynlib.c`):** `dlsym_soloader` ahora cae a la tabla `default_dynlib` (resuelve `AAssetManager_open` que libfmod pedía). `getauxval` sigue sin resolverse (FMOD cae a `/proc/self/auxv`, no fatal).

- **2026-10-13 — Análisis de `logs/badland2_006.log` (sin .psp2dmp → cuelgue, se queda en la imagen de LiveArea):**
  - El fix de shaders funcionó (2 `.gxp` corruptos detectados y recompilados). El log se corta después del 2º shader.
  - `Calling nativeInitFmod...` nunca devuelve (tampoco en 005) y el heartbeat del hilo main nunca aparece → main bloqueado en `b::AudioEngine::init` (Studio::System::create → setSoftwareFormat → setDSPBufferSize → **Studio::System::initialize**, sospechoso: espera al hilo mixer/OpenSL). Probable que el render thread se bloquee luego contra el mismo lock de FMOD.
  - Descartado: `syscall` (no logueó), wrappers `sem_*`/`pthread_*` parecen correctos para lo que importa FMOD.
  - **Herramienta agregada:** watchdog en `main.c` — si una etapa de init dura > 20 s, loguea `watchdog: stuck in <etapa>` y fuerza un data abort para generar `.psp2dmp` con PC/LR de todos los hilos.

- **2026-10-13 — Análisis de `logs/badland2_008.log` + `badland2-psp2core-1791940812-...psp2dmp`:**
  - **Avance mayúsculo:** ¡El juego superó por completo la inicialización de FMOD, VitaGL, compilación de shaders (~110 s de shader cache), guardado de archivos de usuario (`user.bin`, `netcache.bin`), `StaticData` y la inicialización de `SocialManager`/`SocialCache`!
  - **Ubicación real del crash:**
    `PC = 0x985f1fa2` -> `libbadland2.so + 0x1f1fa2` (`b::StateMenuMain::init() + 0x7a`).
    Instrucción: `ldr r0, [r0, #96]` con `R0 = 0x00000000` (Data Abort por desreferencia de puntero nulo).
  - **Causa raíz identificada (cruzando .so, MurmurHash2 y logs):**
    1. En `b::StateMenuMain::init()`, el motor llama a `f::UINodeLoader::loadNode("/ui/menu/main.txt", parent, 1)` y luego busca el nodo `"PROMOBANNER"` con `searchChildByName`. Al no encontrarlo (`NULL`), intenta leer su campo `[node + 96]` sin verificar y crashea.
    2. ¿Por qué no existía `"PROMOBANNER"`? Porque `/ui/menu/main.txt` no cargó.
    3. ¿Por qué no cargó `/ui/menu/main.txt`? Los assets del juego (menús, mundos como `/world/World.json`) están empaquetados dentro de `data.bin` (verificado con su función hash MurmurHash2 con seed `0x0ab1cdb3`).
    4. Al arrancar `StaticData::init()`, `f::DataPack::openPack("data.bin", 0)` intenta leer la cabecera (4 bytes para el número de archivos y luego `N * 24` bytes para la tabla de contenidos) mediante `AAsset_read()`.
    5. **Bug crítico en `source/reimpl/asset_manager.cpp`:** En `AAssetManager_open()`, tras calcular el `fileSize` con `fseek/ftell`, ¡había una llamada errónea a `fclose(a->f)`! Por tanto, `a->f` quedaba CERRADO inmediatamente tras abrirse. Cualquier llamada subsiguiente a `AAsset_read()` (`data.bin`, `tex_SD/RAW.bin`, `tex_SD/PVRTC.bin`) fallaba devolviendo 0 bytes leídos. El motor creía que los paquetes estaban vacíos (0 archivos indexados), por lo que cualquier búsqueda en `DataPack` fallaba y caía al fallback de disco suelto que no existía en el APK.
    6. Además, `AAsset_seek()` devolvía `0` (retorno de `fseek`) en vez de la nueva posición de byte exigida por la especificación de Android NDK (`ftell`), `AAsset_close()` llamaba al `fclose()` estándar en lugar de `sceLibcBridge_fclose()`, y `AAsset_openFileDescriptor()` abría nuevos descriptores de archivo sin liberarlos en `AAsset_close()`.
  - **Solución implementada:**
    1. `source/reimpl/asset_manager.cpp`: Se eliminó el `fclose(a->f)` destructivo en `AAssetManager_open()`.
    2. Se implementó `AAsset_seek()` retornando la posición real obtenida mediante `ftell/sceLibcBridge_ftell`.
    3. Se implementó `AAsset_getRemainingLength()`, `AAsset_getLength()` y la función faltante `AAsset_getBuffer()`, vinculada en `source/dynlib.c`.
    4. Se aseguró la apertura en modo binario `"rb"` y la normalización de rutas (eliminación de barras iniciales y fallback a `DATA_PATH`).
    5. Se evitó fuga de descriptores de archivo en `AAsset_openFileDescriptor()` manteniendo `fd` en la estructura del asset y cerrándolo en `AAsset_close()`.
  - **Build verificado:** `eboot.bin` y `badland2.vpk` compilados limpiamente.

- **2026-10-14 — Análisis de `logs/badland2_011.log` + `badland2-psp2core-1791948952-0x0006992d0b-eboot.bin.psp2dmp`:**
  - **Contexto:** Al arrancar en consola, el log se cortó abruptamente en `Init Shaders`.
  - **Ubicación y Crash:**
    `PC = 0xe00003d6` en `SceLibKernel` (`sceClibMemcpy`), `R1 = 0x00000000` (puntero fuente NULL), `LR = 0x81014389` (`serialize_shader` en vitaGL vendored).
    Stack trace completo: `f::ShaderManager::init()` (`0x35748f`) → `addShader()` (`0x35740b`) → `createShader()` (`0x341d37`) → `GLES_makeShader()` (`0x341c69`) → `glCompileShader_soloader()` (`glutil.c`).
  - **Causa raíz identificada:**
    1. En `source/utils/glutil.c`, existía un bloque `#ifdef DUMP_COMPILED_SHADERS` que llamaba a `vglGetShaderBinary(shader, 32 * 1024, &len, bin)` inmediatamente tras `glCompileShader(shader)`.
    2. Sin embargo, en `gl_preload()`, se configura `vglSetSemanticBindingMode(VGL_MODE_POSTPONED)`. En modo pospuesto, vitaGL no compila los shaders GLSL en `glCompileShader` sino que posterga su compilación hasta `glLinkProgram` (para enlazar correctamente los atributos y semánticas de hardware Vita). Por ende, en `glCompileShader`, `s->prog` es `NULL`.
    3. En `vendor/vitaGL/source/custom_shaders.c`, la función `vglGetShaderBinary()` verificaba si `s->prog == NULL` y establecía `GL_INVALID_OPERATION`, pero **le faltaba un `return;`**, continuando la ejecución hacia `serialize_shader()`, la cual ejecutaba `sceClibMemcpy(buf, s->prog, s->size)` con `s->prog == NULL` (Data Abort instantáneo en `SceLibKernel`).
  - **Solución implementada:**
    1. `vendor/vitaGL/source/custom_shaders.c`: Agregado `return;` en `vglGetShaderBinary()` tras detectar `s->prog == NULL`, y guarda defensiva en `serialize_shader()`.
    2. `CMakeLists.txt`:
       - Activado el cache nativo de vitaGL mediante `HAVE_SHADER_CACHE=1` en `VITAGL_MAKE_FLAGS_STR`. VitaGL calcula el hash XXH3 de cada shader y cachea el binario compilado de forma segura durante `glLinkProgram` con todas las semánticas resueltas.
       - Limitado `valid_dump_formats` a solo `CG` (deshabilitando el dumper ad-hoc para GLSL que rompía con `VGL_MODE_POSTPONED`).
       - Renombrado `.vitagl_flags` a `vitagl_flags` en `CMakeLists.txt` para evitar que `rsync` lo excluyera como dotfile.
    3. `source/utils/glutil.c`:
       - Añadido `vglSetShaderCachePath(DATA_PATH "cache")` en `gl_preload()`.
       - Protegido `glCompileShader_soloader()` con `!defined(USE_GLSL_SHADERS)` para evitar llamadas a `vglGetShaderBinary` en shaders GLSL no enlazados.
       - Simplificado `load_shader()` para GLSL como un passthrough limpio hacia `glShaderSource`.
    4. `source/utils/init.c`: Añadida creación de subdirectorios `ux0:data/badland2/cache/v4/v` y `f` requeridos por el shader cache de vitaGL.
    5. `.gitignore`: Anclados artefactos de build raíz (`/Makefile`, `/CMakeFiles/`, etc.) para prevenir que `rsync` excluyera `vendor/vitaGL/Makefile`.
  - **Build verificado:** `psvita-toolkit build` completado exitosamente (código 0). VPK y eboot generados en `build/`.

- **2026-10-14 — Análisis de `logs/badland2_012.log` (¡Hito mayor: Ingame alcanzado!):**
  - **Contexto:** Al probar en consola física, ¡el juego superó toda la carga, inicialización de datos, shaders y menús, cargando el nivel Genesis (`/level/GENESIS`, `Total triangles: 889`, `Mesh count: 162`, `objects: 1413`) y activando checkpoints (`ActiveWorld: activating cp 0`, `cp 1`) manteniéndose vivo a 150+ segundos!
  - **Problema 1: Ausencia de sonido (FMOD Error 51):**
    - En el log: `bAudioEngine.cpp(128): FMOD error 51 - Error initializing output device.`
    - Desensamblando `b::AudioEngine::init()` (0x1ad9b8) y `libfmod.so`:
      1. El motor llama a `FMOD::System::setSoftwareFormat(0, ...)` (0 = usar tasa de muestreo del hardware Android devuelta por Java).
      2. FMOD llama al método JNI `org.fmod.FMOD.getOutputSampleRate()`, el cual en `source/java.c` devolvía `0`.
      3. FMOD procede a inicializar OpenSL ES llamando a `CreateAudioPlayer` con un formato PCM con `samplesPerSec = 0`.
      4. En `libOpenSLES.a` (`sles.o`), la función `checkDataFormat` valida la tasa de muestreo contra una lista estricta (8000, 11025, ..., 44100, 48000). Al recibir 0, falla inmediatamente con `SL_RESULT_CONTENT_UNSUPPORTED` (9).
      5. `CreateAudioPlayer` falla y FMOD aborta la inicialización con el código de error 51 (`FMOD_ERR_OUTPUT_INIT`), poniendo `m_system` a NULL (lo que causaba los posteriores errores `FMOD error 30 - An invalid object handle was used`).
    - **Solución implementada:**
      - En `source/java.c`: Se implementó `FMOD_getOutputSampleRate` devolviendo la tasa nativa del backend de PS Vita `44100` Hz, y `FMOD_getOutputBlockSize` devolviendo `512` frames.
      - En `source/utils/init.c`: Se añadió la creación de los directorios `DATA_PATH "logs"` y `saves` para que el archivo `stderr.log` de OpenSL ES se guarde correctamente.
  - **Problema 2: Controles no responsivos:**
    - Diagnóstico:
      1. El muestreo de hardware de PS Vita no estaba iniciado (`sceTouchSetSamplingState` y `sceCtrlSetSamplingModeExt` nunca se llamaban en el arranque).
      2. `Main_JNI_isControllerOnly` en `source/java.c` devolvía `1`, indicando erróneamente al juego que el dispositivo no tenía pantalla táctil.
      3. `AInputQueue` en `source/reimpl/input_queue.c` y los accesores de `AInputEvent` en `source/reimpl/input_event.c` eran stubs vacíos que nunca encolaban ni leían eventos.
    - **Solución implementada:**
      - En `source/utils/init.c`: Se inicializó `sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG_WIDE)`, `sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START)` y se desactivó el panel trasero (`SCE_TOUCH_SAMPLING_STATE_STOP`) para evitar toques accidentales.
      - En `source/java.c`: Se cambió `Main_JNI_isControllerOnly` para devolver `0` (pantalla táctil activa).
      - En `source/reimpl/input_event.h` y `input_event.c`: Se implementó la estructura completa `AInputEvent` y todos los getters Android NDK requeridos (`AInputEvent_getType`, `AInputEvent_getSource`, `AInputEvent_getDeviceId`, `AKeyEvent_getAction`, `AKeyEvent_getKeyCode`, `AMotionEvent_getAction`, `AMotionEvent_getPointerCount`, `AMotionEvent_getPointerId`, `AMotionEvent_getX`, `AMotionEvent_getY`, `AMotionEvent_getAxisValue`).
      - En `source/reimpl/input_queue.c`:
        - Sistema de cola circular thread-safe con `pthread_mutex_t`.
        - Muestreo de pantalla táctil frontal (`sceTouchPeek`) escalado a 960x544 con seguimiento de IDs de puntero y eventos `DOWN`, `MOVE` y `UP`.
        - Muestreo de botones físicos (`sceCtrlPeekBufferPositive`) mapeados a `AKeyEvent` (Cross = Button A, Circle = Button B, Square = Button X, Triangle = Button Y, D-Pad, L1, R1, Start, Select).
        - **Mapeo virtual de aleteo ingame para Badland 2:**
          - L Trigger / D-Pad Izquierda / Stick Izquierdo Izquierda (< 80) → Aleteo hacia la izquierda (toque virtual en X=240, Y=272).
          - R Trigger / D-Pad Derecha / Stick Izquierdo Derecha (> 175) → Aleteo hacia la derecha (toque virtual en X=720, Y=272).
          - Cruz / D-Pad Arriba / Stick Arriba (< 80) → Aleteo simultáneo para ascender.
          - Si el usuario toca la pantalla física, los aleteos virtuales se cancelan de inmediato dando total prioridad a la pantalla táctil.
  - **Build verificado:** `psvita-toolkit build` completado exitosamente (código 0). VPK y eboot listos en `build/`.

- **2026-10-14 — Análisis de `logs/badland2_013.log` y Crash Dump `badland2-psp2core-1792002853-0x00062337ef-eboot.bin.psp2dmp`:**
  - **Contexto:** El usuario confirmó que llegó a in-game (nivel Genesis visible a 60 FPS según captura `screenshots/fg/2026-10-14/2026-10-14-143408.jpg`), pero sufrió un crash al pausar / cambiar de estado a los 56.6s.
  - **Triage del crash dump (`0x00062337ef`):**
    - Hilo causante: `pthread` (ID: `0x400101b7`).
    - Tipo de excepción: `Data abort exception (0x30004)` (intento de lectura en dirección de memoria inválida).
    - Registros:
      - `PC = 0x985d0cc8` (`libbadland2.so` base `0x98400000` + `0x1d0cc8`).
      - `LR = 0x985d0cc1`.
      - `R0 = 0x00000000` (puntero NULL).
      - `R1 = 0x988c2648`.
    - Desensamblado en `0x1d0cc8`:
      - La función es `b::StateGamePause::requestFacebookFriendsInfo(int)` (inicio en `0x1d0c8c`).
      - Instrucción que falla: `ldr r3, [r0, #0]` con `r0 = 0` (dereferencia de puntero NULL).
  - **Causa raíz identificada (cadena de causalidad):**
    1. En el log 013:
       ```
       [000696][ 56.405][INFO ] [ALOG][Badland2] loading ui is 9
       [000697][ 56.656][INFO ] [ALOG][Badland2] IAP::isNetConnected()1
       [000698][ 56.669][DEBUG] [ALOG][Badland2] Failed to find method id of JNI_getUserId
       ```
    2. Al entrar a la pausa (UI 9), la función `b::IAP::isNetConnected()` evalúa si hay conexión.
    3. En `JNI_IsNetConnected@@Base` (offset `0x247160`), si el método no se encuentra en Java (`f::JniUtils::getStaticMethodInfo` devuelve 0), el código binario tiene un fallback por defecto que establece `r5 = 1` y retorna `1` (true).
    4. Asimismo, `JNI_getLoginState@@Base` (offset `0x18536c`) cae en un fallback similar retornando `1` (sesión iniciada).
    5. Creyendo falsamente que había conexión y login de Facebook activo, `b::StateGamePause::showMenu` invoca `requestFacebookFriendsInfo(int)`.
    6. Dicha función carga el singleton de red/social desde el GOT, el cual es `NULL` en modo offline soloader, y desreferencia inmediatamente su vtable (`[r0, #0]`), provocando el Data Abort en `0x1d0cc8`.
  - **Solución implementada:**
    1. `source/patch.c`:
       - Se hookeó `b::IAP::isNetConnected()` (`_ZN1b3IAP14isNetConnectedEv`) y `b::IAPAndroid::IsNetConnected()` (`_ZN1b10IAPAndroid14IsNetConnectedEv`) para retornar siempre `0` (offline).
       - Se hookeó `FacebookAndroid::getState()` (`_ZN15FacebookAndroid8getStateEv`) para retornar siempre `0` (desconectado).
       - Se hookearon `b::StateGamePause::requestFacebookFriendsInfo` (`_ZN1b14StateGamePause26requestFacebookFriendsInfoEi`), `requestChallengeRankInfo` (`_ZN1b14StateGamePause24requestChallengeRankInfoEi`) y `requestRushFriendsInfo` (`_ZN1b14StateGamePause22requestRushFriendsInfoEii`) como funciones no-op seguras.
       - Se hookearon las consultas de amigos de Facebook en la selección de niveles (`StateLevelSelect` y `StateLevelSelectBonus`) como no-op para evitar desreferencias similares en menús de mapa.
    2. `source/java.c`:
       - Se agregaron las implementaciones FalsoJNI para `com/frogmind/badland2/FacebookManager`:
         - `JNI_getUserId` → retorna cadena vacía `""`.
         - `JNI_getUserTokenString` → retorna cadena vacía `""`.
         - `JNI_getUserName` → retorna cadena vacía `""`.
         - `JNI_getUserFirstName` → retorna cadena vacía `""`.
         - `JNI_getUserSurname` → retorna cadena vacía `""`.
         - `JNI_getLoginState` → retorna `0` (desconectado).
         - `JNI_loginWithFacebook` y `JNI_logoutWithFacebook` → no-op seguro.
    3. `.gitignore`: Se ajustaron las reglas para no ignorar `vendor/vitaGL` ni los iconos de LiveArea, garantizando builds limpios y reproducibles mediante `psvita-toolkit`.
    4. Documentación:
       - Actualizado `README.md` siguiendo el estándar de los mejores ports de PS Vita (Rayman Jungle Run, The Impossible Game).
       - Creado `RELEASE.md` detallando las novedades de la v1.0, controles y guía de instalación.
  - **Build verificado:** `psvita-toolkit build` ejecutado exitosamente generando el VPK y `eboot.bin` actualizados.

- **2026-10-14 — Solución de audio y triage de crash en debugBank (FMOD AudioTrack + SceLibcBridge fseeko + patch noop):**
  - **Diagnóstico del problema (badland2_014.log + dump badland2-psp2core-1792004289):**
    1. En `badland2_014.log`, `b::AudioEngine::init()` fallaba con:
       `FMOD error 51 - Error initializing output device.`
       Seguido de:
       `FMOD error 30 - An invalid object handle was used. Cannot load audio bank /MasterBank.bank`
    2. El crash o fallo de salida se debía a que `libfmod.so` seleccionaba `FMOD_OUTPUTTYPE_OPENSL` (16) al recibir valores positivos de `getOutputSampleRate` y `getOutputBlockSize`. La implementación de OpenSL ES en VitaSDK falla en `CreateAudioPlayer` con `SL_RESULT_CONTENT_UNSUPPORTED`.
    3. El fallo al cargar bancos (error 30) se debía a que en `source/dynlib.c`, `fseeko` y `ftello` invocaban directamente las funciones de `newlib` en vez de `sceLibcBridge_fseek` y `sceLibcBridge_ftell`. Bajo `USE_SCELIBC_IO`, `fopen` devuelve un `SceLibc FILE*`, incompatible con newlib.
    4. El crash analizado en el dump `badland2-psp2core-1792004289-0x000a56208b-eboot.bin.psp2dmp` (PC `0x985adb34`) ocurrió en `b::AudioEngine::debugBank(FMOD::Studio::Bank *bank)`. Como `/MasterBank.bank` devolvió `NULL`, `debugBank` recibió `bank = NULL` (`r0 = 0x0`) e intentó invocar la vtable `bank->getEventList(...)` desreferenciando la dirección cero (Data Abort).
  - **Solución implementada:**
    1. `source/dynlib.c`:
       - Se implementó `dlopen_soloader` para rechazar explícitamente `"libOpenSLES.so"` (retorna `NULL`), forzando a FMOD a usar la salida estándar `AudioTrack`.
       - Se envolvieron `fseeko` y `ftello` dentro del bloque `#ifdef USE_SCELIBC_IO`, enrutándolos a `sceLibcBridge_fseek` y `sceLibcBridge_ftell`.
    2. `source/java.c`:
       - `FMOD_getOutputSampleRate` y `FMOD_getOutputBlockSize` retornan `0`, señalando a FMOD que la salida de baja latencia no está presente y seleccionando automáticamente `FMOD_OUTPUTTYPE_AUDIOTRACK` (15).
       - Se implementó el puente nativo para `org/fmod/AudioDevice`:
         - `AudioDevice_ctor`: Retorna puntero de objeto válido.
         - `AudioDevice_init`: Abre el puerto `SceAudioOut` (`SCE_AUDIO_OUT_PORT_TYPE_BGM`, 44.1 kHz, estéreo) con volumen `0 dB` e inicializa un buffer FIFO de acumulación.
         - `AudioDevice_write`: Recibe el buffer PCM generado por el hilo de mezcla de FMOD, lo acumula en el FIFO y lo envía a la tarjeta de sonido de PS Vita mediante `sceAudioOutOutput()`.
         - `AudioDevice_close`: Libera el puerto de audio y desaloja la memoria del buffer.
       - Se registraron los métodos en `nameToMethodId`, `methodsObject`, `methodsBoolean` y `methodsVoid`.
    3. `source/patch.c`:
       - Se hookeó `b::AudioEngine::debugBank(FMOD::Studio::Bank*)` (`_ZN1b11AudioEngine9debugBankEPN4FMOD6Studio4BankE`) a `noop_func`. `debugBank` es una función puramente cosmética que imprime eventos de FMOD en logcat y no afecta el gameplay ni el audio; anularla previene permanentemente cualquier desreferencia a `NULL` si algún banco no existe o falla en cargar.
  - **Build verificado:** `psvita-toolkit build` ejecutado exitosamente con código 0, generando `eboot.bin` y `badland2.vpk`.

- **Crash al guardar `save.bin` (logs 014/015, Prefetch abort, PC = `libbadland2+0x4b1ac4` = `_GLOBAL_OFFSET_TABLE_`):**
  - `SaveGame::save()` → `FacebookAndroid::getUserId()` → `JNI_getUserId`. El epílogo de `JNI_getUserId` (`ldmia sp!, {r4-r9,pc}`) sacó basura del stack: su prólogo `stmdb sp!, {r4-r9,lr}` nunca corrió.
  - Causa: el hook de `FacebookAndroid::getState()` (0x1853a8) — esa función mide 4 bytes (`b.w JNI_getLoginState`) y el trampolín Thumb de `hook_addr` mide 8, así que pisaba los primeros 4 bytes de `JNI_getUserId` (0x1853ac). Se manifestó al registrar `JNI_getUserId` en `java.c` (antes salía temprano por "method not found").
  - Fix (`source/patch.c`): hookear `JNI_getLoginState` (0x5c bytes) en vez de `getState`. Los demás hooks miden ≥16 bytes.
  - Regla: antes de `hook_addr`, comprobar que la función mida ≥8 bytes (distancia al siguiente símbolo).

- **Touch lento / hacen falta varios toques (log 016):**
  - `input_queue.c` re-leía el hardware en cada `has_events`/`getEvent` con la cola vacía; con el dedo apoyado siempre generaba un `MOVE`, así que el bucle "vaciar input" del juego no terminaba hasta levantar el dedo (DOWN y UP llegaban en el mismo frame y el botón no se registraba).
  - Fix: muestrear touch/pad como máximo una vez cada 8 ms (`sceKernelGetProcessTimeWide`).
- **Crash en nivel tutorial (log 016, sin resolver):** Prefetch abort PC=0x1f0 en `b2Contact::Destroy` ← `b2ContactManager::Destroy` ← `b2Body::SetType` ← `GameObject::setDynamic` ← `SignalSystem::EventEnabler::action`. El `b2PolygonAndCircleContact` está sano, pero el `destroyFcn` leído de `s_registers` vale 0x1f2 (fixture con tipo de shape basura o tabla pisada). El dump no incluye heap ni `.bss` del `.so`: falta evidencia para la causa. Reprobar con el fix de input.

- **Touch no responde (log 018):** el handler de input del juego (`.so` 0x13fac0) guarda dedos en una tabla de 5 entradas indexada por pointer id e ignora ids > 4; `input_queue.c` pasaba `SceTouchReport.id` (contador creciente de la Vita). Fix: asignar cada dedo a un slot libre 0..4, DOWN/UP por dedo, MOVE solo si hubo movimiento. Botones: START → `BUTTON_B` (pausa, bit NvGamepad 0x2000); CIRCLE sin mapear (pedido del usuario).

- **Flaps físicos débiles (log 020):** `b::Controller::onPressStart` (0x22d798) llama `PlayerManager::onTouchStart(..., fuerza=|valor|)`. Los botones (`gp::Gamepad::gamepadButtonChanged`, 0x1407dc) llegan con valor 0.0 → aleteo sin fuerza; solo el stick (`checkGamepadAnalog`, 0x14058c; ejes AXIS_X/AXIS_Y vía eventos MOTION con source JOYSTICK 0x01000010) pasa magnitud. Fix: d-pad/L/R/stick/CROSS → eventos de joystick (izq x=-1, der x=+1, ambos y=-1), sin keys DPAD/L1/R1 ni toques simulados. Sin probar en consola.

## Cómo seguir (una cosa a la vez)
1. Desplegar el nuevo binario a la consola mediante `psvita-toolkit deploy --eboot`.
2. Lanzar Badland 2 y verificar que:
   - FMOD inicializa correctamente con `AudioDevice.init` abriendo el puerto `SceAudioOut`.
   - Los bancos `/MasterBank.bank` y `/MasterBank.strings.bank` cargan sin error 30 ni 51.
   - `debugBank` no desreferencia punteros nulos.
   - La música y efectos sonoros se escuchan con total claridad a 44.1 kHz durante la partida.
   - El menú de pausa y la navegación táctil y física siguen operando sin crasheos.
3. Continuar disfrutando de la campaña de Badland 2 a 60 FPS en PS Vita con audio completo.


