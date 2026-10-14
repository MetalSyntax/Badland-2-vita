/**
 * @file  input_queue.c
 * @brief Thread-safe AInputQueue implementation with PS Vita touch & gamepad polling.
 */

#include "reimpl/input_queue.h"
#include "reimpl/input_event.h"
#include "utils/logger.h"

#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/kernel/clib.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define QUEUE_CAPACITY 64

#define SCREEN_W 960.0f
#define SCREEN_H 544.0f
#define VITA_TOUCH_W 1920.0f
#define VITA_TOUCH_H 1088.0f

struct AInputQueue {
    pthread_mutex_t lock;
    AInputEvent events[QUEUE_CAPACITY];
    int head;
    int tail;
    int count;
    ALooper *looper;
};

static AInputQueue g_default_input_queue;
static volatile int g_input_queue_inited = 0;

// State tracking across frames
static SceTouchData s_prev_touch;
static uint32_t s_prev_buttons = 0;
static int s_prev_virt_left = 0;
static int s_prev_virt_right = 0;

static const struct {
    uint32_t mask;
    int32_t key;
} s_btn_map[] = {
    { SCE_CTRL_CROSS,    AKEYCODE_BUTTON_A },
    { SCE_CTRL_CIRCLE,   AKEYCODE_BUTTON_B },
    { SCE_CTRL_SQUARE,   AKEYCODE_BUTTON_X },
    { SCE_CTRL_TRIANGLE, AKEYCODE_BUTTON_Y },
    { SCE_CTRL_UP,       AKEYCODE_DPAD_UP },
    { SCE_CTRL_DOWN,     AKEYCODE_DPAD_DOWN },
    { SCE_CTRL_LEFT,     AKEYCODE_DPAD_LEFT },
    { SCE_CTRL_RIGHT,    AKEYCODE_DPAD_RIGHT },
    { SCE_CTRL_LTRIGGER, AKEYCODE_BUTTON_L1 },
    { SCE_CTRL_RTRIGGER, AKEYCODE_BUTTON_R1 },
    { SCE_CTRL_START,    AKEYCODE_BUTTON_START },
    { SCE_CTRL_SELECT,   AKEYCODE_BACK },
};
#define NUM_BUTTONS (sizeof(s_btn_map) / sizeof(s_btn_map[0]))

static void ensure_input_queue_inited(AInputQueue *q) {
    if (!g_input_queue_inited) {
        pthread_mutex_init(&q->lock, NULL);
        q->head = 0;
        q->tail = 0;
        q->count = 0;
        q->looper = NULL;
        sceClibMemset(&s_prev_touch, 0, sizeof(s_prev_touch));
        g_input_queue_inited = 1;
    }
}

static void queue_push_locked(AInputQueue *q, const AInputEvent *ev) {
    if (q->count >= QUEUE_CAPACITY) {
        // Drop oldest event if queue is full
        q->head = (q->head + 1) % QUEUE_CAPACITY;
        q->count--;
    }
    q->events[q->tail] = *ev;
    q->tail = (q->tail + 1) % QUEUE_CAPACITY;
    q->count++;
}

AInputQueue *input_queue_create(void) {
    ensure_input_queue_inited(&g_default_input_queue);
    return &g_default_input_queue;
}

