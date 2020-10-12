#include <jni.h>
#include <string>
#include <android/log.h>


extern "C" {
#include <libswresample/swresample.h>
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
//像素处理
#include "libswscale/swscale.h"
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include <unistd.h>
}

#define TAG "ffmpegJNI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,TAG,__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL,TAG,__VA_ARGS__)

extern "C"
JNIEXPORT jstring JNICALL
Java_com_ziky_ffmpegJni_FFmpegUtils_getVersion(JNIEnv *env, jclass clazz) {
    return env->NewStringUTF(av_version_info());
}extern "C"
JNIEXPORT void JNICALL
Java_com_ziky_ffmpegJni_FFmpegUtils_play(JNIEnv *env, jclass clazz, jstring path, jobject surface) {

    const char *inputPath = env->GetStringUTFChars(path, JNI_FALSE);
    LOGD("file = %s", inputPath);
    //注册各大组件
    av_register_all();
    LOGE("注册成功");
    AVFormatContext *avFormatContext = avformat_alloc_context();//获取上下文
    int error;
    char buf[] = "";
    if ((error = avformat_open_input(&avFormatContext, inputPath, NULL, NULL)) < 0) {
        av_strerror(error, buf, 1024);
        LOGE("Could't open file %s: %d(%s)", inputPath, error, buf);
        LOGE("打开视频失败");
        return;
    }

    if (avformat_find_stream_info(avFormatContext, NULL) < 0) {
        LOGE("读取内容失败");
        return;
    }
//获取到整个内容过后找到里面的视频流
    int video_index = -1;
    for (int i = 0; i < avFormatContext->nb_streams; i++) {
        if (avFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = i;
            break;
        }
    }

    LOGD("video_index = %d", video_index);
    if (video_index < 0) {
        LOGE("未找到视频流");
        return;
    }

    //对视频流进行解码
    //获取解码器上下文
    AVCodecContext *avCodecContext = avFormatContext->streams[video_index]->codec;
    //获取解码器
    AVCodec *avCodec = avcodec_find_decoder(avCodecContext->codec_id);
    //打开解码器
    if (avcodec_open2(avCodecContext, avCodec, NULL) < 0) {
        LOGE("打开解码器失败");
        return;
    }

    //申请AVPacket
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    av_init_packet(packet);
    //申请AVFrame
    AVFrame *frame = av_frame_alloc();//分配一个AVFrame结构体，AVFrame结构体一般用于存储原始数据，指向解码后的原始帧
    AVFrame *rgb_frame = av_frame_alloc();//分配一个AVFrame结构体，指向存放转换成rgb后的帧

    //缓存区
    uint8_t *out_buffer = (uint8_t *) av_malloc(
            avpicture_get_size(AV_PIX_FMT_RGBA, avCodecContext->width, avCodecContext->height));
    //与缓存区相关联，设置rgb_frame缓存区

    avpicture_fill((AVPicture *) (rgb_frame), out_buffer, AV_PIX_FMT_RGBA, avCodecContext->width,
                   avCodecContext->height);

    SwsContext *swsContext = sws_getContext(avCodecContext->width, avCodecContext->height,
                                            avCodecContext->pix_fmt,
                                            avCodecContext->width, avCodecContext->height,
                                            AV_PIX_FMT_RGBA, SWS_BICUBIC, NULL, NULL, NULL);

    //读取nativewindow
    ANativeWindow *aNativeWindow = ANativeWindow_fromSurface(env, surface);
    if (aNativeWindow == 0) {
        LOGE("nativewindow取到失败");
        return;
    }
    //视频缓冲区
    ANativeWindow_Buffer native_outBuffer;

    int frameCount;
    int h = 0;
    LOGE("解码");
    while (av_read_frame(avFormatContext, packet) >= 0) {
        LOGE("解码 %d", packet->stream_index);
        LOGE("VINDEX %d", video_index);
        if (packet->stream_index == video_index) {
            LOGE("解码 hhhh");
            avcodec_decode_video2(avCodecContext, frame, &frameCount, packet);
            LOGE("解码中......  %d", frameCount);
            if (frameCount) {
                LOGE("转换并绘制");
                //说明有内容
                //绘制之前配置nativewindow
                ANativeWindow_setBuffersGeometry(aNativeWindow, avCodecContext->width,
                                                 avCodecContext->height, WINDOW_FORMAT_RGBA_8888);
                //上锁
                ANativeWindow_lock(aNativeWindow, &native_outBuffer, NULL);
                //转换成rgb格式
                sws_scale(swsContext, (const uint8_t *const *) frame->data, frame->linesize, 0,
                          frame->height,
                          rgb_frame->data, rgb_frame->linesize);
                //rgb_frame是有画面数据
                uint8_t *dst = (uint8_t *) native_outBuffer.bits;
                //拿到一行有多少个字节 RGBA
                int destStride = native_outBuffer.stride * 4;
                //像素数据的首地址
                uint8_t *src = rgb_frame->data[0];
                //实际内存一行数量
                int srcStride = rgb_frame->linesize[0];
                for (int i = 0; i < avCodecContext->height; i++) {
                    memcpy(dst + i * destStride, src + i * srcStride, srcStride);
                }
                ANativeWindow_unlockAndPost(aNativeWindow);
                //usleep(1000 * 16);
            }
        }
        av_free_packet(packet);
    }
    ANativeWindow_release(aNativeWindow);
    av_frame_free(&frame);
    av_frame_free(&rgb_frame);
    avcodec_close(avCodecContext);
    avformat_free_context(avFormatContext);
    env->ReleaseStringUTFChars(path, inputPath);
}extern "C"
JNIEXPORT void JNICALL
Java_com_ziky_ffmpegJni_FFmpegUtils_playSound(JNIEnv *env, jclass clazz, jstring path) {

    const char *inputPath = env->GetStringUTFChars(path, JNI_FALSE);
    LOGD("path = %s", inputPath);

    av_register_all();
    AVFormatContext *pFormatCtx = avformat_alloc_context();

    //open
    if (avformat_open_input(&pFormatCtx, inputPath, NULL, NULL) < 0) {
        LOGE("打开输入视频文件失败");
        return;
    }
    //获取视频信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("获取视频信息失败");
        return;
    }
    int audio_stream_index = -1;
    for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            LOGE("找到音频id %d", pFormatCtx->streams[i]->codec->codec_type);
            audio_stream_index = i;
            break;
        }
    }

    //获取解码器上下文
    AVCodecContext *pCodecCtx = pFormatCtx->streams[audio_stream_index]->codec;
    //获取解码器
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    //打开解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGE("打开解码器失败");
        return;
    }
    //申请avpacket，装解码前的数据
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    //申请avframe,装解码后的数据
    AVFrame *frame = av_frame_alloc();

    //得到SwrContext,进行重采样
    SwrContext *swrContext = swr_alloc();
    //缓冲区
    uint8_t *out_buffer = (uint8_t *) av_malloc(44100 * 2);
    //输出的声道布局(立体声)
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    //输出采样位数 16位
    enum AVSampleFormat out_format = AV_SAMPLE_FMT_S16;
    //输出的采样率必须与输入相同
    int out_sample_rate = pCodecCtx->sample_rate;
    //swr_alloc_set_opts将PCM源文件的采样格式转换为自己希望的采样格式
    swr_alloc_set_opts(swrContext, out_ch_layout, out_format, out_sample_rate,
                       pCodecCtx->channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate, 0,
                       NULL);
    swr_init(swrContext);
    //获取通道数 2
    int out_channer_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    //反射得到class类型
