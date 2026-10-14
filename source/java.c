#include <falso_jni/FalsoJNI_Impl.h>
#include <falso_jni/FalsoJNI.h>

#include "utils/logger.h"

/*
 * JNI Methods
 *
 * Only methods the native .so actually looks up (verified: exact method-name
 * strings present in libbadland2.so via `strings`) are registered here, with
 * semantics read from the real Java implementations in
 * decompiled/apk_jadx/sources/com/frogmind/badland2/Main.java.
 * Everything else keeps FalsoJNI's defaults (void=no-op, object=NULL,
 * boolean=false, int=-1) until a device log proves it is called.
 */

// --- com.frogmind.badland2.Main static helpers called from native code ---

static jobject Main_JNI_textAsBitmap(jmethodID id, va_list args) {
    // Main.JNI_textAsBitmap(String, float): renders text with the bundled
    // "American Captain.ttf" via android.graphics.Canvas. No Canvas on Vita.
    // RISK (so-crash-triage §8): NULL bitmap -- the native wrapper's
    // NULL-handling is UNVERIFIED (no Ghidra yet: docker daemon down). If the
    // first boot crashes in AndroidBitmap_lockPixels/getInfo or right after
    // this log line, this stub is suspect #1. Real impl = Phase 5
    // (stb_truetype, cf. text_rendering reference).
    (void)id;
    (void)args;
    l_error("Main.JNI_textAsBitmap called -- returning NULL (Phase 5 TODO).");
    return NULL;
}

#include <psp2/apputil.h>
#include <psp2/system_param.h>

static jint Main_JNI_getLocalLanguage(jmethodID id, va_list args) {
    (void)id;
    (void)args;
    int lang = SCE_SYSTEM_PARAM_LANG_ENGLISH_US;
    sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, &lang);

    switch (lang) {
        case SCE_SYSTEM_PARAM_LANG_FRENCH:
            return 1;
        case SCE_SYSTEM_PARAM_LANG_ITALIAN:
            return 2;
        case SCE_SYSTEM_PARAM_LANG_GERMAN:
            return 3;
        case SCE_SYSTEM_PARAM_LANG_SPANISH:
            return 4;
        case SCE_SYSTEM_PARAM_LANG_PORTUGUESE_PT:
        case SCE_SYSTEM_PARAM_LANG_PORTUGUESE_BR:
            return 5;
        case SCE_SYSTEM_PARAM_LANG_RUSSIAN:
            return 6;
        case SCE_SYSTEM_PARAM_LANG_FINNISH:
            return 8;
        case SCE_SYSTEM_PARAM_LANG_CHINESE_S:
            return 9;
        case SCE_SYSTEM_PARAM_LANG_CHINESE_T:
            return 13;
        case SCE_SYSTEM_PARAM_LANG_KOREAN:
            return 10;
        case SCE_SYSTEM_PARAM_LANG_JAPANESE:
            return 11;
        case SCE_SYSTEM_PARAM_LANG_TURKISH:
            return 12;
        default:
            return 0; // English default
    }
}

static jint Main_JNI_IsNetConnected(jmethodID id, va_list args) {
    // No network stack wired yet: report offline so the engine takes its
    // cached/local-data paths (nativeCheckCachedData exists for this).
    (void)id;
    (void)args;
    return 0;
}

static jint Main_JNI_isControllerOnly(jmethodID id, va_list args) {
    // PS Vita has a front touchscreen (and physical controls). Returning 0 enables
    // both touch and gamepad modes in Badland 2.
    (void)id;
    (void)args;
    return 0;
}

static jint Main_JNI_openUrl(jmethodID id, va_list args) {
    // Cannot open a browser from the port: report failure (Main.java: 0).
    (void)id;
    (void)args;
    return 0;
}

static jobject Main_JNI_getModelName(jmethodID id, va_list args) {
    // Build.MODEL equivalent. The engine logs it (Badland2Activity.startup)
    // and passes it to nativeSocialPlatformChosen -- any non-NULL string works.
    (void)id;
    (void)args;
    return jni->NewStringUTF(&jni, "PS Vita");
}

static void Main_JNI_keyboardShow(jmethodID id, va_list args) {
    // Soft keyboard: no-op until Phase 6/7 (no text input path on Vita yet).
    (void)id;
    (void)args;
    l_debug("Main.JNI_keyboardShow: ignored (no soft keyboard).");
}

static void Main_JNI_keyboardHide(jmethodID id, va_list args) {
    (void)id;
    (void)args;
    l_debug("Main.JNI_keyboardHide: ignored (no soft keyboard).");
}

