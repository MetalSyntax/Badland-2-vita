#include "reimpl/native_window.h"

#include "utils/logger.h"

struct ANativeWindow {
    int unused;
};

// Never dereferenced by our own code (eglCreateWindowSurface ignores its `win`
// argument) -- just a stable, non-null pointer the engine can hold.
ANativeWindow g_fake_native_window = {0};

int32_t ANativeWindow_getWidth(ANativeWindow *window) {
    (void)window;
    return 960;
}

int32_t ANativeWindow_getHeight(ANativeWindow *window) {
    (void)window;
    return 544;
}

int32_t ANativeWindow_setBuffersGeometry(ANativeWindow *window, int32_t width,
                                         int32_t height, int32_t format) {
    l_debug("ANativeWindow_setBuffersGeometry(%d, %d, %d)", width, height, format);
    (void)window;
    return 0;
}
