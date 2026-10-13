#include "reimpl/bitmap.h"

#include "utils/logger.h"

#include <stddef.h>

int AndroidBitmap_getInfo(JNIEnv *env, jobject bitmap, AndroidBitmapInfo *info) {
    l_error("AndroidBitmap_getInfo: no bitmaps implemented yet (textAsBitmap pending).");
    (void)env;
    (void)bitmap;
    (void)info;
    return ANDROID_BITMAP_RESULT_BAD_PARAMETER;
}

int AndroidBitmap_lockPixels(JNIEnv *env, jobject bitmap, void **addrPtr) {
    l_error("AndroidBitmap_lockPixels: no bitmaps implemented yet (textAsBitmap pending).");
    (void)env;
    (void)bitmap;
    if (addrPtr)
        *addrPtr = NULL;
    return ANDROID_BITMAP_RESULT_BAD_PARAMETER;
}

int AndroidBitmap_unlockPixels(JNIEnv *env, jobject bitmap) {
    (void)env;
    (void)bitmap;
    return ANDROID_BITMAP_RESULT_BAD_PARAMETER;
}
