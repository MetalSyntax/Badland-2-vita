#include "reimpl/input_event.h"

int32_t AInputEvent_getType(AInputEvent *event) {
    return event ? event->type : 0;
}

int32_t AInputEvent_getSource(AInputEvent *event) {
    return event ? event->source : 0;
}

int32_t AInputEvent_getDeviceId(AInputEvent *event) {
    return event ? event->deviceId : 0;
}

int32_t AKeyEvent_getAction(AInputEvent *event) {
    return event ? event->action : 0;
}

int32_t AKeyEvent_getKeyCode(AInputEvent *event) {
    return event ? event->keyCode : 0;
}

int32_t AMotionEvent_getAction(AInputEvent *event) {
    return event ? event->action : 0;
}

float AMotionEvent_getAxisValue(AInputEvent *event, int32_t axis, size_t pointerIndex) {
    (void)pointerIndex;
    if (event && axis >= 0 && axis < 32)
        return event->axes[axis];
    return 0.0f;
}

size_t AMotionEvent_getPointerCount(AInputEvent *event) {
    return event ? event->pointerCount : 0;
}

int32_t AMotionEvent_getPointerId(AInputEvent *event, size_t pointerIndex) {
    if (event && pointerIndex < event->pointerCount)
        return event->pointers[pointerIndex].id;
    return 0;
}

float AMotionEvent_getX(AInputEvent *event, size_t pointerIndex) {
    if (event && pointerIndex < event->pointerCount)
        return event->pointers[pointerIndex].x;
    return 0.0f;
}

float AMotionEvent_getY(AInputEvent *event, size_t pointerIndex) {
    if (event && pointerIndex < event->pointerCount)
        return event->pointers[pointerIndex].y;
    return 0.0f;
}
