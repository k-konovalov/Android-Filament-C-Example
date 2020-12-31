package ru.arvrlab.hardcoreFilament.ui

import android.Manifest
import android.annotation.SuppressLint
import android.graphics.SurfaceTexture
import android.hardware.Camera
import android.os.Bundle
import android.util.Log
import android.util.Size
import android.view.*
import android.view.Choreographer.FrameCallback
import android.widget.*
import android.widget.AdapterView.OnItemSelectedListener
import android.widget.SeekBar.OnSeekBarChangeListener
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.doOnLayout
import org.xmlpull.v1.XmlPullParserException
import ru.arvrlab.hardcoreFilament.*
import ru.arvrlab.hardcoreFilament.filament.FilamentHelper
import ru.arvrlab.hardcoreFilament.filament.FilamentHelper.RendererCallback
import ru.arvrlab.hardcoreFilament.filament.HelloFilament
import ru.arvrlab.hardcoreFilament.filament.Material
import java.io.IOException
import java.util.*

/*
* Copyright (C) 2015 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
class MainActivity : AppCompatActivity() {
    private enum class StreamMode { SURFACE_TEXTURE, TEXTURE_ID }
    private enum class SurfaceMode { SURFACE_VIEW, TEXTURE_VIEW }

    private open inner class AbstractChangeAdapter : OnSeekBarChangeListener {
        override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {}
        override fun onStartTrackingTouch(seekBar: SeekBar) {}
        override fun onStopTrackingTouch(seekBar: SeekBar) {}
    }

    internal abstract class RenderItem(private val mName: CharSequence) : Comparable<RenderItem?> {
        override operator fun compareTo(o: RenderItem?): Int {
            return mName.toString().compareTo(o.toString())
        }

        override fun toString(): String {
            return mName.toString()
        }
    }
    internal class MeshItem(name: CharSequence) : RenderItem(name)
    internal class ModelItem(name: CharSequence) : RenderItem(name)

    private val viewModel: MainViewModel by viewModels()

    private var renderView: View? = null
    private var mSurfaceMode =
        SurfaceMode.SURFACE_VIEW
    private val surfaceSize = Size(1920, 1080)

    private val mChoreographer by lazy {Choreographer.getInstance()}
    private val mEditPanel by lazy { findViewById<ViewGroup>(R.id.material_controls)}
    //size of the render view (which we'll use as the native size)
    private val mNativeWidth by lazy { renderView?.width ?: 640 }
    private val mNativeHeight by lazy { renderView?.height ?: 480 }
    private val mMaterials = HashMap<String, Material>()
    private val mMaterial = Material()
    private var mMaterialNeedsUpdate = false
    private var mObjectRotation = true
    private var mCameraRotation = false
    private var mCameraSurfaceTexture = SurfaceTexture(0)
    private var mCamera: Camera? = null
    private var mUseCameraTexture = false

    private val meshNames: Array<String>?
        get() = try {
            assets.list("mesh")?.apply {
                /*forEachIndexed { index, s ->
                    val mesh = this[index]
                    this[index] = mesh.substring(mesh.lastIndexOf('/') + 1, mesh.lastIndexOf('.'))
                }*/
                sort()
            } ?: arrayOf("No mesh found")
        } catch (e: IOException) {
            arrayOf("No mesh found")
        }
    private val modelNames: Array<String>?
        get() {
            return try {
                assets.list("models")?.apply { sort() } ?: arrayOf("No mesh found")
            } catch (e: IOException) {
                arrayOf("No mesh found")
            }
        }
    private val envNames: Array<String>?
        get() = try {
                assets.list("env")?.apply { sort() }
            } catch (e: IOException) {
                arrayOf("No env found")
            }

    private val materialNames: Array<String>
        get() {
            val names = mMaterials.keys.toTypedArray()
            Arrays.sort(names)
            return names
        }

    companion object {
        private val mStreamMode =
            StreamMode.SURFACE_TEXTURE
        const val EGL_CONTEXT_OPENGL_NO_ERROR_KHR = 0x31B3
        const val EGL_OPENGL_ES3_BIT = 0x40
        private const val mCameraTexture: Long = 0
        private const val LOG_TAG = "filament.direct"
        private const val MSAA_SAMPLE_COUNT = 1
        // in this sample code, we're keeping FilamentHelper around when the activity stops
        // instead, we could call for e.g. terminate() in onDestroy().
        private var sFilamentHelper =
            FilamentHelper(FilamentHelper.ContextErrorPolicy.DONT_CHECK)
    }

    private val rendererCallback = object: RendererCallback{
        override fun onNativeWindowChanged(surface: Surface?) {
            HelloFilament.setSwapChain(surface)
        }

        override fun onDetachedFromSurface() {
            HelloFilament.finish()
        }

        override fun onResized(width: Int, height: Int) {
            HelloFilament.resize(width, height)
        }

    }

    private val frameCallback = object : FrameCallback{
        override fun doFrame(p0: Long) {
            mChoreographer.postFrameCallback(this)
            if (sFilamentHelper.isReadyToRender) {
                if (mStreamMode == StreamMode.TEXTURE_ID && mUseCameraTexture) mCameraSurfaceTexture.updateTexImage()
                if (mMaterialNeedsUpdate) {
                    mMaterialNeedsUpdate = false
                    val material = mMaterial
                    HelloFilament.updateMaterial(
                        material.metallic,
                        material.roughness,
                        material.clearCoat
                    )
                    HelloFilament.updateMaterialAlbedo(
                        material.albedo[0],
                        material.albedo[1],
                        material.albedo[2]
                    )
                }
                HelloFilament.render(true, false)

                        //mObjectRotation, mCameraRotation)
            }
        }
    }

    @SuppressLint("ClickableViewAccessibility")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        setContentView(R.layout.activity_main)

        //Cam Permissions
        requestPermissions(arrayOf(Manifest.permission.CAMERA), 0)
        initUI()
        setupFilamentHelper()
        // in this demo Engine doesn't shut down, and won't be recreeated in init(),
        // so the sharedContext parameter is ignored
        HelloFilament.init(
            MSAA_SAMPLE_COUNT,
            0,
            mStreamMode == StreamMode.SURFACE_TEXTURE
        )
        if (mStreamMode == StreamMode.SURFACE_TEXTURE) {
            // this is to emulate API 26, which allows to start detached.
            mCameraSurfaceTexture.detachFromGLContext()
        } else {
            mCameraSurfaceTexture = SurfaceTexture(mCameraTexture.toInt())
        }

        renderView?.doOnLayout {
            viewModel.loadEnvironment(assets, "")
            viewModel.loadModel(assets, "cube_1m_centered.glb")
            HelloFilament.updateTransform()
            /*AssetReader.getFileFromAssets(applicationContext, "wolf_centered_3.glb", "models/")
                .run {
                    val modelSize = readBytes().size
                    if (modelSize > 0) viewModel.loadModel(assets, absolutePath)
                }*/
        }
    }

    override fun onResume() {
        Log.d(LOG_TAG, "onResume")
        super.onResume()
        sFilamentHelper.setDesiredSize(mNativeWidth, mNativeHeight)
        mChoreographer.postFrameCallback(frameCallback)
        if (mUseCameraTexture) startCameraPreview()
    }

    override fun onPause() {
        // Activity paused, but still visible
        Log.d(LOG_TAG, "onPause")
        super.onPause()
        mChoreographer.removeFrameCallback(frameCallback)
        mCamera?.stopCameraPreview()
    }

    override fun onDestroy() {
        // Activity no longer needed (e.g. pressed [back])
        /*
        // In this demo, we don't (can't) destroy our context because we're also keeping filament
        // around and there would be no way to create a new context and share it with filament's

        int[] tex = {(int) mCameraTexture};
        GLES30.glDeleteTextures(1, tex, 0);
        EGLDisplay dpy = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);
        EGLSurface sur = EGL14.eglGetCurrentSurface(EGL14.EGL_READ);
        EGLContext ctx = EGL14.eglGetCurrentContext();
        EGL14.eglMakeCurrent(dpy, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_CONTEXT);
        EGL14.eglDestroySurface(dpy, sur);
        EGL14.eglDestroyContext(dpy, ctx);
        EGL14.eglTerminate(dpy);
        EGL14.eglReleaseThread();
        */
        Log.d(LOG_TAG, "onDestroy")
        super.onDestroy()
        HelloFilament.destroy()
        sFilamentHelper.detach()
        mCameraSurfaceTexture.release()
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menuInflater.inflate(R.menu.main_menu, menu);
        menu.findItem(R.id.res_720).isChecked = true;
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        when (item.itemId) {
            R.id.camera -> {
                mUseCameraTexture = !mUseCameraTexture
                if (mUseCameraTexture) {
                    startCameraPreview()
                } else {
                    mCamera?.stopCameraPreview()
                }
            }
            R.id.surface -> {
                sFilamentHelper.detach()
                var isTextureViewNeeded = true
                if (mSurfaceMode == SurfaceMode.SURFACE_VIEW) {
                    mSurfaceMode =
                        SurfaceMode.TEXTURE_VIEW
                    item.setTitle(R.string.textureview)
                    isTextureViewNeeded = false
                } else if (mSurfaceMode == SurfaceMode.TEXTURE_VIEW) {
                    mSurfaceMode =
                        SurfaceMode.SURFACE_VIEW
                    item.setTitle(R.string.surfaceview)
                    isTextureViewNeeded = true
                }
                toggleRenderSurface(isTextureViewNeeded)
            }
            R.id.action_edit_material -> mEditPanel.toggleMaterialControls()
            R.id.res_native -> {
                item.isChecked = true
                sFilamentHelper.setDesiredSize(mNativeWidth, mNativeHeight)
            }
            R.id.res_1080 -> {
                item.isChecked = true
                sFilamentHelper.setDesiredSize(1920, 1080)
            }
            R.id.res_720 -> {
                item.isChecked = true
                sFilamentHelper.setDesiredSize(1280, 720)
            }
            R.id.res_sd -> {
                item.isChecked = true
                sFilamentHelper.setDesiredSize(720, 576)
            }
        }
        return super.onOptionsItemSelected(item)
    }

    private fun initUI() {
        renderView = findViewById<SurfaceView>(R.id.render_surface) as View
        mEditPanel.visibility = View.INVISIBLE
        initEnvironmentSpinner()
        //initMeshSpinner();
        initModelSpinner()
        initMaterials()
        setupSpinnersUI()
    }

    private fun initEnvironmentSpinner() {
        val envNames = envNames ?: return
        findViewById<Spinner>(R.id.environment).run {
            adapter = ArrayAdapter(this@MainActivity, android.R.layout.simple_spinner_item, envNames).apply { this.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)  }
            onItemSelectedListener = object : OnItemSelectedListener {
                override fun onItemSelected(
                    parent: AdapterView<*>,
                    view: View,
                    position: Int,
                    id: Long
                ) {
                    val env = parent.adapter.getItem(position).toString()
                    viewModel.loadEnvironment(assets, env)
                }

                override fun onNothingSelected(parent: AdapterView<*>?) {}
            }
            setSelection(Arrays.binarySearch(envNames, "river"))
        }
    }

    private fun initModelSpinner() {
        val meshNames = meshNames ?: return
        val modelNames = modelNames ?: return
        val list: MutableList<RenderItem> = ArrayList()

        meshNames.forEach { list.add(
            MeshItem(
                it
            )
        ) }
        modelNames.forEach { list.add(
            ModelItem(
                it
            )
        ) }
        list.sort()

        findViewById<Spinner>(R.id.mesh).run {
            adapter = ArrayAdapter(this@MainActivity, android.R.layout.simple_spinner_item, list).apply { setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item) }
            onItemSelectedListener = object : OnItemSelectedListener {
                override fun onItemSelected(
                    parent: AdapterView<*>,
                    view: View,
                    position: Int,
                    id: Long
                ) {
                    val item = parent.adapter.getItem(position) as RenderItem
                    val status: Boolean
                    if (item is MeshItem) {
                        //viewModel.loadMesh(assets, item.toString())
                        status = true
                    } else {
                        mUseCameraTexture = false
                        mCamera?.stopCameraPreview()

                        //viewModel.loadModel(assets, item.toString())
                        status = false
                    }
                    findViewById<View>(R.id.material_metallic)?.run{ isEnabled = status}
                    findViewById<View>(R.id.material_roughness)?.run{ isEnabled = status}
                    findViewById<View>(R.id.material_clearcoat)?.run{ isEnabled = status}
                    findViewById<View>(R.id.color)?.run{isEnabled = status}
                }

                override fun onNothingSelected(parent: AdapterView<*>?) {}
            }
            setSelection(Arrays.binarySearch(meshNames, "monkey"))
        }
    }

    private fun initMaterials() {
        try {
            val parser = resources.getXml(R.xml.material)
            viewModel.inflate(parser, mMaterials)
            parser.close()
        } catch (e: IOException) {
            Log.w(LOG_TAG, "An error occured while loading the bookstores", e)
        } catch (e: XmlPullParserException) {
            Log.w(LOG_TAG, "An error occured while loading the bookstores", e)
        }
        val materialNames = materialNames

        findViewById<Spinner>(R.id.color).run {
            adapter = ArrayAdapter(this@MainActivity, android.R.layout.simple_spinner_item, materialNames).apply { setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item) }
            onItemSelectedListener = object : OnItemSelectedListener {
                override fun onItemSelected(parent: AdapterView<*>, view: View, position: Int, id: Long) {
                    mMaterials[parent.adapter.getItem(position).toString()]?.run {
                        mMaterialNeedsUpdate = true
                        mMaterial.albedo[0] = albedo[0]
                        mMaterial.albedo[1] = albedo[1]
                        mMaterial.albedo[2] = albedo[2]
                        mMaterial.metallic = metallic
                        mMaterial.roughness = roughness
                        mMaterial.clearCoat = clearCoat
                    }

                    updateUi()
                }

                override fun onNothingSelected(parent: AdapterView<*>?) {}
            }
            setSelection(Arrays.binarySearch(materialNames, "Silver"))
        }
    }

    private fun updateUi() {
        findViewById<SeekBar>(R.id.material_metallic).run { progress = (mMaterial.metallic * max).toInt() }
        findViewById<SeekBar>(R.id.material_roughness).run { progress = (mMaterial.roughness * max).toInt() }
    }

    private fun setupSpinnersUI(){
        val material = mMaterial
        val metallic = findViewById<SeekBar>(R.id.material_metallic)
        val roughness = findViewById<SeekBar>(R.id.material_roughness)
        val clearCoat = findViewById<SeekBar>(R.id.material_clearcoat)

        metallic.run {
            progress = (material.metallic * metallic.max).toInt()
            setOnSeekBarChangeListener(object : AbstractChangeAdapter() {
                override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                    mMaterialNeedsUpdate = true
                    material.metallic = progress / seekBar.max.toFloat()
                }
            })
        }
        roughness.run {
            progress = (material.roughness * roughness.max).toInt()
            setOnSeekBarChangeListener(object : AbstractChangeAdapter() {
                override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                    mMaterialNeedsUpdate = true
                    material.roughness = progress / seekBar.max.toFloat()
                }
            })
        }
        clearCoat.run {
            progress = (material.clearCoat * clearCoat.max).toInt()
            setOnSeekBarChangeListener(object : AbstractChangeAdapter() {
                override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                    mMaterialNeedsUpdate = true
                    material.clearCoat = progress / seekBar.max.toFloat()
                }
            })
        }

        findViewById<CheckBox>(R.id.material_object).run {
            isChecked = mObjectRotation
            setOnCheckedChangeListener { buttonView: CompoundButton?, isChecked: Boolean ->
                mObjectRotation = isChecked
            }
        }
        findViewById<CheckBox>(R.id.material_camera).run {
            isChecked = mCameraRotation
            setOnCheckedChangeListener { _: CompoundButton?, isChecked: Boolean ->
                mCameraRotation = isChecked
            }
        }
    }

    private fun setupFilamentHelper() {
        sFilamentHelper.run {
            setRenderCallback(rendererCallback)
            setDesiredSize(surfaceSize.width, surfaceSize.height)
            attachTo(renderView as SurfaceView)
        }
    }

    //--------------  Camera section
    private fun startCameraPreview() {
        if (mCamera == null) {
            mCamera = Camera.open()
            mCamera?.run{
                var size = Size(0,0)
                parameters = parameters.let{
                    val p = it
                    val s = p.supportedPreviewSizes[0]
                    size = s
                    p.setPreviewSize(s.width, s.height)
                    p
                }
                //Try write camera stream to texture
                try { setPreviewTexture(mCameraSurfaceTexture) } catch (e: IOException) {e.printStackTrace() }
                startPreview()
                Toast.makeText(this@MainActivity,"Camera started", Toast.LENGTH_SHORT).show()
                if (mStreamMode == StreamMode.SURFACE_TEXTURE) HelloFilament.setCameraStream(
                    mCameraSurfaceTexture
                )
                else HelloFilament.setCameraStreamWithTexture(
                    mCameraTexture,
                    size.width,
                    size.height
                )
            }
        }
    }

    private fun Camera.stopCameraPreview() {
        if (mStreamMode == StreamMode.SURFACE_TEXTURE) {
            HelloFilament.setCameraStream(null)
        } else {
            HelloFilament.setCameraStreamWithTexture(
                0,
                0,
                0
            )
        }
            stopPreview()
            Toast.makeText(this@MainActivity,"Camera stopped", Toast.LENGTH_SHORT).show()
            try { setPreviewTexture(null) } catch (e: IOException) {
                e.printStackTrace()
            }
            release()
            mCamera = null
    }

    @SuppressLint("ClickableViewAccessibility")
    private fun toggleRenderSurface(isTextureViewNeeded: Boolean) {
        // remove render surface from hierarchy
        val parent = renderView?.parent as ViewGroup
        parent.removeView(renderView)

        // add a new render surface to the hierarchy
        if (isTextureViewNeeded) parent.addView(TextureView(this).apply { id =
            R.id.render_surface
        })
        else parent.addView(SurfaceView(this).apply { id =
            R.id.render_surface
        })

        renderView = parent.findViewById(R.id.render_surface)
        when(renderView){
            is SurfaceView -> sFilamentHelper.attachTo(renderView as SurfaceView)
            is TextureView -> sFilamentHelper.attachTo(renderView as TextureView)
        }

        Toast.makeText(this@MainActivity,"RenderView re-attached as ${if (renderView is TextureView) "TextureView" else "SurfaceView"}", Toast.LENGTH_SHORT).show()
    }

    private fun View.toggleMaterialControls() {
        if (visibility == View.VISIBLE) hideMaterialControls()
        else showMaterialControls()
    }

    private fun View.showMaterialControls() {
        visibility = View.VISIBLE
        translationY = height.toFloat()
        animate().translationY(0f).start()
    }

    private fun View.hideMaterialControls() {
        animate()
            .translationY(height.toFloat())
            .withEndAction { visibility = View.INVISIBLE }
            .start()
    }
}