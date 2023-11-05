package ru.arvrlab.hardcoreFilament.ui

import android.content.res.AssetManager
import android.content.res.XmlResourceParser
import androidx.lifecycle.ViewModel
import org.xmlpull.v1.XmlPullParser
import org.xmlpull.v1.XmlPullParserException
import ru.arvrlab.hardcoreFilament.filament.HelloFilament
import ru.arvrlab.hardcoreFilament.filament.Material
import java.io.IOException
import java.nio.ByteBuffer

class MainViewModel: ViewModel() {
    fun loadEnvironment() {
        HelloFilament.loadLight()
    }

    fun loadMesh(assets: AssetManager, mesh: String) {
        HelloFilament.loadMesh(
            assets,
            "models/$mesh"
        )//.filamesh
    }

    fun loadModel(assets: AssetManager, model: String) {
        val buffer = assets.open("models/$model").use { input ->
            val bytes = ByteArray(input.available())
            input.read(bytes)
            ByteBuffer.wrap(bytes)
        }

        HelloFilament.loadGlbModelWith(
            buffer,
            buffer.remaining()
        )
    }

    @Throws(IOException::class, XmlPullParserException::class)
    fun inflate(
        parser: XmlResourceParser,
        materials: HashMap<String, Material>
    ) {
        var type: Int
        while (parser.next().also {
                type = it
            } != XmlPullParser.START_TAG && type != XmlPullParser.END_DOCUMENT) { /* Empty */
        }
        if (type == XmlPullParser.START_TAG) {
            // START_TAG must be "materials"
            if (parser.name == "materials") {
                while (parser.next().also {
                        type = it
                    } != XmlPullParser.END_TAG && type != XmlPullParser.END_DOCUMENT) {
                    if (type == XmlPullParser.START_TAG && parser.name == "material") {
                        val materialName = parser.getAttributeValue(null, "name")
                        val material =
                            Material()
                        while (parser.next().also {
                                type = it
                            } != XmlPullParser.END_TAG && type != XmlPullParser.END_DOCUMENT) {
                            if (type == XmlPullParser.START_TAG) {
                                val name = parser.name
                                if (name == "albedo") {
                                    while (parser.next().also {
                                            type = it
                                        } != XmlPullParser.END_TAG && type != XmlPullParser.END_DOCUMENT) {
                                        if (type == XmlPullParser.TEXT) {
                                            val albedoText = parser.text
                                            val albedoChannels =
                                                albedoText.split(",").toTypedArray()
                                            if (albedoChannels.size >= 3) {
                                                material.albedo[0] = albedoChannels[0].toFloat()
                                                material.albedo[1] = albedoChannels[1].toFloat()
                                                material.albedo[2] = albedoChannels[2].toFloat()
                                            }
                                        }
                                    }
                                }
                                if (name == "metallic") {
                                    while (parser.next().also {
                                            type = it
                                        } != XmlPullParser.END_TAG && type != XmlPullParser.END_DOCUMENT) {
                                        if (type == XmlPullParser.TEXT) {
                                            material.metallic = parser.text.toFloat()
                                        }
                                    }
                                }
                                if (name == "roughness") {
                                    while (parser.next().also {
                                            type = it
                                        } != XmlPullParser.END_TAG && type != XmlPullParser.END_DOCUMENT) {
                                        if (type == XmlPullParser.TEXT) {
                                            material.roughness = parser.text.toFloat()
                                        }
                                    }
                                }
                                if (name == "clearCoat") {
                                    while (parser.next().also {
                                            type = it
                                        } != XmlPullParser.END_TAG && type != XmlPullParser.END_DOCUMENT) {
                                        if (type == XmlPullParser.TEXT) {
                                            material.clearCoat = parser.text.toFloat()
                                        }
                                    }
                                }
                            }
                        }
                        materials[materialName] = material
                    }
                }
            }
        }
    }

}