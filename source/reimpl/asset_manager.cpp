#include "reimpl/asset_manager.h"
#include "utils/logger.h"

#include <pthread.h>
#include <malloc.h>
#include <cstring>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#ifdef USE_SCELIBC_IO
#include <libc_bridge/libc_bridge.h>
#endif
#include <string>

typedef struct assetManager {
    int dummy = 0;
    pthread_mutex_t mLock;
} assetManager;

typedef struct aAsset {
    char * filename;
    FILE* f;
    int fd;
    size_t bytesRead;
    size_t fileSize;
    void * buffer;
} asset;

static AAssetManager * g_AAssetManager = nullptr;

AAssetManager * AAssetManager_create() {
    if (g_AAssetManager) return g_AAssetManager;

    assetManager am;
    pthread_mutex_init(&am.mLock, nullptr);

    g_AAssetManager = (AAssetManager *) malloc(sizeof(assetManager));
    memcpy(g_AAssetManager, &am, sizeof(assetManager));

    return g_AAssetManager;
}

AAsset* AAssetManager_open(AAssetManager* mgr, const char* filename, int mode) {
    if (!filename) return nullptr;

    const char* p = filename;
    while (*p == '/') p++;
    while (p[0] == '.' && p[1] == '/') p += 2;

    std::string rel_path = p;
    std::string realp = std::string(DATA_PATH) + std::string("assets/") + rel_path;

    auto * a = new aAsset;
    a->filename = strdup(realp.c_str());
    a->bytesRead = 0;
    a->fileSize = 0;
    a->fd = -1;
    a->buffer = nullptr;

#ifdef USE_SCELIBC_IO
    a->f = sceLibcBridge_fopen(a->filename, "rb");
#else
    a->f = fopen(a->filename, "rb");
#endif

    // Fallback 1: if path starts with "../", try under DATA_PATH directly without "../"
    if (!a->f && rel_path.rfind("../", 0) == 0) {
        std::string fallback = std::string(DATA_PATH) + rel_path.substr(3);
#ifdef USE_SCELIBC_IO
        a->f = sceLibcBridge_fopen(fallback.c_str(), "rb");
#else
        a->f = fopen(fallback.c_str(), "rb");
#endif
        if (a->f) {
            free(a->filename);
            a->filename = strdup(fallback.c_str());
        }
    }

    // Fallback 2: try directly under DATA_PATH
    if (!a->f) {
        std::string fallback2 = std::string(DATA_PATH) + rel_path;
#ifdef USE_SCELIBC_IO
        a->f = sceLibcBridge_fopen(fallback2.c_str(), "rb");
#else
        a->f = fopen(fallback2.c_str(), "rb");
#endif
        if (a->f) {
            free(a->filename);
            a->filename = strdup(fallback2.c_str());
        }
    }

    if (!a->f) {
        free(a->filename);
        delete a;
        l_warn("[AAssetManager] AAssetManager_open(%p, %s, %i): not found", mgr, filename, mode);
        return nullptr;
    }

#ifdef USE_SCELIBC_IO
    sceLibcBridge_fseek(a->f, 0, SEEK_END);
    a->fileSize = sceLibcBridge_ftell(a->f);
    sceLibcBridge_fseek(a->f, 0, SEEK_SET);
#else
    fseek(a->f, 0, SEEK_END);
    a->fileSize = ftell(a->f);
    fseek(a->f, 0, SEEK_SET);
#endif

    l_debug("[AAssetManager] AAssetManager_open(%p, %s, %i): opened %s (size %zu) -> %p",
            mgr, filename, mode, a->filename, a->fileSize, a);
    return (AAsset *) a;
}

void AAsset_close(AAsset* asset) {
    l_debug("AAsset_close(%p)", asset);

    if (asset) {
        auto * a = (aAsset *) asset;
        if (a->fd >= 0) {
            close(a->fd);
            a->fd = -1;
        }
        if (a->f) {
#ifdef USE_SCELIBC_IO
            sceLibcBridge_fclose(a->f);
#else
            fclose(a->f);
#endif
            a->f = nullptr;
        }
        if (a->buffer) {
            free(a->buffer);
            a->buffer = nullptr;
        }
        if (a->filename) {
            free(a->filename);
            a->filename = nullptr;
        }
        delete a;
    }
}

