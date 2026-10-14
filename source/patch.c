/*
 * Copyright (C) 2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

/**
 * @file  patch.c
 * @brief Patching some of the .so internal functions or bridging them to native
 *        for better compatibility.
 */

#include <kubridge.h>
#include <so_util/so_util.h>
#include "utils/logger.h"

extern so_module so_mod;

static int ret0(void) {
    return 0;
}

static void noop_func(void) {
}

void so_patch(void) {
    l_info("Applying game patches...");

    // Force network disconnected: Vita is running offline soloader
    uintptr_t isNetConnected_sym = so_symbol(&so_mod, "_ZN1b3IAP14isNetConnectedEv");
    if (isNetConnected_sym) {
        hook_addr(isNetConnected_sym, (uintptr_t)&ret0);
    }

    uintptr_t iapAndroid_isNetConnected_sym = so_symbol(&so_mod, "_ZN1b10IAPAndroid14IsNetConnectedEv");
    if (iapAndroid_isNetConnected_sym) {
        hook_addr(iapAndroid_isNetConnected_sym, (uintptr_t)&ret0);
    }

    // Force Facebook state to logged out / disconnected
    uintptr_t fb_getState_sym = so_symbol(&so_mod, "_ZN15FacebookAndroid8getStateEv");
    if (fb_getState_sym) {
        hook_addr(fb_getState_sym, (uintptr_t)&ret0);
    }

    // Disable social/network ranking queries on Pause menu to prevent NULL derefs
    uintptr_t pause_reqFb_sym = so_symbol(&so_mod, "_ZN1b14StateGamePause26requestFacebookFriendsInfoEi");
    if (pause_reqFb_sym) {
        hook_addr(pause_reqFb_sym, (uintptr_t)&noop_func);
    }

    uintptr_t pause_reqChallenge_sym = so_symbol(&so_mod, "_ZN1b14StateGamePause24requestChallengeRankInfoEi");
    if (pause_reqChallenge_sym) {
        hook_addr(pause_reqChallenge_sym, (uintptr_t)&noop_func);
    }

    uintptr_t pause_reqRush_sym = so_symbol(&so_mod, "_ZN1b14StateGamePause22requestRushFriendsInfoEii");
    if (pause_reqRush_sym) {
        hook_addr(pause_reqRush_sym, (uintptr_t)&noop_func);
    }

    // Disable social queries in level select menus
    uintptr_t lvl_reqFb_sym = so_symbol(&so_mod, "_ZN1b16StateLevelSelect26requestFacebookFriendsInfoEii");
    if (lvl_reqFb_sym) {
        hook_addr(lvl_reqFb_sym, (uintptr_t)&noop_func);
    }

    uintptr_t lvlBonus_reqFb_sym = so_symbol(&so_mod, "_ZN1b21StateLevelSelectBonus26requestFacebookFriendsInfoEii");
    if (lvlBonus_reqFb_sym) {
        hook_addr(lvlBonus_reqFb_sym, (uintptr_t)&noop_func);
    }

    l_info("Patches applied successfully.");
}
