package com.qsp.nvpfa;

import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.content.Intent;
import android.provider.Settings;
import android.net.Uri;
import org.libsdl.app.SDLActivity;

public class NvpfaActivity extends SDLActivity {
    private boolean mRequestedAllFiles = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().getDecorView().setOnSystemUiVisibilityChangeListener(
            visibility -> hideSystemUI());
        hideSystemUI();
        if(!mRequestedAllFiles && shouldRequestAllFilesAccess())
        {
            mRequestedAllFiles = true;
            Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
            intent.setData(Uri.parse("package:" + getPackageName()));
            startActivity(intent);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        hideSystemUI();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus)
            hideSystemUI();
    }

    private static boolean shouldRequestAllFilesAccess() {
        if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.R)
            return false;
        return !Environment.isExternalStorageManager();
    }

    private void hideSystemUI() {
        View decorView = getWindow().getDecorView();
        decorView.setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_FULLSCREEN);
    }
}
