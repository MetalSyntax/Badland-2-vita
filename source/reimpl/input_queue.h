/**
 * @file  input_queue.h
 * @brief AInputQueue reimplementation.
 */

#ifndef ANDROID_INPUT_QUEUE_H
#define ANDROID_INPUT_QUEUE_H

#include <stdint.h>
#include "reimpl/looper.h"
#include "reimpl/input_event.h"

#ifdef __cplusplus
extern "C" {
#endif

struct AInputQueue;
typedef struct AInputQueue AInputQueue;

AInputQueue *input_queue_create(void);
int input_queue_has_events(void *queue);

void AInputQueue_attachLooper(AInputQueue *queue, ALooper *looper, int ident,
                              ALooper_callbackFunc callback,
                              void *data);
void AInputQueue_detachLooper(AInputQueue *queue);
int32_t AInputQueue_hasEvents(AInputQueue *queue);
int32_t AInputQueue_getEvent(AInputQueue *queue, AInputEvent **outEvent);
int32_t AInputQueue_preDispatchEvent(AInputQueue *queue, AInputEvent *event);
void AInputQueue_finishEvent(AInputQueue *queue, AInputEvent *event, int handled);

#ifdef __cplusplus
};
#endif

#endif // ANDROID_INPUT_QUEUE_H
