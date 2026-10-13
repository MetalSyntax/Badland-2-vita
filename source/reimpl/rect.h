#ifndef ANDROID_RECT_H
#define ANDROID_RECT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ARect {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} ARect;

#ifdef __cplusplus
};
#endif

#endif // ANDROID_RECT_H
