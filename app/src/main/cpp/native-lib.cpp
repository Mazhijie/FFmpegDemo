#include <jni.h>
#include <string>

extern "C" {
#include <libavutil/avutil.h>
}


extern "C"
JNIEXPORT jstring JNICALL
Java_com_ziky_ffmpegJni_FFmpegUtils_getVersion(JNIEnv *env, jclass clazz) {
    return env->NewStringUTF(av_version_info());
}