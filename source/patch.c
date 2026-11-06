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
#include "trophies.h"

#include <string.h>

extern so_module so_mod;

static int ret0(void) {
    return 0;
}

static void noop_func(void) {
}


/**
 * Trophies. Every achievement goes through the static
 * SocialManager::reportAchievement(const std::string &name, int percent, bool)
 * (percent clamped to 100 = completed). The JNI path below it
 * (SocialPlatformGooglePlay -> JNI_reportAchievement) only runs with a Google
 * Play session (JNI_isSignedIn), so it never fires on Vita: hook here instead.
 * Name = internal key (libstdc++ COW std::string: first word = char data).
 * Trophy N <-> BL2_ACHIEVEMENTS[N], same order as extras/trophy/trophies.json
 * (generated from it). Two keys exist with a space ("u turn", "zero gravity")
 * in the engine's Google Play map, so spaces are matched as '_'.
 */
static const char *const BL2_ACHIEVEMENTS[] = {
    NULL, /* 0 = Platinum, unlocked by the OS */
    "starter", "polar_express", "boiled", "kubrik", "quantum",
    "evolution", "double_figures", "eggtastic", "sixties", "average_iq",
    "triple_figures", "one_fiddy", "u_turn", "liberator", "clone_hugger",
    "magician", "millenium", "black_swarm", "clone_master", "savior",
    "y2k", "dedicated", "legacy", "oh_my_clone", "quadratic",
    "unfortunate_errors", "collateral_damage", "not_afraid", "pyro", "bonfire",
    "sunshine", "natural_selection", "private_road", "big_red_eye", "cooling",
    "snowball", "winter_is", "where_is", "recycling", "porridge",
    "globetrotter", "dizzy", "race_driver", "on_the_road", "60_minutes",
    "true_gamer", "time_to_go_outside", "seriously_addicted", "stop_whenever", "semipro",
    "thumb_surgeon", "perfectionistish", "professional", "annoyingly_good", "specialist",
    "prodigy", "deminer", "demo_expert", "halloween_party", "clonefall_survivor",
    "butterfly", "zero_gravity", "bomberman", "skateboarder", "surefire",
    "minimalist", "great_escape", "twins", "alone", "clonerush_survivor",
    "forty_below", "untouchable", "tower", "big_eyes", "navigator",
    "all_together", "five_red_lights", "ancient_treasure", "electric_sheep",
};
#define BL2_NUM_ACHIEVEMENTS (sizeof(BL2_ACHIEVEMENTS) / sizeof(BL2_ACHIEVEMENTS[0]))

static so_hook reportAchievement_hook;
static uint8_t trophies_sent[BL2_NUM_ACHIEVEMENTS]; /* once per session: the queue holds 32 */

static int key_eq(const char *engine, const char *key) {
    for (; *engine && *key; engine++, key++)
        if (*engine != *key && !(*engine == ' ' && *key == '_'))
            return 0;
    return *engine == *key;
}

static void SocialManager_reportAchievement(const void *name, int percent, int flag) {
    const char *n = *(const char *const *)name;
    // Few calls (level end / stat milestones), so INFO is cheap: proves the hook fires.
    l_info("SocialManager::reportAchievement(%s, %d)", n ? n : "(null)", percent);
    if (percent >= 100 && n) {
        uint32_t id;
        for (id = 1; id < BL2_NUM_ACHIEVEMENTS; id++)
            if (key_eq(n, BL2_ACHIEVEMENTS[id]))
                break;
        if (id == BL2_NUM_ACHIEVEMENTS) {
            l_warn("reportAchievement(%s): no trophy for this key", n);
        } else if (!trophies_sent[id]) {
            trophies_sent[id] = 1;
            l_info("Achievement \"%s\" -> trophy %u", n, (unsigned)id);
            trophies_unlock(id);
        }
    }
    // int: SO_CONTINUE needs a non-void type; the real function returns void.
    SO_CONTINUE(int, reportAchievement_hook, name, percent, flag);
}


/* Language change -> b::StateManager::recreateAllStates() -> f::StateMachine::uninit()/initAll(),
 * which call a virtual on every one of the 62 state slots. StateManager::init never creates
 * state 58, so the slot is NULL and the engine data-aborts (dump 0x000e8e2631). Same loops, NULL-safe. */
static int *sm_stateCount;
static void ***sm_states;
static uint8_t *sm_initialized;

static void sm_call_all(int vslot) {
    for (int i = 0; i < *sm_stateCount; i++) {
        void **st = (*sm_states)[i];
        if (st)
            ((void (*)(void *))(*(void ***)st)[vslot])(st);
    }
}
static void StateMachine_uninit(void) { sm_call_all(7); *sm_initialized = 0; }  /* vtable +0x1c */
static void StateMachine_initAll(void) { sm_call_all(5); *sm_initialized = 1; } /* vtable +0x14 */

