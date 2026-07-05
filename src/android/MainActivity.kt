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
import org.libsdl.app.SDLActivity
import java.io.File
import java.io.FileOutputStream
import java.io.IOException

class NvpfaActivity : SDLActivity()
{

    companion object
    {
        private const val REQUEST_STORAGE_PERMISSION = 1
        private const val REQUEST_MANAGE_EXTERNAL_STORAGE = 2
        private const val TAG = "NvpfaActivity"
    }

    override fun onCreate(savedInstanceState: Bundle?)
    {
        super.onCreate(savedInstanceState)

        extractAssets()
        requestStorageAccess()
    }

    private fun extractAssets()
    {
        val assetFiles = listOf("piano_maganda.sf2")
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
