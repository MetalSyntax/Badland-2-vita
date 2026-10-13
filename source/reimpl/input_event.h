/**
 * @file  input_event.h
 * @brief Minimal reimplementation of android/input.h's event-accessor side.
 *        All UND imports of libbadland2.so in this family (AInputEvent_getType/
 *        getSource, AKeyEvent_getAction/getKeyCode, AMotionEvent_getAction/
 *        getAxisValue/getPointerCount/getPointerId/getX/getY) return neutral
 *        zeroes: no input device is attached yet (Phase 6/7), so "no keys, no
 *        touches" is the truthful answer, not just a convenient stub.
 */

#ifndef ANDROID_INPUT_EVENT_H
#define ANDROID_INPUT_EVENT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct AInputEvent;
typedef struct AInputEvent AInputEvent;

int32_t AInputEvent_getType(AInputEvent *event);
int32_t AInputEvent_getSource(AInputEvent *event);
int32_t AInputEvent_getDeviceId(AInputEvent *event);

int32_t AKeyEvent_getAction(AInputEvent *event);
int32_t AKeyEvent_getKeyCode(AInputEvent *event);

int32_t AMotionEvent_getAction(AInputEvent *event);
float AMotionEvent_getAxisValue(AInputEvent *event, int32_t axis, size_t pointerIndex);
size_t AMotionEvent_getPointerCount(AInputEvent *event);
int32_t AMotionEvent_getPointerId(AInputEvent *event, size_t pointerIndex);
float AMotionEvent_getX(AInputEvent *event, size_t pointerIndex);
float AMotionEvent_getY(AInputEvent *event, size_t pointerIndex);

#ifdef __cplusplus
};
#endif

#endif // ANDROID_INPUT_EVENT_H
