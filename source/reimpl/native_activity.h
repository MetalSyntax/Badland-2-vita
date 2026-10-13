/**
 * @file  native_activity.h
 * @brief Minimal reimplementation of android/native_activity.h. This is the
 *        struct/callback ABI the real libbadland2.so's exported
 *        ANativeActivity_onCreate() expects -- confirmed via nm/objdump
 *        (ANativeActivity_onCreate @ T, android_main @ T: android_native_app_glue
 *        is statically linked into the .so). This is the standard, stable NDK
 *        ABI -- not engine-specific -- so the bootstrap pattern proven in a
 *        sibling port (Aralon) applies here; only sdkVersion/paths are
 *        Badland-2-specific (SDK_INT 19 per source/java.c, DATA_PATH).
 */

#ifndef ANDROID_NATIVE_ACTIVITY_H
#define ANDROID_NATIVE_ACTIVITY_H

#include <stddef.h>
#include <stdint.h>

#include <falso_jni/jni.h>

#include "reimpl/asset_manager.h"
#include "reimpl/configuration.h"
#include "reimpl/input_queue.h"
#include "reimpl/native_window.h"
#include "reimpl/rect.h"

#include <so_util/so_util.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ANativeActivity;

typedef struct ANativeActivityCallbacks {
    void (*onStart)(struct ANativeActivity *activity);
    void (*onResume)(struct ANativeActivity *activity);
    void *(*onSaveInstanceState)(struct ANativeActivity *activity, size_t *outLen);
    void (*onPause)(struct ANativeActivity *activity);
    void (*onStop)(struct ANativeActivity *activity);
    void (*onDestroy)(struct ANativeActivity *activity);
    void (*onWindowFocusChanged)(struct ANativeActivity *activity, int hasFocus);
    void (*onNativeWindowCreated)(struct ANativeActivity *activity, ANativeWindow *window);
    void (*onNativeWindowResized)(struct ANativeActivity *activity, ANativeWindow *window);
    void (*onNativeWindowRedrawNeeded)(struct ANativeActivity *activity, ANativeWindow *window);
    void (*onNativeWindowDestroyed)(struct ANativeActivity *activity, ANativeWindow *window);
    void (*onInputQueueCreated)(struct ANativeActivity *activity, AInputQueue *queue);
    void (*onInputQueueDestroyed)(struct ANativeActivity *activity, AInputQueue *queue);
    void (*onContentRectChanged)(struct ANativeActivity *activity, const ARect *rect);
    void (*onConfigurationChanged)(struct ANativeActivity *activity);
    void (*onLowMemory)(struct ANativeActivity *activity);
} ANativeActivityCallbacks;

typedef struct ANativeActivity {
    ANativeActivityCallbacks *callbacks;
    JavaVM *vm;
    JNIEnv env;
    jobject clazz;
    const char *internalDataPath;
    const char *externalDataPath;
    int32_t sdkVersion;
    void *instance;
    AConfiguration *configuration;
    AAssetManager *assetManager;
    const char *obbPath;
} ANativeActivity;

// Builds the fake ANativeActivity, calls the real ANativeActivity_onCreate()
// exported by libbadland2.so, and drives the callbacks it fills in through
// onStart/onResume/onWindowFocusChanged/onNativeWindowCreated.
// Unverified past this point until tested against a real device log.
void native_activity_bootstrap(so_module *mod);

#ifdef __cplusplus
};
#endif

#endif // ANDROID_NATIVE_ACTIVITY_H