static jobject LoginHandler_JNI_getAndroidID(jmethodID id, va_list args) {
    (void)id;
    (void)args;
    return jni->NewStringUTF(&jni, "ad_psvita_user_id");
}

static jint FacebookManager_JNI_isLoggedIn(jmethodID id, va_list args) {
    (void)id;
    (void)args;
    return 0; // Not logged in
}

static void FacebookManager_JNI_startupLogin(jmethodID id, va_list args) {
    (void)id;
    (void)args;
    l_debug("FacebookManager.JNI_startupLogin: ignored.");
}

static jint FacebookManager_JNI_getLoginState(jmethodID id, va_list args) {
    (void)id;
    (void)args;
    return 0; // Disconnected / Not logged in
}

static jobject FacebookManager_JNI_getUserId(jmethodID id, va_list args) {
    (void)id;
    (void)args;
    return jni->NewStringUTF(&jni, "");
}

static jobject FacebookManager_JNI_getUserTokenString(jmethodID id, va_list args) {
    (void)id;
    (void)args;
    return jni->NewStringUTF(&jni, "");
}

static jobject FacebookManager_JNI_getUserName(jmethodID id, va_list args) {
    (void)id;
    (void)args;
    return jni->NewStringUTF(&jni, "");
}

static jobject FacebookManager_JNI_getUserFirstName(jmethodID id, va_list args) {
    (void)id;
    (void)args;
    return jni->NewStringUTF(&jni, "");
}

static jobject FacebookManager_JNI_getUserSurname(jmethodID id, va_list args) {
    (void)id;
    (void)args;
    return jni->NewStringUTF(&jni, "");
}

static void FacebookManager_JNI_loginWithFacebook(jmethodID id, va_list args) {
    (void)id;
    (void)args;
    l_debug("FacebookManager.JNI_loginWithFacebook: ignored.");
}

static void FacebookManager_JNI_logoutWithFacebook(jmethodID id, va_list args) {
    (void)id;
    (void)args;
    l_debug("FacebookManager.JNI_logoutWithFacebook: ignored.");
}

// --- org.fmod.FMOD statics (libfmod picks its output device with these) ---

static jboolean FMOD_checkInit(jmethodID id, va_list args) {
    // Java: gContext != null. Always true on a real device after FMOD.init().
    (void)id;
    (void)args;
    return JNI_TRUE;
}

static jboolean FMOD_supportsLowLatency(jmethodID id, va_list args) {
    // libfmod (0xcae94/0xcaebc): checkInit && supportsLowLatency -> OpenSL (16),
    // else AudioTrack (15). OpenSL is refused in dlopen_soloader, so TRUE here
    // made output init fail (FMOD error 51). FALSE -> org.fmod.AudioDevice bridge.
    // (Only a missing method ID returns FMOD_ERR_INITIALIZATION 28, not the value.)
    (void)id;
    (void)args;
    return JNI_FALSE;
}

static jint FMOD_getOutputSampleRate(jmethodID id, va_list args) {
    // Return 0 so libfmod chooses FMOD_OUTPUTTYPE_AUDIOTRACK (15),
    // which delegates audio playback to org.fmod.AudioDevice via SceAudioOut.
    (void)id;
    (void)args;
    return 0;
}

static jint FMOD_getOutputBlockSize(jmethodID id, va_list args) {
    // Return 0 so libfmod chooses FMOD_OUTPUTTYPE_AUDIOTRACK (15).
    (void)id;
    (void)args;
    return 0;
}

// --- org.fmod.AudioDevice implementation (FMOD AudioTrack output bridge) ---

#include <psp2/audioout.h>
#include <stdlib.h>
#include <string.h>

static int s_audio_port = -1;
static int s_audio_grain = 1024;
static int16_t *s_audio_fifo = NULL;
static int s_audio_fifo_len = 0; // in stereo frames (1 frame = 4 bytes)
static int s_audio_fifo_cap = 0; // in stereo frames

static jobject AudioDevice_ctor(jmethodID id, va_list args) {
    (void)id;
    (void)args;
    l_info("AudioDevice.<init>() called");
    return (jobject)1;
}

