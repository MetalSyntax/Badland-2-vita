/**
 * @file  native_window.h
 * @brief Minimal reimplementation of android/native_window.h. Our EGL layer
 *        (reimpl/egl.c) never reads the ANativeWindow* it is handed --
 *        eglCreateWindowSurface() ignores its `win` argument -- so this only
 *        needs to satisfy the .so's UND imports (getWidth/getHeight/
 *        setBuffersGeometry per `objdump -T`) without crashing.
 */

#ifndef ANDROID_NATIVE_WINDOW_H
#define ANDROID_NATIVE_WINDOW_H

#include <stdint.h>

#include "reimpl/rect.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ANativeWindow;
typedef struct ANativeWindow ANativeWindow;

// Global fake window handed to the engine via
// ANativeActivityCallbacks::onNativeWindowCreated.
extern ANativeWindow g_fake_native_window;

int32_t ANativeWindow_getWidth(ANativeWindow *window);
int32_t ANativeWindow_getHeight(ANativeWindow *window);
int32_t ANativeWindow_setBuffersGeometry(ANativeWindow *window, int32_t width,
                                         int32_t height, int32_t format);

#ifdef __cplusplus
};
#endif

#endif // ANDROID_NATIVE_WINDOW_H
