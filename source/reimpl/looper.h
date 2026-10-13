/**
 * @file  looper.h
 * @brief Minimal reimplementation of android/looper.h. No real epoll/fd event
 *        source exists on Vita, so ALooper_pollAll() sleeps for the caller's
 *        timeout and reports "nothing happened" -- enough for the
 *        android_native_app_glue thread inside libbadland2.so to not busy-spin
 *        while input isn't wired up yet (PORTING_PLAN.md Phase 6/7).
 */

#ifndef ANDROID_LOOPER_H
#define ANDROID_LOOPER_H

#ifdef __cplusplus
extern "C" {
#endif

struct ALooper;
typedef struct ALooper ALooper;

enum {
    ALOOPER_PREPARE_ALLOW_NON_CALLBACKS = 1 << 0,
};

enum {
    ALOOPER_POLL_WAKE = -1,
    ALOOPER_POLL_CALLBACK = -2,
    ALOOPER_POLL_TIMEOUT = -3,
    ALOOPER_POLL_ERROR = -4,
};

enum {
    ALOOPER_EVENT_INPUT = 1 << 0,
    ALOOPER_EVENT_OUTPUT = 1 << 1,
    ALOOPER_EVENT_ERROR = 1 << 2,
    ALOOPER_EVENT_HANGUP = 1 << 3,
    ALOOPER_EVENT_INVALID = 1 << 4,
};

ALooper * ALooper_forThread(void);
ALooper * ALooper_prepare(int opts);
int ALooper_pollAll(int timeoutMillis, int *outFd, int *outEvents, void **outData);
int ALooper_addFd(ALooper *looper, int fd, int ident, int events,
                  int (*callback)(int fd, int events, void *data), void *data);

#ifdef __cplusplus
};
#endif

#endif // ANDROID_LOOPER_H
