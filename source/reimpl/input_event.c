#include "reimpl/input_event.h"

#include <stddef.h>

int32_t AInputEvent_getType(AInputEvent *event) {
    (void)event;
    return 0;
}

int32_t AInputEvent_getSource(AInputEvent *event) {
    (void)event;
    return 0;
}

int32_t AInputEvent_getDeviceId(AInputEvent *event) {
    (void)event;
    return 0;
}

int32_t AKeyEvent_getAction(AInputEvent *event) {
    (void)event;
    return 0;
}

int32_t AKeyEvent_getKeyCode(AInputEvent *event) {
    (void)event;
    return 0;
}

int32_t AMotionEvent_getAction(AInputEvent *event) {
    (void)event;
    return 0;
}

float AMotionEvent_getAxisValue(AInputEvent *event, int32_t axis, size_t pointerIndex) {
    (void)event;
    (void)axis;
    (void)pointerIndex;
    return 0.0f;
}

size_t AMotionEvent_getPointerCount(AInputEvent *event) {
    (void)event;
    return 0;
}

int32_t AMotionEvent_getPointerId(AInputEvent *event, size_t pointerIndex) {
    (void)event;
    (void)pointerIndex;
    return 0;
}

float AMotionEvent_getX(AInputEvent *event, size_t pointerIndex) {
    (void)event;
    (void)pointerIndex;
    return 0.0f;
}

float AMotionEvent_getY(AInputEvent *event, size_t pointerIndex) {
    (void)event;
    (void)pointerIndex;
    return 0.0f;
}
