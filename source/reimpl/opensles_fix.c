/*
 * Workaround for a deadlock in VitaSDK's libOpenSLES (wilhelm + Vita.o backend).
 *
 * Its COutputMix_PreDestroy always sets mDestroyRequested and waits on the
 * object's condvar until the mixer (audioThread -> IOutputMixExt_FillBuffer)
 * acknowledges. Upstream wilhelm only waits if this mix is the engine's
 * *active* one. The Vita backend has a single global mixer thread, which
 * SDL_close() (any CEngine_Destroy) kills, so FMOD destroying its OutputMix
 * hung forever in Studio::System::initialize (psp2dmp 007: main thread in
 * object_cond_wait, "OpenSLES Playback" dormant).
 *
 * Linked with -Wl,--wrap=COutputMix_PreDestroy (referenced from classes.o).
 * Offsets read from the linked library, see port_progress.md (log 007).
 */

#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>

#include "utils/logger.h"

#define OBJ_ENGINE          4       // IObject.mEngine (CEngine *)
#define OBJ_MUTEX           36      // IObject.mMutex
#define OBJ_COND            40      // IObject.mCond
#define OBJ_STRONG_REFS     0x2e    // IObject.mStrongRefCount
#define CENGINE_IENGINE     0x4c    // CEngine.mEngine (what SDL_open gets)
#define IENGINE_OUTPUTMIX   12      // IEngine.mOutputMix
#define MIX_DESTROY_REQ     0x3e8   // COutputMix.mOutputMixExt.mDestroyRequested

#define PREDESTROY_OK 1

int __real_COutputMix_PreDestroy(void *self);

int __wrap_COutputMix_PreDestroy(void *self) {
    uint8_t *mix = self;

    // Strong refs still held: the original logs + refuses; keep that.
    if (mix[OBJ_STRONG_REFS] != 0)
        return __real_COutputMix_PreDestroy(self);

    uint8_t *cengine = *(uint8_t **)(mix + OBJ_ENGINE);
    void * volatile *active = cengine
        ? (void * volatile *)(cengine + CENGINE_IENGINE + IENGINE_OUTPUTMIX)
        : NULL;

    if (!active || *active != self) {
        l_debug("OpenSLES: OutputMix %p is not the active mix -- no mixer handshake.", self);
        return PREDESTROY_OK;
    }

    // Same handshake as the original, but bounded: the caller holds the
    // object mutex, and timedwait releases it so a live mixer can ack.
    pthread_mutex_t *m = (pthread_mutex_t *)(mix + OBJ_MUTEX);
    pthread_cond_t *c = (pthread_cond_t *)(mix + OBJ_COND);
    volatile uint32_t *req = (volatile uint32_t *)(mix + MIX_DESTROY_REQ);

    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_nsec += 500 * 1000 * 1000;
    if (deadline.tv_nsec >= 1000000000) {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000;
    }

    *req = 1;
    while (*req) {
        if (pthread_cond_timedwait(c, m, &deadline) == ETIMEDOUT && *req) {
            // Mixer thread is gone: do its half of the handshake ourselves.
            l_warn("OpenSLES: mixer never acked OutputMix %p destroy -- unlinking it.", self);
            *active = NULL;
            *req = 0;
        }
    }
    return PREDESTROY_OK;
}
