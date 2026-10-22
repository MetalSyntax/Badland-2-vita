/*
 * Copyright (C) 2021      Andy Nguyen
 * Copyright (C) 2021-2022 Rinnegatamante
 * Copyright (C) 2022-2024 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "utils/init.h"

#include "utils/dialog.h"
#include "utils/glutil.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/settings.h"

#include <string.h>

#include <psp2/appmgr.h>
#include <psp2/apputil.h>
#include <psp2/kernel/clib.h>
#include <psp2/power.h>
#include <psp2/io/stat.h>
#include <psp2/ctrl.h>
#include <psp2/touch.h>

#include <falso_jni/FalsoJNI.h>
#include <so_util/so_util.h>
#include <fios/fios.h>

// Virtual load addresses for multi-module layout (aligned to 2MB boundaries)
#define LOAD_ADDRESS_FMOD       0x98000000
#define LOAD_ADDRESS_FMODSTUDIO 0x98200000
#define LOAD_ADDRESS_GAME       0x98400000

#define FMOD_SO_PATH       DATA_PATH "libfmod.so"
#define FMODSTUDIO_SO_PATH DATA_PATH "libfmodstudio.so"

void soloader_init_all() {
    log_init();

	// Launch `app0:configurator.bin` on `-config` init param
    sceAppUtilInit(&(SceAppUtilInitParam){}, &(SceAppUtilBootParam){});
    SceAppUtilAppEventParam eventParam;
    sceClibMemset(&eventParam, 0, sizeof(SceAppUtilAppEventParam));
    sceAppUtilReceiveAppEvent(&eventParam);
    if (eventParam.type == 0x05) {
        char buffer[2048];
        sceAppUtilAppEventParseLiveArea(&eventParam, buffer);
        if (strstr(buffer, "-config"))
            sceAppMgrLoadExec("app0:/configurator.bin", NULL, NULL);
    }

    // Set default overclock values
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

#ifdef USE_SCELIBC_IO
    if (fios_init(DATA_PATH) == 0)
        l_success("FIOS initialized.");
#endif

    // Set up Vita input hardware sampling
    sceCtrlSetSamplingModeExt(SCE_CTRL_MODE_ANALOG_WIDE);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_STOP);

    // Ensure runtime directories exist
    sceIoMkdir(DATA_PATH "logs", 0777);
    sceIoMkdir(DATA_PATH "saves", 0777);
    sceIoMkdir(DATA_PATH "cache", 0777);
    sceIoMkdir(DATA_PATH "cache/v4", 0777);
    sceIoMkdir(DATA_PATH "cache/v4/v", 0777);
    sceIoMkdir(DATA_PATH "cache/v4/f", 0777);
    sceIoMkdir(DATA_PATH "gxp", 0777);
    sceIoMkdir(DATA_PATH "cg", 0777);
    sceIoMkdir(DATA_PATH "glsl", 0777);

    if (!module_loaded("kubridge")) {
        l_fatal("kubridge is not loaded.");
        fatal_error("Error: kubridge.skprx is not installed.");
    }
    l_success("kubridge check passed.");

    // 1. Load libfmod.so if present
    if (file_exists(FMOD_SO_PATH)) {
        l_info("Loading %s at 0x%08x...", FMOD_SO_PATH, LOAD_ADDRESS_FMOD);
        if (so_file_load(&so_mod_fmod, FMOD_SO_PATH, LOAD_ADDRESS_FMOD) < 0) {
            l_fatal("libfmod.so could not be loaded.");
            fatal_error("Error: could not load %s.", FMOD_SO_PATH);
        }
        so_relocate(&so_mod_fmod);
        resolve_imports(&so_mod_fmod);
        so_flush_caches(&so_mod_fmod);
        so_initialize(&so_mod_fmod);
        l_success("libfmod.so initialized.");
    } else {
        l_warn("%s not found -- FMOD audio may be unavailable.", FMOD_SO_PATH);
    }

    // 2. Load libfmodstudio.so if present
    if (file_exists(FMODSTUDIO_SO_PATH)) {
        l_info("Loading %s at 0x%08x...", FMODSTUDIO_SO_PATH, LOAD_ADDRESS_FMODSTUDIO);
        if (so_file_load(&so_mod_fmodstudio, FMODSTUDIO_SO_PATH, LOAD_ADDRESS_FMODSTUDIO) < 0) {
            l_fatal("libfmodstudio.so could not be loaded.");
            fatal_error("Error: could not load %s.", FMODSTUDIO_SO_PATH);
        }
        so_relocate(&so_mod_fmodstudio);
        resolve_imports(&so_mod_fmodstudio);
        so_flush_caches(&so_mod_fmodstudio);
        so_initialize(&so_mod_fmodstudio);
        l_success("libfmodstudio.so initialized.");
    } else {
        l_warn("%s not found -- FMOD Studio may be unavailable.", FMODSTUDIO_SO_PATH);
    }

    // 3. Load main game binary (libbadland2.so)
    // v1.0 looked for main.so; keep accepting it for users who renamed the file.
    const char *so_path = SO_PATH;
    if (!file_exists(so_path) && file_exists(DATA_PATH "main.so"))
        so_path = DATA_PATH "main.so";

    if (!file_exists(so_path)) {
        fatal_error("Looks like you haven't installed the data files for this "
                    "port, or they are in an incorrect location. Please make "
                    "sure that you have %s file exactly at that path.", SO_PATH);
    }

    l_info("Loading %s at 0x%08x...", so_path, LOAD_ADDRESS_GAME);
    if (so_file_load(&so_mod, so_path, LOAD_ADDRESS_GAME) < 0) {
        l_fatal("SO could not be loaded.");
        fatal_error("Error: could not load %s.", so_path);
    }

    settings_load();
    l_success("Settings loaded.");

    so_relocate(&so_mod);
    l_success("SO relocated.");

    resolve_imports(&so_mod);
    l_success("SO imports resolved.");

    so_patch();
    l_success("SO patched.");

    so_flush_caches(&so_mod);
    l_success("SO caches flushed.");

    so_initialize(&so_mod);
    l_success("SO initialized.");

    gl_preload();
    l_success("OpenGL preloaded.");

    jni_init();
    l_success("FalsoJNI initialized.");
}
