package com.ziky.ffmpegJni;

import android.view.Surface;

public class FFmpegUtils {

    static {
        System.loadLibrary("native-lib");
        System.loadLibrary("avcodec");
        System.loadLibrary("avfilter");
        System.loadLibrary("avformat");
        System.loadLibrary("avutil");
        System.loadLibrary("swresample");
        System.loadLibrary("swscale");
    }

    public static native String getVersion();
    public static native void play(String path, Surface surface);

}
