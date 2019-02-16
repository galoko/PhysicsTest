package com.example.physicstest;

import android.app.Activity;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Toast;

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.StringWriter;

public class MainActivity extends Activity implements SurfaceHolder.Callback {

    private AssetManager assetManager;

    static {
        System.loadLibrary("main");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        assetManager = getAssets();

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        SurfaceView surfaceView = (SurfaceView)findViewById(R.id.surfaceview);
        surfaceView.getHolder().addCallback(this);

        // just a test
        surfaceView.setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                Toast toast = Toast.makeText(MainActivity.this,
                        "Java is working!",
                        Toast.LENGTH_LONG);
                toast.show();
            }});
    }

    @Override
    protected void onStart() {
        super.onStart();
        JNIHandler.init(assetManager);
    }

    @Override
    protected void onResume() {
        super.onResume();
        JNIHandler.start();
    }

    @Override
    protected void onPause() {
        super.onPause();
        JNIHandler.stop();
    }

    @Override
    protected void onStop() {
        super.onStop();
        JNIHandler.destroy();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    public void surfaceCreated(SurfaceHolder holder) {
    }

    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        JNIHandler.setOutputSurface(holder.getSurface());
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
        JNIHandler.setOutputSurface(null);
    }
}
