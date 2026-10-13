/**
 * @file  looper.h
 * @brief ALooper + fake pipe() implementation for android_native_app_glue.
 */

#ifndef ANDROID_LOOPER_H
#define ANDROID_LOOPER_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

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

typedef int (*ALooper_callbackFunc)(int fd, int events, void *data);

typedef enum {
    LOOPER_SRC_FD = 0,
    LOOPER_SRC_INPUT,
} looper_src_type;

ALooper * ALooper_forThread(void);
ALooper * ALooper_prepare(int opts);
int ALooper_pollAll(int timeoutMillis, int *outFd, int *outEvents, void **outData);
int ALooper_pollOnce(int timeoutMillis, int *outFd, int *outEvents, void **outData);
int ALooper_addFd(ALooper *looper, int fd, int ident, int events,
                  ALooper_callbackFunc callback, void *data);
int ALooper_removeFd(ALooper *looper, int fd);
void ALooper_wake(ALooper *looper);

void looper_signal(void);
int looper_attach_source(ALooper *looper, looper_src_type type, void *obj,
                         int ident, void *data);
void looper_detach_source(ALooper *looper, void *obj);

/* Fake pipe implementation */
#define FAKE_PIPE_FD_BASE 0x7000

int pipe_soloader(int fds[2]);
ssize_t read_soloader(int fd, void *buf, size_t count);
ssize_t write_soloader(int fd, const void *buf, size_t count);
int fake_pipe_close(int fd);

#ifdef __cplusplus
};
#endif

#endif // ANDROID_LOOPER_H
