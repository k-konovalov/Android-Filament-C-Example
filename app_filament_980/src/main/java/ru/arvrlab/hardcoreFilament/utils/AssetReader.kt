package ru.arvrlab.hardcoreFilament.utils

import android.content.Context
import java.io.File

object AssetReader {
    fun getFileFromAssets(context: Context, fileName: String, assetsPath: String): File = File(context.cacheDir, fileName)
        .also {
            if (!it.exists()) {
                it.outputStream().use { cache ->
                    context.assets
                        .open(assetsPath + fileName)
                        .use { inputStream -> inputStream.copyTo(cache) }
                }
            }
        }
}