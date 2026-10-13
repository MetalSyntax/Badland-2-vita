#include "reimpl/input_queue.h"

#include <stddef.h>

void AInputQueue_attachLooper(AInputQueue *queue, ALooper *looper, int ident,
                              int (*callback)(int fd, int events, void *data),
                              void *data) {
    (void)queue;
    (void)looper;
    (void)ident;
    (void)callback;
    (void)data;
}

void AInputQueue_detachLooper(AInputQueue *queue) {
    (void)queue;
}

int32_t AInputQueue_getEvent(AInputQueue *queue, AInputEvent **outEvent) {
    // Negative return: no event available, per android/input.h.
    (void)queue;
    if (outEvent)
        *outEvent = NULL;
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
