/**
 * @file  input_queue.h
 * @brief Minimal reimplementation of android/input.h's AInputQueue side. The
 *        loader never creates/hands out a queue (native_activity.c never calls
 *        onInputQueueCreated), so these only exist to satisfy the .so's UND
 *        imports (AInputQueue_attachLooper/detachLooper/getEvent/
 *        preDispatchEvent/finishEvent per `objdump -T`) without crashing.
 */

#ifndef ANDROID_INPUT_QUEUE_H
#define ANDROID_INPUT_QUEUE_H

#include <stdint.h>

#include "reimpl/looper.h"

#ifdef __cplusplus
extern "C" {
#endif

struct AInputQueue;
typedef struct AInputQueue AInputQueue;
struct AInputEvent;
typedef struct AInputEvent AInputEvent;

void AInputQueue_attachLooper(AInputQueue *queue, ALooper *looper, int ident,
                              int (*callback)(int fd, int events, void *data),
                              void *data);
void AInputQueue_detachLooper(AInputQueue *queue);
int32_t AInputQueue_getEvent(AInputQueue *queue, AInputEvent **outEvent);
int32_t AInputQueue_preDispatchEvent(AInputQueue *queue, AInputEvent *event);
void AInputQueue_finishEvent(AInputQueue *queue, AInputEvent *event, int handled);

#ifdef __cplusplus
};
#endif

#endif // ANDROID_INPUT_QUEUE_H
