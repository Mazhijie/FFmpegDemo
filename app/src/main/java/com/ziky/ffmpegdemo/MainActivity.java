package com.ziky.ffmpegdemo;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.ziky.ffmpegJni.FFmpegUtils;

public class MainActivity extends AppCompatActivity {

    private TextView tvVersion;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        tvVersion = findViewById(R.id.tv_version_info);
        findViewById(R.id.btn_version).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                tvVersion.setText(FFmpegUtils.getVersion());
            }
        });
    }


}