static void input_queue_poll(AInputQueue *q) {
    SceTouchData touch;
    SceCtrlData pad;

    sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
    sceCtrlPeekBufferPositive(0, &pad, 1);

    // 1. Physical Gamepad Buttons -> AKeyEvent
    uint32_t cur_buttons = pad.buttons;
    for (size_t i = 0; i < NUM_BUTTONS; i++) {
        uint32_t mask = s_btn_map[i].mask;
        int32_t key = s_btn_map[i].key;

        if ((cur_buttons & mask) && !(s_prev_buttons & mask)) {
            AInputEvent ev;
            sceClibMemset(&ev, 0, sizeof(ev));
            ev.type = AINPUT_EVENT_TYPE_KEY;
            ev.source = AINPUT_SOURCE_GAMEPAD;
            ev.action = AKEY_EVENT_ACTION_DOWN;
            ev.keyCode = key;
            queue_push_locked(q, &ev);
        } else if (!(cur_buttons & mask) && (s_prev_buttons & mask)) {
            AInputEvent ev;
            sceClibMemset(&ev, 0, sizeof(ev));
            ev.type = AINPUT_EVENT_TYPE_KEY;
            ev.source = AINPUT_SOURCE_GAMEPAD;
            ev.action = AKEY_EVENT_ACTION_UP;
            ev.keyCode = key;
            queue_push_locked(q, &ev);
        }
    }
    s_prev_buttons = cur_buttons;

    // 2. Front Touchscreen -> AMotionEvent
    if (touch.reportNum > 0) {
        // Cancel any active virtual flap touches so real screen touches take priority
        s_prev_virt_left = 0;
        s_prev_virt_right = 0;

        AInputEvent ev;
        sceClibMemset(&ev, 0, sizeof(ev));
        ev.type = AINPUT_EVENT_TYPE_MOTION;
        ev.source = AINPUT_SOURCE_TOUCHSCREEN;
        ev.pointerCount = touch.reportNum > 8 ? 8 : touch.reportNum;

        for (size_t i = 0; i < ev.pointerCount; i++) {
            ev.pointers[i].id = touch.report[i].id;
            float x = (float)touch.report[i].x * SCREEN_W / VITA_TOUCH_W;
            float y = (float)touch.report[i].y * SCREEN_H / VITA_TOUCH_H;
            if (x < 0.0f) x = 0.0f;
            if (x >= SCREEN_W) x = SCREEN_W - 1.0f;
            if (y < 0.0f) y = 0.0f;
            if (y >= SCREEN_H) y = SCREEN_H - 1.0f;
            ev.pointers[i].x = x;
            ev.pointers[i].y = y;
        }

        if (s_prev_touch.reportNum == 0) {
            ev.action = AMOTION_EVENT_ACTION_DOWN;
            queue_push_locked(q, &ev);
        } else {
            ev.action = AMOTION_EVENT_ACTION_MOVE;
            queue_push_locked(q, &ev);
        }
    } else if (s_prev_touch.reportNum > 0) {
        // Finger lifted
        AInputEvent ev;
        sceClibMemset(&ev, 0, sizeof(ev));
        ev.type = AINPUT_EVENT_TYPE_MOTION;
        ev.source = AINPUT_SOURCE_TOUCHSCREEN;
        ev.action = AMOTION_EVENT_ACTION_UP;
        ev.pointerCount = 1;
        ev.pointers[0].id = s_prev_touch.report[0].id;
        float x = (float)s_prev_touch.report[0].x * SCREEN_W / VITA_TOUCH_W;
        float y = (float)s_prev_touch.report[0].y * SCREEN_H / VITA_TOUCH_H;
        ev.pointers[0].x = x;
        ev.pointers[0].y = y;
        queue_push_locked(q, &ev);
    } else {
        // 3. Virtual Flap Mapping (when not using touch screen)
        // Badland 2 gameplay:
        // Left side (< 480): Flap Left
        // Right side (>= 480): Flap Right
        int virt_left = (cur_buttons & (SCE_CTRL_LTRIGGER | SCE_CTRL_LEFT)) || (pad.lx < 80);
        int virt_right = (cur_buttons & (SCE_CTRL_RTRIGGER | SCE_CTRL_RIGHT)) || (pad.lx > 175);
        if (cur_buttons & (SCE_CTRL_CROSS | SCE_CTRL_UP)) {
            virt_left = 1;
            virt_right = 1;
        }

        // Left flap transitions
        if (virt_left && !s_prev_virt_left) {
            AInputEvent ev;
            sceClibMemset(&ev, 0, sizeof(ev));
            ev.type = AINPUT_EVENT_TYPE_MOTION;
            ev.source = AINPUT_SOURCE_TOUCHSCREEN;
            ev.action = AMOTION_EVENT_ACTION_DOWN;
            ev.pointerCount = 1;
            ev.pointers[0].id = 6;
            ev.pointers[0].x = 240.0f;
            ev.pointers[0].y = 272.0f;
            queue_push_locked(q, &ev);
        } else if (!virt_left && s_prev_virt_left) {
            AInputEvent ev;
            sceClibMemset(&ev, 0, sizeof(ev));
            ev.type = AINPUT_EVENT_TYPE_MOTION;
            ev.source = AINPUT_SOURCE_TOUCHSCREEN;
            ev.action = AMOTION_EVENT_ACTION_UP;
            ev.pointerCount = 1;
            ev.pointers[0].id = 6;
            ev.pointers[0].x = 240.0f;
            ev.pointers[0].y = 272.0f;
            queue_push_locked(q, &ev);
        }

        // Right flap transitions
        if (virt_right && !s_prev_virt_right) {
            AInputEvent ev;
            sceClibMemset(&ev, 0, sizeof(ev));
            ev.type = AINPUT_EVENT_TYPE_MOTION;
            ev.source = AINPUT_SOURCE_TOUCHSCREEN;
            ev.action = virt_left ? (AMOTION_EVENT_ACTION_POINTER_DOWN | (1 << 8)) : AMOTION_EVENT_ACTION_DOWN;
            ev.pointerCount = virt_left ? 2 : 1;
            if (virt_left) {
                ev.pointers[0].id = 6;
                ev.pointers[0].x = 240.0f;
                ev.pointers[0].y = 272.0f;
                ev.pointers[1].id = 7;
                ev.pointers[1].x = 720.0f;
                ev.pointers[1].y = 272.0f;
            } else {
                ev.pointers[0].id = 7;
                ev.pointers[0].x = 720.0f;
                ev.pointers[0].y = 272.0f;
            }
            queue_push_locked(q, &ev);
        } else if (!virt_right && s_prev_virt_right) {
            AInputEvent ev;
            sceClibMemset(&ev, 0, sizeof(ev));
            ev.type = AINPUT_EVENT_TYPE_MOTION;
            ev.source = AINPUT_SOURCE_TOUCHSCREEN;
            ev.action = virt_left ? (AMOTION_EVENT_ACTION_POINTER_UP | (1 << 8)) : AMOTION_EVENT_ACTION_UP;
            ev.pointerCount = virt_left ? 2 : 1;
            if (virt_left) {
                ev.pointers[0].id = 6;
                ev.pointers[0].x = 240.0f;
                ev.pointers[0].y = 272.0f;
                ev.pointers[1].id = 7;
                ev.pointers[1].x = 720.0f;
                ev.pointers[1].y = 272.0f;
            } else {
                ev.pointers[0].id = 7;
                ev.pointers[0].x = 720.0f;
                ev.pointers[0].y = 272.0f;
            }
            queue_push_locked(q, &ev);
        }

        s_prev_virt_left = virt_left;
        s_prev_virt_right = virt_right;
    }

    s_prev_touch = touch;
}

