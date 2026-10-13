/**
 * @file  input_queue.c
 * @brief Thread-safe AInputQueue implementation.
 */

#include "reimpl/input_queue.h"
#include "utils/logger.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define QUEUE_CAPACITY 64

struct AInputQueue {
    pthread_mutex_t lock;
    int head, count;
    ALooper *looper;
};

static AInputQueue g_default_input_queue;
static volatile int g_input_queue_inited = 0;

static void ensure_input_queue_inited(AInputQueue *q) {
    if (!g_input_queue_inited) {
        pthread_mutex_init(&q->lock, NULL);
        q->head = 0;
        q->count = 0;
        q->looper = NULL;
        g_input_queue_inited = 1;
    }
}

AInputQueue *input_queue_create(void) {
    ensure_input_queue_inited(&g_default_input_queue);
    return &g_default_input_queue;
}

int input_queue_has_events(void *queue) {
    AInputQueue *q = (AInputQueue *)queue;
    if (!q) return 0;
    ensure_input_queue_inited(q);
    pthread_mutex_lock(&q->lock);
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

static inline int q_event_count(AInputQueue *q) {
    return q->count;
}

int32_t AInputQueue_getEvent(AInputQueue *queue, AInputEvent **outEvent) {
    if (!queue) return -1;
    ensure_input_queue_inited(queue);
    pthread_mutex_lock(&queue->lock);
    if (q_event_count(queue) == 0) {
        pthread_mutex_unlock(&queue->lock);
        if (outEvent) *outEvent = NULL;
        return -1;
    }
    // No events to dequeue currently
    pthread_mutex_unlock(&queue->lock);
    if (outEvent) *outEvent = NULL;
    return -1;
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
