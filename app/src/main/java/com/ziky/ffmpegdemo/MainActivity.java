package com.ziky.ffmpegdemo;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.ziky.ffmpegJni.FFmpegUtils;

public class MainActivity extends AppCompatActivity {

    private TextView tvVersion;
    private SurfaceView surfaceView;
    private final String path = "/storage/emulated/0/test.mp4";


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        tvVersion = findViewById(R.id.tv_version_info);
        surfaceView = findViewById(R.id.surfaceview);
        findViewById(R.id.btn_version).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                tvVersion.setText(FFmpegUtils.getVersion());
            }
        });
        findViewById(R.id.btn_play).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        FFmpegUtils.play(path, surfaceView.getHolder().getSurface());
                    }
                }).start();
            }
        });
    }


}