//      jclass player = env->GetObjectClass(clazz);
    jclass player = env->FindClass("com/ziky/ffmpegJni/FFmpegUtils");
    //反射得到createAudio方法
    jmethodID createAudio = env->GetStaticMethodID(player, "createTrack", "(II)V");
    //反射调用createAudio
    env->CallStaticVoidMethod(player, createAudio, 44100, out_ch_layout);
    jmethodID audio_write = env->GetStaticMethodID(player, "playTrack", "([BI)V");

    int got_frame;
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index == audio_stream_index) {
            //解码MP3 编码格式frame---pcm frame
            avcodec_decode_audio4(pCodecCtx, frame, &got_frame, packet);
            if (got_frame) {
                LOGE("解码");
                swr_convert(swrContext, &out_buffer, 44100 * 2, (const uint8_t **) frame->data,
                            frame->nb_samples);
                //缓冲区的大小
                int size = av_samples_get_buffer_size(NULL, out_channer_nb, frame->nb_samples,
                                                      AV_SAMPLE_FMT_S16, 1);
                jbyteArray audio_sample_array = env->NewByteArray(size);
                env->SetByteArrayRegion(audio_sample_array, 0, size, (const jbyte *) out_buffer);
                env->CallStaticVoidMethod(player, audio_write, audio_sample_array, size);
                env->DeleteLocalRef(audio_sample_array);
            }
        }
    }
    av_frame_free(&frame);
    swr_free(&swrContext);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    env->ReleaseStringUTFChars(path, inputPath);
}