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
#include <psp2/kernel/processmgr.h>

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
#define TOUCH_SLOTS 5
static int s_slot_id[TOUCH_SLOTS] = { -1, -1, -1, -1, -1 }; // Vita report id per game pointer id
static float s_slot_x[TOUCH_SLOTS], s_slot_y[TOUCH_SLOTS];
static uint32_t s_prev_buttons = 0;
// Negative slot owners for the simulated flap fingers (Vita report ids are >= 0)
#define VIRT_LEFT_ID  -2
#define VIRT_RIGHT_ID -3
#define FLAP_Y 500.0f // near the bottom edge, away from centred menu buttons

static const struct {
    uint32_t mask;
    int32_t key;
} s_btn_map[] = {
    { SCE_CTRL_CROSS,    AKEYCODE_BUTTON_A },
    { SCE_CTRL_SQUARE,   AKEYCODE_BUTTON_X },
    { SCE_CTRL_TRIANGLE, AKEYCODE_BUTTON_Y },
    { SCE_CTRL_UP,       AKEYCODE_DPAD_UP },
    { SCE_CTRL_DOWN,     AKEYCODE_DPAD_DOWN },
    { SCE_CTRL_LEFT,     AKEYCODE_DPAD_LEFT },
    { SCE_CTRL_RIGHT,    AKEYCODE_DPAD_RIGHT },
    { SCE_CTRL_LTRIGGER, AKEYCODE_BUTTON_L1 },
    { SCE_CTRL_RTRIGGER, AKEYCODE_BUTTON_R1 },
    // Badland 2 only opens the pause menu on BUTTON_B (NvGamepad bit 0x2000);
    // BUTTON_START (bit 0x10) reaches the game but nothing reacts to it.
    { SCE_CTRL_START,    AKEYCODE_BUTTON_B },
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

static void push_touch(AInputQueue *q, int32_t action, int id, float x, float y) {
    AInputEvent ev;
    sceClibMemset(&ev, 0, sizeof(ev));
    ev.type = AINPUT_EVENT_TYPE_MOTION;
    ev.source = AINPUT_SOURCE_TOUCHSCREEN;
    ev.action = action;
    ev.pointerCount = 1;
    ev.pointers[0].id = id;
    ev.pointers[0].x = x;
    ev.pointers[0].y = y;
    queue_push_locked(q, &ev);
}

static void sim_flap(AInputQueue *q, int owner, int down, float x) {
    int s = 0;
    while (s < TOUCH_SLOTS && s_slot_id[s] != owner) s++;
    if (down && s == TOUCH_SLOTS) {
        s = 0;
        while (s < TOUCH_SLOTS && s_slot_id[s] != -1) s++;
        if (s == TOUCH_SLOTS) return;
        s_slot_id[s] = owner;
        push_touch(q, AMOTION_EVENT_ACTION_DOWN, s, x, FLAP_Y);
    } else if (!down && s < TOUCH_SLOTS) {
        s_slot_id[s] = -1;
        push_touch(q, AMOTION_EVENT_ACTION_UP, s, x, FLAP_Y);
    }
}

AInputQueue *input_queue_create(void) {
    ensure_input_queue_inited(&g_default_input_queue);
    return &g_default_input_queue;
}

static void input_queue_poll(AInputQueue *q) {
    SceTouchData touch;
    SceCtrlData pad;

    // Sample hardware at most once per frame: a held finger emits MOVE on every
    // poll, so the game's "drain until empty" input loop would never end.
    static SceUInt64 s_last_poll = 0;
    SceUInt64 now = sceKernelGetProcessTimeWide();
    if (now - s_last_poll < 8000)
        return;
    s_last_poll = now;

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

    // 2. Front Touchscreen -> AMotionEvent.
    // The game keeps a 5-entry table indexed by pointer id and ignores ids > 4,
    // but Vita report ids grow with every new touch: map each finger to a slot.
    // Every finger gets its own DOWN/UP event; MOVE only when something moved.
    for (int s = 0; s < TOUCH_SLOTS; s++) {
        if (s_slot_id[s] < 0) continue;
        int still_down = 0;
        for (int i = 0; i < touch.reportNum; i++)
            still_down |= touch.report[i].id == s_slot_id[s];
        if (!still_down) {
            push_touch(q, AMOTION_EVENT_ACTION_UP, s, s_slot_x[s], s_slot_y[s]);
            s_slot_id[s] = -1;
        }
    }

    AInputEvent move;
    sceClibMemset(&move, 0, sizeof(move));
    move.type = AINPUT_EVENT_TYPE_MOTION;
    move.source = AINPUT_SOURCE_TOUCHSCREEN;
    move.action = AMOTION_EVENT_ACTION_MOVE;
    int moved = 0;

    for (int i = 0; i < touch.reportNum; i++) {
        float x = (float)touch.report[i].x * SCREEN_W / VITA_TOUCH_W;
        float y = (float)touch.report[i].y * SCREEN_H / VITA_TOUCH_H;
        if (x < 0.0f) x = 0.0f;
        if (x >= SCREEN_W) x = SCREEN_W - 1.0f;
        if (y < 0.0f) y = 0.0f;
        if (y >= SCREEN_H) y = SCREEN_H - 1.0f;

        int s = 0;
        while (s < TOUCH_SLOTS && s_slot_id[s] != touch.report[i].id) s++;
        if (s == TOUCH_SLOTS) {
            s = 0;
            while (s < TOUCH_SLOTS && s_slot_id[s] != -1) s++;
            if (s == TOUCH_SLOTS) continue; // more fingers than the game tracks
            s_slot_id[s] = touch.report[i].id;
            s_slot_x[s] = x;
            s_slot_y[s] = y;
            push_touch(q, AMOTION_EVENT_ACTION_DOWN, s, x, y);
            continue;
        }

        moved |= x != s_slot_x[s] || y != s_slot_y[s];
        s_slot_x[s] = x;
        s_slot_y[s] = y;
        move.pointers[move.pointerCount].id = s;
        move.pointers[move.pointerCount].x = x;
        move.pointers[move.pointerCount].y = y;
        move.pointerCount++;
    }
    if (moved)
        queue_push_locked(q, &move);

    // 3. Virtual flaps: d-pad/L/R/stick hold a simulated finger on each screen
    // half (CROSS/UP = both), sharing the real-finger slots so ids stay in 0..4.
    int virt_left = (cur_buttons & (SCE_CTRL_LTRIGGER | SCE_CTRL_LEFT)) || (pad.lx < 80);
    int virt_right = (cur_buttons & (SCE_CTRL_RTRIGGER | SCE_CTRL_RIGHT)) || (pad.lx > 175);
    if (cur_buttons & (SCE_CTRL_CROSS | SCE_CTRL_UP)) {
        virt_left = 1;
        virt_right = 1;
    }
    sim_flap(q, VIRT_LEFT_ID, virt_left, 240.0f);
    sim_flap(q, VIRT_RIGHT_ID, virt_right, 720.0f);
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