static jboolean AudioDevice_init(jmethodID id, va_list args) {
    (void)id;
    jint channels = va_arg(args, jint);
    jint rate = va_arg(args, jint);
    jint dsp_len = va_arg(args, jint);
    jint num_buffers = va_arg(args, jint);

    l_info("AudioDevice.init: channels=%d, rate=%d, dsp_len=%d, num_buffers=%d",
           channels, rate, dsp_len, num_buffers);

    if (s_audio_port >= 0) {
        sceAudioOutOutput(s_audio_port, NULL);
        sceAudioOutReleasePort(s_audio_port);
        s_audio_port = -1;
    }

    if (rate != 44100 && rate != 48000) {
        rate = 44100;
    }

    int grain = dsp_len;
    if (grain <= 0 || grain % 64 != 0 || grain > 65472) {
        grain = 1024;
    }
    s_audio_grain = grain;

    s_audio_port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM, grain, rate, SCE_AUDIO_OUT_MODE_STEREO);
    if (s_audio_port < 0) {
        l_error("AudioDevice.init: sceAudioOutOpenPort failed: 0x%08X", s_audio_port);
        return JNI_FALSE;
    }

    int vol[2] = { SCE_AUDIO_VOLUME_0DB, SCE_AUDIO_VOLUME_0DB };
    sceAudioOutSetVolume(s_audio_port, SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, vol);

    s_audio_fifo_cap = (grain * 4 > 8192) ? (grain * 4) : 8192;
    if (s_audio_fifo) free(s_audio_fifo);
    s_audio_fifo = calloc(s_audio_fifo_cap * 2, sizeof(int16_t));
    s_audio_fifo_len = 0;

    l_success("AudioDevice.init: SceAudioOut port %d ready (rate=%d, grain=%d, fifo_cap=%d)",
              s_audio_port, rate, s_audio_grain, s_audio_fifo_cap);
    return JNI_TRUE;
}

static void AudioDevice_write(jmethodID id, va_list args) {
    (void)id;
    jbyteArray bArr = va_arg(args, jbyteArray);
    jint size = va_arg(args, jint);

    if (s_audio_port < 0 || !bArr || size <= 0 || !s_audio_fifo) {
        return;
    }

    const int16_t *src = (const int16_t *)bArr;
    JavaDynArray *jda = (JavaDynArray *)bArr;
    if (jda && jda->magic == 0xB00B1E5) {
        src = (const int16_t *)jda->array;
    }

    if (!src) return;

    int in_frames = size / (2 * sizeof(int16_t)); // 4 bytes per frame (stereo 16-bit)
    if (in_frames <= 0) return;

    if (s_audio_fifo_len + in_frames > s_audio_fifo_cap) {
        s_audio_fifo_cap = s_audio_fifo_len + in_frames + s_audio_grain * 2;
        int16_t *new_fifo = realloc(s_audio_fifo, s_audio_fifo_cap * 2 * sizeof(int16_t));
        if (!new_fifo) {
            l_error("AudioDevice.write: FIFO realloc failed");
            return;
        }
        s_audio_fifo = new_fifo;
    }

    memcpy(s_audio_fifo + s_audio_fifo_len * 2, src, in_frames * 2 * sizeof(int16_t));
    s_audio_fifo_len += in_frames;

    while (s_audio_fifo_len >= s_audio_grain) {
        sceAudioOutOutput(s_audio_port, s_audio_fifo);
        s_audio_fifo_len -= s_audio_grain;
        if (s_audio_fifo_len > 0) {
            memmove(s_audio_fifo, s_audio_fifo + s_audio_grain * 2, s_audio_fifo_len * 2 * sizeof(int16_t));
        }
    }
}

static void AudioDevice_close(jmethodID id, va_list args) {
    (void)id;
    (void)args;
    l_info("AudioDevice.close: releasing audio port %d", s_audio_port);
    if (s_audio_port >= 0) {
        sceAudioOutOutput(s_audio_port, NULL);
        sceAudioOutReleasePort(s_audio_port);
        s_audio_port = -1;
    }
    if (s_audio_fifo) {
        free(s_audio_fifo);
        s_audio_fifo = NULL;
    }
    s_audio_fifo_len = 0;
    s_audio_fifo_cap = 0;
}

