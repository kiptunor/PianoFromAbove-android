package com.qsp.nvpfa

import android.os.Bundle
import android.util.Log
import android.os.Build
import android.Manifest
import android.content.pm.PackageManager
import android.content.Intent
import android.net.Uri
import android.os.Environment
import android.provider.Settings
import android.app.AlertDialog
import android.os.Handler
import android.os.Looper
import android.view.View
import android.view.Gravity
import android.widget.TextView
import android.widget.LinearLayout
import android.widget.FrameLayout
import android.graphics.Color
import android.graphics.Typeface
import android.content.Context

import org.libsdl.app.SDLActivity
import java.io.File
import java.io.FileOutputStream
import java.io.IOException

class NvpfaActivity : SDLActivity()
{
    companion object
    {
        private const val TAG = "NvpfaActivity"
        private const val REQUEST_MANAGE_EXTERNAL_STORAGE = 1001
        private const val REQUEST_STORAGE_PERMISSION = 1002

        private var overlayLayout: LinearLayout? = null
        private val uiHandler = Handler(Looper.getMainLooper())

        private var timeValueViews = mutableListOf<TextView>()
        private var fpsValueView: TextView? = null

        @JvmStatic
        fun updateTime(timeStr: String)
        {
            uiHandler.post {
                if (timeValueViews.size < 1) return@post
                timeValueViews[0].text = timeStr
            }
        }

        @JvmStatic
        fun updateFps(fps: String)
        {
            uiHandler.post {
                fpsValueView?.text = fps
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?)
    {
        super.onCreate(savedInstanceState)

        extractAssets()
        requestStorageAccess()

        Handler(Looper.getMainLooper()).postDelayed({
            hideSystemBars()
        }, 3000L)

        val density = resources.displayMetrics.density
        val shadowColor = Color.argb(255, 0x40, 0x40, 0x40)
        val tahoma = Typeface.createFromAsset(assets, "tahoma.ttf")
        var rowCount = 0

        overlayLayout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(Color.argb(128, 0, 0, 0))
            setPadding(4, 3, 6, 3)
        }

        fun makeRow(label: String, valueInit: String): TextView
        {
            val labelTv = TextView(this).apply {
                text = label
                setTextColor(Color.WHITE)
                setShadowLayer(0f, 1f, 1f, shadowColor)
                textSize = 11f
                typeface = tahoma
            }

            val spacer = View(this).apply {
                layoutParams = LinearLayout.LayoutParams(0, 0, 1f)
            }

            val valueTv = TextView(this).apply {
                text = valueInit
                setTextColor(Color.WHITE)
                setShadowLayer(0f, 1f, 1f, shadowColor)
                textSize = 11f
                typeface = tahoma
                layoutParams = LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.WRAP_CONTENT,
                    LinearLayout.LayoutParams.WRAP_CONTENT
                ).apply { rightMargin = (4 * density).toInt() }
            }

            val row = LinearLayout(this).apply {
                layoutParams = LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT,
                    LinearLayout.LayoutParams.WRAP_CONTENT
                ).apply {
                    if (rowCount > 0) topMargin = (1 * density).toInt()
                }
                orientation = LinearLayout.HORIZONTAL
                addView(labelTv)
                addView(spacer)
                addView(valueTv)
            }

            overlayLayout?.addView(row)
            rowCount++
            return valueTv
        }

        timeValueViews = mutableListOf(makeRow("Time:", "-:-- / -:--"))
        fpsValueView = makeRow("FPS:", "0")
        makeRow("Score:", "N/A")

        val overlayWidth = (135 * density).toInt()
        val params = FrameLayout.LayoutParams(
            overlayWidth,
            FrameLayout.LayoutParams.WRAP_CONTENT,
            Gravity.TOP or Gravity.END
        ).apply {
            //rightMargin = (15 * density).toInt()
            rightMargin = 0 // Terrible for phones that have rounded corners but it also triggers my OCD
            topMargin = 0
        }
        addContentView(overlayLayout, params)
    }

    private fun hideSystemBars()
    {
        @Suppress("DEPRECATION")
        window.decorView.systemUiVisibility = (
            View.SYSTEM_UI_FLAG_FULLSCREEN or
            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY or
            View.SYSTEM_UI_FLAG_LAYOUT_STABLE or
            View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION or
            View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
        )
    }

    private fun extractAssets()
    {
        val assetFiles = listOf("piano_maganda.sf2", "gm_generic.sf2")
        val filesDir = filesDir

        for(filename in assetFiles)
        {
            val outFile = File(filesDir, filename)
            if(outFile.exists())
            {
                Log.d(TAG, "File already extracted: $filename")
                continue
            }
            try {
                assets.open(filename).use { is_ ->
                    FileOutputStream(outFile).use { fos ->
                        val buffer = ByteArray(8192)
                        var read: Int
                        while(is_.read(buffer).also { read = it } != -1)
                        {
                            fos.write(buffer, 0, read)
                        }
                        fos.flush()
                    }
                }
                Log.d(TAG, "Successfully extracted: $filename")
            }
            catch(e: IOException)
            {
                Log.e(TAG, "Failed to extract $filename", e)
            }
        }

        Log.d(TAG, "Files directory contents:")
        filesDir.listFiles()?.forEach { file ->
            Log.d(TAG, " - ${file.name}")
        }
    }

    private fun requestStorageAccess()
    {
        when {
            Build.VERSION.SDK_INT >= 30 ->
            {
                if(!Environment.isExternalStorageManager())
                {
                    val intent = Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION).apply {
                        data = Uri.parse("package:$packageName")
                    }
                    startActivityForResult(intent, REQUEST_MANAGE_EXTERNAL_STORAGE)
                }
                else
                {
                    initializeApp()
                }
            }
            Build.VERSION.SDK_INT >= 23 ->
            {
                if(checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED ||
                    checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED
                ){
                    requestPermissions(
                        arrayOf(Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE),
                        REQUEST_STORAGE_PERMISSION
                    )
                }
                else
                {
                    initializeApp()
                }
            }
            else -> initializeApp()
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray)
    {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if(requestCode == REQUEST_STORAGE_PERMISSION)
        {
            if(grantResults.size > 1 && grantResults[0] == PackageManager.PERMISSION_GRANTED && grantResults[1] == PackageManager.PERMISSION_GRANTED)
            {
                initializeApp()
            }
            else
            {
                Log.w(TAG, "Storage permissions were not granted.")
            }
        }
    }

    @Deprecated("Deprecated in Java", ReplaceWith("registerForActivityResult"))
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?)
    {
        super.onActivityResult(requestCode, resultCode, data)
        if(requestCode == REQUEST_MANAGE_EXTERNAL_STORAGE)
        {
            if(Environment.isExternalStorageManager())
            {
                initializeApp()
            }
            else
            {
                Log.w(TAG, "MANAGE_EXTERNAL_STORAGE permission was not granted.")
            }
        }
    }

    private fun initializeApp()
    {
        Log.d(TAG, "NvpfaActivity initialized with storage permissions.")
    }
}