int AAsset_read(AAsset* asset, void* buf, size_t count) {
    if (!asset || !buf) {
        return -1;
    }

    auto * a = (aAsset *) asset;
    if (!a->f) {
        return -1;
    }

#ifdef USE_SCELIBC_IO
    size_t ret = sceLibcBridge_fread(buf, 1, count, a->f);
#else
    size_t ret = fread(buf, 1, count, a->f);
#endif

    if (ret > 0) {
        a->bytesRead += ret;
        return (int) ret;
    } else {
#ifdef USE_SCELIBC_IO
        if (count == 0 || sceLibcBridge_feof(a->f)) {
#else
        if (count == 0 || feof(a->f)) {
#endif
            return 0;
        } else {
            l_warn("AAsset_read(%p, %p, %u): error (-1)", asset, buf, (unsigned int)count);
            return -1;
        }
    }
}

off_t AAsset_seek(AAsset* asset, off_t offset, int whence) {
    if (!asset) {
        return (off_t) -1;
    }

    auto * a = (aAsset *) asset;
    if (!a->f) {
        return (off_t) -1;
    }

#ifdef USE_SCELIBC_IO
    int res = sceLibcBridge_fseek(a->f, offset, whence);
    if (res != 0) return (off_t) -1;
    long pos = sceLibcBridge_ftell(a->f);
#else
    int res = fseek(a->f, offset, whence);
    if (res != 0) return (off_t) -1;
    long pos = ftell(a->f);
#endif

    if (pos >= 0) {
        a->bytesRead = (size_t)pos;
    }
    return (off_t) pos;
}

off_t AAsset_getRemainingLength(AAsset* asset) {
    if (!asset) {
        return (off_t) -1;
    }

    auto * a = (aAsset *) asset;
    if (!a->f) {
        return (off_t) -1;
    }

#ifdef USE_SCELIBC_IO
    long pos = sceLibcBridge_ftell(a->f);
#else
    long pos = ftell(a->f);
#endif
    if (pos < 0) pos = (long)a->bytesRead;
    off_t remaining = (a->fileSize > (size_t)pos) ? (off_t)(a->fileSize - (size_t)pos) : (off_t)0;
    l_debug("AAsset_getRemainingLength(%p): %ld", asset, (long)remaining);
    return remaining;
}

off_t AAsset_getLength(AAsset* asset) {
    if (!asset) {
        return (off_t) -1;
    }

    auto * a = (aAsset *) asset;
    l_debug("AAsset_getLength(%p): %u", asset, (unsigned int)a->fileSize);
    return (off_t)a->fileSize;
}

const void* AAsset_getBuffer(AAsset* asset) {
    if (!asset) return nullptr;
    auto * a = (aAsset *) asset;
    if (a->buffer) return a->buffer;
    if (!a->f || a->fileSize == 0) return nullptr;

    a->buffer = malloc(a->fileSize);
    if (!a->buffer) return nullptr;

#ifdef USE_SCELIBC_IO
    long cur = sceLibcBridge_ftell(a->f);
    sceLibcBridge_fseek(a->f, 0, SEEK_SET);
    size_t read_bytes = sceLibcBridge_fread(a->buffer, 1, a->fileSize, a->f);
    sceLibcBridge_fseek(a->f, cur, SEEK_SET);
#else
    long cur = ftell(a->f);
    fseek(a->f, 0, SEEK_SET);
    size_t read_bytes = fread(a->buffer, 1, a->fileSize, a->f);
    fseek(a->f, cur, SEEK_SET);
#endif

    if (read_bytes < a->fileSize) {
        l_warn("AAsset_getBuffer: read %zu of %zu bytes", read_bytes, a->fileSize);
    }
    return a->buffer;
}

int AAsset_openFileDescriptor(AAsset* asset, off_t* outStart, off_t* outLength) {
    if (!asset) {
        return -1;
    }

    auto * a = (aAsset *) asset;
    if (a->fd < 0) {
        a->fd = open(a->filename, O_RDONLY);
        if (a->fd < 0) {
            l_error("AAsset_openFileDescriptor: cannot open %s", a->filename);
            return -1;
        }
    }

    if (outStart)
        *outStart = 0;
    if (outLength)
        *outLength = (off_t)a->fileSize;

    l_debug("AAsset_openFileDescriptor(%p, len=%u): fd %d", asset, (unsigned int)a->fileSize, a->fd);
    return a->fd;
}

AAssetDir* AAssetManager_openDir(AAssetManager* mgr, const char* dirName) {
    l_error("AAssetManager_openDir: %s", dirName ? dirName : "(null)");
    return (AAssetDir *)strdup("dummy");
}

void AAssetDir_close(AAssetDir* assetDir) {
    l_error("AAssetDir_close");
    if (assetDir) free(assetDir);
}