int input_queue_has_events(void *queue) {
    AInputQueue *q = (AInputQueue *)queue;
    if (!q) return 0;
    ensure_input_queue_inited(q);

    pthread_mutex_lock(&q->lock);
    if (q->count == 0) {
        input_queue_poll(q);
    }
    int has = q->count > 0;
    pthread_mutex_unlock(&q->lock);
    return has;
}

void AInputQueue_attachLooper(AInputQueue *queue, ALooper *looper, int ident,
                              ALooper_callbackFunc callback,
                              void *data) {
    (void)callback;
    if (!queue) return;
    ensure_input_queue_inited(queue);
    queue->looper = looper;
    looper_attach_source(looper, LOOPER_SRC_INPUT, queue, ident, data);
}

void AInputQueue_detachLooper(AInputQueue *queue) {
    if (!queue) return;
    ensure_input_queue_inited(queue);
    if (queue->looper) {
        looper_detach_source(queue->looper, queue);
        queue->looper = NULL;
    }
}

int32_t AInputQueue_hasEvents(AInputQueue *queue) {
    return input_queue_has_events(queue) ? 1 : 0;
}

int32_t AInputQueue_getEvent(AInputQueue *queue, AInputEvent **outEvent) {
    if (!queue) return -1;
    ensure_input_queue_inited(queue);

    pthread_mutex_lock(&queue->lock);
    if (queue->count == 0) {
        input_queue_poll(queue);
    }

    if (queue->count == 0) {
        pthread_mutex_unlock(&queue->lock);
        if (outEvent) *outEvent = NULL;
        return -1;
    }

    int idx = queue->head;
    queue->head = (queue->head + 1) % QUEUE_CAPACITY;
    queue->count--;

    if (outEvent) {
        *outEvent = &queue->events[idx];
    }
    pthread_mutex_unlock(&queue->lock);
    return 0;
}

int32_t AInputQueue_preDispatchEvent(AInputQueue *queue, AInputEvent *event) {
    (void)queue;
    (void)event;
    return 0;
}

void AInputQueue_finishEvent(AInputQueue *queue, AInputEvent *event, int handled) {
    (void)queue;
    (void)event;
    (void)handled;
}
