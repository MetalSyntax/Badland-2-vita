/**
 * @file  input_event.h
 * @brief Android NDK input events reimplementation for PS Vita.
 */

#ifndef ANDROID_INPUT_EVENT_H
#define ANDROID_INPUT_EVENT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    AINPUT_EVENT_TYPE_KEY    = 1,
    AINPUT_EVENT_TYPE_MOTION = 2,
};

enum {
    AINPUT_SOURCE_KEYBOARD    = 0x00000101,
    AINPUT_SOURCE_TOUCHSCREEN = 0x00001002,
    AINPUT_SOURCE_GAMEPAD     = 0x00000401,
    AINPUT_SOURCE_JOYSTICK    = 0x01000010,
};

enum {
    AKEY_EVENT_ACTION_DOWN = 0,
    AKEY_EVENT_ACTION_UP   = 1,
};

enum {
    AMOTION_EVENT_ACTION_DOWN             = 0,
    AMOTION_EVENT_ACTION_UP               = 1,
    AMOTION_EVENT_ACTION_MOVE             = 2,
    AMOTION_EVENT_ACTION_CANCEL           = 3,
    AMOTION_EVENT_ACTION_OUTSIDE          = 4,
    AMOTION_EVENT_ACTION_POINTER_DOWN     = 5,
    AMOTION_EVENT_ACTION_POINTER_UP       = 6,
    AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT = 8,
};

enum {
    AKEYCODE_UNKNOWN       = 0,
    AKEYCODE_BACK          = 4,
    AKEYCODE_DPAD_UP       = 19,
    AKEYCODE_DPAD_DOWN     = 20,
    AKEYCODE_DPAD_LEFT     = 21,
    AKEYCODE_DPAD_RIGHT    = 22,
    AKEYCODE_DPAD_CENTER   = 23,
    AKEYCODE_MENU          = 82,
    AKEYCODE_BUTTON_A      = 96,
    AKEYCODE_BUTTON_B      = 97,
    AKEYCODE_BUTTON_X      = 99,
    AKEYCODE_BUTTON_Y      = 100,
    AKEYCODE_BUTTON_L1     = 102,
    AKEYCODE_BUTTON_R1     = 103,
    AKEYCODE_BUTTON_START  = 108,
    AKEYCODE_BUTTON_SELECT = 109,
};

enum {
    AMOTION_EVENT_AXIS_X        = 0,
    AMOTION_EVENT_AXIS_Y        = 1,
    AMOTION_EVENT_AXIS_PRESSURE = 2,
    AMOTION_EVENT_AXIS_SIZE     = 3,
    AMOTION_EVENT_AXIS_TOUCH_MAJOR = 4,
    AMOTION_EVENT_AXIS_TOUCH_MINOR = 5,
    AMOTION_EVENT_AXIS_Z        = 11,
    AMOTION_EVENT_AXIS_RZ       = 14,
    AMOTION_EVENT_AXIS_GAS      = 22,
    AMOTION_EVENT_AXIS_BRAKE    = 23,
};

struct AInputEvent {
    int32_t type;
    int32_t deviceId;
    int32_t source;
    int32_t action;
    int32_t keyCode;
    int32_t scanCode;
    int32_t metaState;
    int32_t repeatCount;
    int64_t downTime;
    int64_t eventTime;
    size_t pointerCount;
    struct {
        int32_t id;
        float x;
        float y;
    } pointers[8];
    float axes[32];
};

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
