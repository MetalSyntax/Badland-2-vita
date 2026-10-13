/**
 * @file  bitmap.h
 * @brief Minimal reimplementation of android/bitmap.h (jnigraphics).
 *        libbadland2.so UND-imports AndroidBitmap_getInfo/lockPixels/
 *        unlockPixels (used on the Bitmap returned by Main.JNI_textAsBitmap).
 *        No bitmaps exist yet -- the textAsBitmap FalsoJNI stub is pending
 *        (Phase 5, see text_rendering reference) -- so these report failure
 *        instead of fabricating pixels. Revisit together with that stub; do
 *        NOT trust a NULL bitmap on the native side until the wrapper's
 *        NULL-handling is verified (so-crash-triage §8).
 */

#ifndef ANDROID_BITMAP_H
#define ANDROID_BITMAP_H

#include <stdint.h>

#include <falso_jni/jni.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    ANDROID_BITMAP_RESULT_SUCCESS = 0,
    ANDROID_BITMAP_RESULT_BAD_PARAMETER = -1,
    ANDROID_BITMAP_RESULT_JNI_EXCEPTION = -2,
    ANDROID_BITMAP_RESULT_ALLOCATION_FAILED = -3,
};

enum {
    ANDROID_BITMAP_FORMAT_NONE = 0,
    ANDROID_BITMAP_FORMAT_RGBA_8888 = 1,
    ANDROID_BITMAP_FORMAT_RGB_565 = 4,
    ANDROID_BITMAP_FORMAT_RGBA_4444 = 7,
    ANDROID_BITMAP_FORMAT_A_8 = 8,
};

typedef struct AndroidBitmapInfo {
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    int32_t format;
    uint32_t flags;
} AndroidBitmapInfo;

int AndroidBitmap_getInfo(JNIEnv *env, jobject bitmap, AndroidBitmapInfo *info);
int AndroidBitmap_lockPixels(JNIEnv *env, jobject bitmap, void **addrPtr);
int AndroidBitmap_unlockPixels(JNIEnv *env, jobject bitmap);

#ifdef __cplusplus
};
#endif

#endif // ANDROID_BITMAP_H
