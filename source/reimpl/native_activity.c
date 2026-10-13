#include "reimpl/native_activity.h"

#include "utils/logger.h"

#include <falso_jni/FalsoJNI.h>
#include <so_util/so_util.h>

#include <string.h>

static ANativeActivity g_activity;
static ANativeActivityCallbacks g_callbacks;

void native_activity_bootstrap(so_module *mod) {
    int (*ANativeActivity_onCreate_fn)(ANativeActivity *, void *, size_t) =
        (void *) so_symbol(mod, "ANativeActivity_onCreate");

    if (!ANativeActivity_onCreate_fn) {
        l_fatal("ANativeActivity_onCreate not found in the loaded .so.");
        return;
    }

    memset(&g_activity, 0, sizeof(g_activity));
    memset(&g_callbacks, 0, sizeof(g_callbacks));

    g_activity.callbacks = &g_callbacks; // allocated by framework (us)
    g_activity.vm = &jvm;
    g_activity.env = jni;
    g_activity.clazz = NULL;
    g_activity.internalDataPath = DATA_PATH;
    g_activity.externalDataPath = DATA_PATH;
    g_activity.sdkVersion = 19; // matches SDK_INT in source/java.c (KitKat)
    g_activity.instance = NULL;
    g_activity.configuration = AConfiguration_new();
    g_activity.assetManager = AAssetManager_create();
    g_activity.obbPath = DATA_PATH;

    l_success("Calling ANativeActivity_onCreate...");
    ANativeActivity_onCreate_fn(&g_activity, NULL, 0);

    if (!g_activity.callbacks) {
        l_fatal("ANativeActivity_onCreate returned without setting activity->callbacks.");
        return;
    }
    l_success("ANativeActivity_onCreate returned, callbacks @ 0x%x.", (int) g_activity.callbacks);

    // Mirrors the real Android Activity lifecycle: onCreate -> onStart ->
    // onResume, then the window/focus plumbing. We never call
    // onInputQueueCreated -- there is no real input queue yet (see
    // reimpl/input_queue.c) -- so the glue treats this as "no input device
    // attached" for now (Phase 6/7 will wire real input).
    if (g_activity.callbacks->onStart) {
        l_success("Calling onStart...");
        g_activity.callbacks->onStart(&g_activity);
    }

    if (g_activity.callbacks->onResume) {
        l_success("Calling onResume...");
        g_activity.callbacks->onResume(&g_activity);
    }

    if (g_activity.callbacks->onWindowFocusChanged) {
        l_success("Calling onWindowFocusChanged...");
        g_activity.callbacks->onWindowFocusChanged(&g_activity, 1);
    }

    if (g_activity.callbacks->onNativeWindowCreated) {
        l_success("Calling onNativeWindowCreated...");
        g_activity.callbacks->onNativeWindowCreated(&g_activity, &g_fake_native_window);
    }

    l_success("native_activity_bootstrap done.");
}
