#include "utils/init.h"
#include "utils/logger.h"
#include "utils/glutil.h"
#include "reimpl/native_activity.h"
#include "reimpl/egl.h"

#include <psp2/kernel/threadmgr.h>

#include <falso_jni/FalsoJNI.h>
#include <so_util/so_util.h>

int _newlib_heap_size_user = 256 * 1024 * 1024;

#ifdef USE_SCELIBC_IO
int sceLibcHeapSize = 4 * 1024 * 1024;
#endif

so_module so_mod;
so_module so_mod_fmod;
so_module so_mod_fmodstudio;

int main() {
    soloader_init_all();

    // Mirrors what a real Android process does before an Activity ever exists:
    // System.loadLibrary() triggers JNI_OnLoad once at library-load time.
    // (libbadland2.so's JNI_OnLoad only calls JniUtils::setJavaVM + returns
    // JNI_VERSION_1_6 -- verified via objdump disasm, .text @ 0x13f088.)
    int (* JNI_OnLoad)(void *jvm) = (void *)so_symbol(&so_mod, "JNI_OnLoad");
    if (JNI_OnLoad) {
        l_debug("Calling JNI_OnLoad...");
        JNI_OnLoad(&jvm);
        l_debug("JNI_OnLoad returned.");
    } else {
        l_error("JNI_OnLoad not found -- continuing anyway.");
    }

    // Do NOT call gl_init() here: reimpl/egl.c's eglInitialize() already calls
    // it, and the engine calls eglInitialize itself from the glue thread in
    // response to onNativeWindowCreated below. Double-init breaks vitaGL.
    native_activity_bootstrap(&so_mod);

    // Drive the Java-side init the real Activity would do (no Java here, so the
    // loader calls the native entry points directly -- cf. Main.java onCreate/
    // onStart and NativeInterface.java init()):
    //   NativeInterface.init -> setupDirectories -> nativeSetFileDirectories
    //   NativeInterface.init (nativeInitJni is invoked from the native side's
    //     own init path; called here explicitly to match Main.onStart order)
    //   Main.onCreate -> nativeInitFmod (FMOD imports currently unresolved, so
    //     this call is expected to trap until the FMOD strategy lands -- see
    //     dynlib.c NOTE and port_progress.md Fase 4).
    // NOTE: this FalsoJNI's own table functions take JNIEnv* (&jni), while the
    // .so's Java_* exports follow the real JNI convention and take JNIEnv.
    void (* nativeSetFileDirectories)(JNIEnv, void *, jstring, jstring) =
        (void *)so_symbol(&so_mod, "Java_com_frogmind_badland2_NativeInterface_nativeSetFileDirectories");
    void (* nativeInitJni)(JNIEnv, void *) =
        (void *)so_symbol(&so_mod, "Java_com_frogmind_badland2_NativeInterface_nativeInitJni");
    void (* nativeInitFmod)(JNIEnv, void *) =
        (void *)so_symbol(&so_mod, "Java_com_frogmind_badland2_Main_nativeInitFmod");

    if (nativeSetFileDirectories) {
        l_debug("Calling nativeSetFileDirectories...");
        jstring internal_ = jni->NewStringUTF(&jni, DATA_PATH);
        jstring external_ = jni->NewStringUTF(&jni, DATA_PATH);
        nativeSetFileDirectories(jni, NULL, internal_, external_);
        l_debug("nativeSetFileDirectories returned.");
    } else {
        l_error("nativeSetFileDirectories not found.");
    }

    if (nativeInitJni) {
        l_debug("Calling nativeInitJni...");
        nativeInitJni(jni, NULL);
        l_debug("nativeInitJni returned.");
    } else {
        l_error("nativeInitJni not found.");
    }

    if (nativeInitFmod) {
        l_debug("Calling nativeInitFmod...");
        nativeInitFmod(jni, NULL);
        l_debug("nativeInitFmod returned.");
    } else {
        l_error("nativeInitFmod not found.");
    }

    // From here the android_native_app_glue thread (spawned by
    // ANativeActivity_onCreate) owns android_main and the frame loop, pumping
    // our ALooper stub and drawing through our EGL reimpl. The main thread
    // just keeps the process alive with a low-frequency heartbeat so a frozen
    // log means "stuck inside an init call above", not "loop died".
    int tick = 0;
    while (1) {
        sceKernelDelayThread(1000 * 1000);
        if (++tick % 10 == 0)
            l_debug("main heartbeat: %d s since boot", tick);
    }

    sceKernelExitDeleteThread(0);
}
