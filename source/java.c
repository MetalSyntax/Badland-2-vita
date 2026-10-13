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
    // Main.java returns 1 when the device has NO touchscreen. Touch input is
    // not wired yet (Phase 6), so 1 is the truthful answer today.
    (void)id;
    (void)args;
    return 1;
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

NameToMethodID nameToMethodId[] = {
    { 1000, "JNI_textAsBitmap", METHOD_TYPE_OBJECT },
    { 1001, "JNI_getLocalLanguage", METHOD_TYPE_INT },
    { 1002, "JNI_IsNetConnected", METHOD_TYPE_INT },
    { 1003, "JNI_isControllerOnly", METHOD_TYPE_INT },
    { 1004, "JNI_openUrl", METHOD_TYPE_INT },
    { 1005, "JNI_getModelName", METHOD_TYPE_OBJECT },
    { 1006, "JNI_keyboardShow", METHOD_TYPE_VOID },
    { 1007, "JNI_keyboardHide", METHOD_TYPE_VOID },
};

MethodsBoolean methodsBoolean[] = {};
MethodsByte methodsByte[] = {};
MethodsChar methodsChar[] = {};
MethodsDouble methodsDouble[] = {};
MethodsFloat methodsFloat[] = {};
MethodsInt methodsInt[] = {
    { 1001, Main_JNI_getLocalLanguage },
    { 1002, Main_JNI_IsNetConnected },
    { 1003, Main_JNI_isControllerOnly },
    { 1004, Main_JNI_openUrl },
};
MethodsLong methodsLong[] = {};
MethodsObject methodsObject[] = {
    { 1000, Main_JNI_textAsBitmap },
    { 1005, Main_JNI_getModelName },
};
MethodsShort methodsShort[] = {};
MethodsVoid methodsVoid[] = {
    { 1006, Main_JNI_keyboardShow },
    { 1007, Main_JNI_keyboardHide },
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