/* Premium, Rayman Jungle Run style: report the store items as owned. b::IAP::isIapPurchased(type)
 * reads m_iapRuntime[type]; 0 = UNLOCK_ALL ("UPGRADE PREMIUM"), 1 = NOADS, 16-18 = world unlocks,
 * 19 = eternal life, 20 = max life (isWorldLocked/isEternalLife/isAddMaxLife go through it).
 * Other types (consumable packs) keep the real answer. */
static so_hook isIapPurchased_hook;
static int IAP_isIapPurchased(int type) {
    if (type == 0 || type == 1 || (type >= 16 && type <= 20))
        return 1;
    return SO_CONTINUE(int, isIapPurchased_hook, type);
}

/* Online-only UI (Facebook/Twitter, video ads, leaderboards, promo, Google Play, store restore)
 * goes to 0% opacity. Names are the real node names from the /ui/*.txt layouts in data.bin;
 * only names unique to online features (no generic BACK/BUTTON). Applied after
 * f::UINodeLoader::parseBaseNode, once name/size/col/mask are set. UINode: +0x08 name[16],
 * +0x24 size, +0x48 color alpha (multiplied into children), +0x5c flags (0x40 = skip render).
 * Size 0 because the hit test ignores alpha and the hidden flag. */
static const char *const ONLINE_NODES[] = {
    /* main menu */ "FACEBOOK", "FBBTN", "FBICON", "FBTIP", "fblabel", "SOCIAL", "TWITTER",
    "ONLINE", "PROMOBANNER", "CUBEAD", "CUBEADCOUNT", "SLOWAD", "SLOWADCOUNT", "BuyIcon",
    /* options */ "FORUM", "GOOGLE", "RESTORE",
    /* level select */ "FBInvite", "FBLogin", "FacebookInvite", "FacebookLogin", "Invite",
    "InviteLabel", "Rank", "RankTips",
    /* lives / shop / ad popups */ "Video", "VideoBtn", "PayVersionBtn", "VedioButton",
    "VedioTimeLabel", "VideoBg", "VideoGrayBg", "ADPASS", "ADPIC", "ADSHOW", "ADTOUCH",
    "OnlineDes", "BTNSHOWAD", "ShowAdIcon", "TEXTSHOWAD", "SelfAdBg", "SelfAdNode",
    "SelfAdText", "SelfAdTouch",
};
static so_hook parseBaseNode_hook;
static int UINodeLoader_parseBaseNode(uint8_t *node, void *json) {
    int r = SO_CONTINUE(int, parseBaseNode_hook, node, json);
    const char *name = (const char *)(node + 0x08);
    for (unsigned i = 0; i < sizeof(ONLINE_NODES) / sizeof(ONLINE_NODES[0]); i++) {
        if (strcmp(name, ONLINE_NODES[i]) == 0) {
            *(float *)(node + 0x48) = 0.0f;
            *(float *)(node + 0x24) = 0.0f;
            *(float *)(node + 0x28) = 0.0f;
            *(uint16_t *)(node + 0x5c) |= 0x40;
            l_debug("ui: hid online node %s", name);
            break;
        }
    }
    return r;
}

void so_patch(void) {

    isIapPurchased_hook = hook_addr(so_symbol(&so_mod, "_ZN1b3IAP14isIapPurchasedENS0_8IAP_TYPEE"),
                                    (uintptr_t)&IAP_isIapPurchased);
    parseBaseNode_hook = hook_addr(so_symbol(&so_mod, "_ZN1f12UINodeLoader13parseBaseNodeEPNS_6UINodeERN4Json5ValueE"),
                                   (uintptr_t)&UINodeLoader_parseBaseNode);

    sm_stateCount = (int *)so_symbol(&so_mod, "_ZN1f12StateMachine12m_stateCountE");
    sm_states = (void ***)so_symbol(&so_mod, "_ZN1f12StateMachine8m_statesE");
    sm_initialized = (uint8_t *)so_symbol(&so_mod, "_ZN1f12StateMachine13m_initializedE");
    hook_addr(so_symbol(&so_mod, "_ZN1f12StateMachine6uninitEv"), (uintptr_t)&StateMachine_uninit);
    hook_addr(so_symbol(&so_mod, "_ZN1f12StateMachine7initAllEv"), (uintptr_t)&StateMachine_initAll);

    reportAchievement_hook = hook_addr(
            (uintptr_t)so_symbol(&so_mod, "_ZN13SocialManager17reportAchievementERKSsib"),
            (uintptr_t)&SocialManager_reportAchievement);

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
    // FacebookAndroid::getState is a 4-byte `b.w JNI_getLoginState`; the 8-byte
    // hook trampoline would clobber the prologue of the adjacent JNI_getUserId.
    uintptr_t fb_getState_sym = so_symbol(&so_mod, "JNI_getLoginState");
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

    // Disable debugBank logging: prevents dereferencing NULL bank handles if a bank load ever fails
    uintptr_t debugBank_sym = so_symbol(&so_mod, "_ZN1b11AudioEngine9debugBankEPN4FMOD6Studio4BankE");
    if (debugBank_sym) {
        hook_addr(debugBank_sym, (uintptr_t)&noop_func);
    }

    l_info("Patches applied successfully.");
}