NameToMethodID nameToMethodId[] = {
    { 1000, "JNI_textAsBitmap", METHOD_TYPE_OBJECT },
    { 1001, "JNI_getLocalLanguage", METHOD_TYPE_INT },
    { 1002, "JNI_IsNetConnected", METHOD_TYPE_INT },
    { 1003, "JNI_isControllerOnly", METHOD_TYPE_INT },
    { 1004, "JNI_openUrl", METHOD_TYPE_INT },
    { 1005, "JNI_getModelName", METHOD_TYPE_OBJECT },
    { 1006, "JNI_keyboardShow", METHOD_TYPE_VOID },
    { 1007, "JNI_keyboardHide", METHOD_TYPE_VOID },
    { 1008, "JNI_getAndroidID", METHOD_TYPE_OBJECT },
    { 1009, "JNI_isLoggedIn", METHOD_TYPE_INT },
    { 1010, "JNI_startupLogin", METHOD_TYPE_VOID },
    { 1011, "checkInit", METHOD_TYPE_BOOLEAN },
    { 1012, "supportsLowLatency", METHOD_TYPE_BOOLEAN },
    { 1013, "getOutputSampleRate", METHOD_TYPE_INT },
    { 1014, "getOutputBlockSize", METHOD_TYPE_INT },
    { 1015, "JNI_getUserId", METHOD_TYPE_OBJECT },
    { 1016, "JNI_getUserTokenString", METHOD_TYPE_OBJECT },
    { 1017, "JNI_getUserName", METHOD_TYPE_OBJECT },
    { 1018, "JNI_getUserFirstName", METHOD_TYPE_OBJECT },
    { 1019, "JNI_getUserSurname", METHOD_TYPE_OBJECT },
    { 1020, "JNI_getLoginState", METHOD_TYPE_INT },
    { 1021, "JNI_loginWithFacebook", METHOD_TYPE_VOID },
    { 1022, "JNI_logoutWithFacebook", METHOD_TYPE_VOID },
    { 1023, "org/fmod/AudioDevice/<init>", METHOD_TYPE_OBJECT },
    { 1024, "AudioDevice/<init>", METHOD_TYPE_OBJECT },
    { 1025, "<init>", METHOD_TYPE_OBJECT },
    { 1026, "init", METHOD_TYPE_BOOLEAN },
    { 1027, "write", METHOD_TYPE_VOID },
    { 1028, "close", METHOD_TYPE_VOID },
};

MethodsBoolean methodsBoolean[] = {
    { 1011, FMOD_checkInit },
    { 1012, FMOD_supportsLowLatency },
    { 1026, AudioDevice_init },
};
MethodsByte methodsByte[] = {};
MethodsChar methodsChar[] = {};
MethodsDouble methodsDouble[] = {};
MethodsFloat methodsFloat[] = {};
MethodsInt methodsInt[] = {
    { 1001, Main_JNI_getLocalLanguage },
    { 1002, Main_JNI_IsNetConnected },
    { 1003, Main_JNI_isControllerOnly },
    { 1004, Main_JNI_openUrl },
    { 1009, FacebookManager_JNI_isLoggedIn },
    { 1013, FMOD_getOutputSampleRate },
    { 1014, FMOD_getOutputBlockSize },
    { 1020, FacebookManager_JNI_getLoginState },
};
MethodsLong methodsLong[] = {};
MethodsObject methodsObject[] = {
    { 1000, Main_JNI_textAsBitmap },
    { 1005, Main_JNI_getModelName },
    { 1008, LoginHandler_JNI_getAndroidID },
    { 1015, FacebookManager_JNI_getUserId },
    { 1016, FacebookManager_JNI_getUserTokenString },
    { 1017, FacebookManager_JNI_getUserName },
    { 1018, FacebookManager_JNI_getUserFirstName },
    { 1019, FacebookManager_JNI_getUserSurname },
    { 1023, AudioDevice_ctor },
    { 1024, AudioDevice_ctor },
    { 1025, AudioDevice_ctor },
};
MethodsShort methodsShort[] = {};
MethodsVoid methodsVoid[] = {
    { 1006, Main_JNI_keyboardShow },
    { 1007, Main_JNI_keyboardHide },
    { 1010, FacebookManager_JNI_startupLogin },
    { 1021, FacebookManager_JNI_loginWithFacebook },
    { 1022, FacebookManager_JNI_logoutWithFacebook },
    { 1027, AudioDevice_write },
    { 1028, AudioDevice_close },
};

/*
 * JNI Fields
*/

// System-wide constant that applications sometimes request
// https://developer.android.com/reference/android/content/Context.html#WINDOW_SERVICE
char WINDOW_SERVICE[] = "window";

// System-wide constant that's often used to determine Android version
// https://developer.android.com/reference/android/os/Build.VERSION.html#SDK_INT
// Possible values: https://developer.android.com/reference/android/os/Build.VERSION_CODES
const int SDK_INT = 19; // Android 4.4 / KitKat

NameToFieldID nameToFieldId[] = {
		{ 0, "WINDOW_SERVICE", FIELD_TYPE_OBJECT }, 
		{ 1, "SDK_INT", FIELD_TYPE_INT },
};

FieldsBoolean fieldsBoolean[] = {};
FieldsByte fieldsByte[] = {};
FieldsChar fieldsChar[] = {};
FieldsDouble fieldsDouble[] = {};
FieldsFloat fieldsFloat[] = {};
FieldsInt fieldsInt[] = {
		{ 1, SDK_INT },
};
FieldsObject fieldsObject[] = {
		{ 0, WINDOW_SERVICE },
};
FieldsLong fieldsLong[] = {};
FieldsShort fieldsShort[] = {};

__FALSOJNI_IMPL_CONTAINER_SIZES
