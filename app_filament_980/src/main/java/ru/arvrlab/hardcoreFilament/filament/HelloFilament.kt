package ru.arvrlab.hardcoreFilament.filament

import android.content.res.AssetManager
import android.graphics.SurfaceTexture
import android.view.Surface
import java.nio.ByteBuffer

object HelloFilament {
    init {
        System.loadLibrary("hello_filament")
    }

    external fun init(msaaSampleCount: Int, sharedContext: Long, useSurfaceTexture: Boolean)

    external fun loadIbl(assets: AssetManager?, name: String?)
    external fun loadMesh(assets: AssetManager?, name: String?)
    external fun loadGlbModel(assets: AssetManager?, name: String?)
    external fun loadGlbModelWith(buffer: ByteBuffer?, remaining: Int)
    external fun resize(width: Int, height: Int)
    external fun destroy()
    external fun render(objectRotation: Boolean, cameraRotation: Boolean)

    external fun updateTransform()
    external fun updateMaterial(metallic: Float, roughness: Float, reflectance: Float)

    external fun updateMaterialAlbedo(r: Float, g: Float, b: Float)

    external fun setSwapChain(nativeWindow: Surface?)
    external fun setCameraStream(st: SurfaceTexture?)
    external fun setCameraStreamWithTexture(cameraTexture: Long, width: Int, height: Int)

    external fun finish()
}