#include "reimpl/looper.h"

#include <psp2/kernel/threadmgr.h>
#include <stddef.h>

struct ALooper {
    int unused;
};

static ALooper g_fake_looper = {0};

ALooper * ALooper_forThread(void) {
    return &g_fake_looper;
}

ALooper * ALooper_prepare(int opts) {
    (void)opts;
    return &g_fake_looper;
}

int ALooper_pollAll(int timeoutMillis, int *outFd, int *outEvents, void **outData) {
    // No real fd/event source to wait on yet (no input queue is ever attached --
    // see native_activity.c). A negative timeoutMillis means "block forever" in
    // the real API; since nothing can wake us, cap the sleep so the glue thread
    // never turns into a 100%-CPU busy loop.
    int sleep_ms = (timeoutMillis < 0 || timeoutMillis > 50) ? 50 : timeoutMillis;
    if (sleep_ms > 0)
        sceKernelDelayThread((SceUInt32)sleep_ms * 1000);
    (void)outFd;
    (void)outEvents;
    (void)outData;
    return ALOOPER_POLL_TIMEOUT;
}

int ALooper_addFd(ALooper *looper, int fd, int ident, int events,
                  int (*callback)(int fd, int events, void *data), void *data) {
    // Accepted but never reported: pollAll above always returns TIMEOUT, so
    // the glue never dispatches this fd. Revisit with real input (Phase 6/7).
    (void)looper;
    (void)fd;
    (void)ident;
    (void)events;
    (void)callback;
    (void)data;
    return 1;
}
