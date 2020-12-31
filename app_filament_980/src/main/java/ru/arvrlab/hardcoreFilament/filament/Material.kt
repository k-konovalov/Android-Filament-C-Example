package ru.arvrlab.hardcoreFilament.filament

data class Material (
    var metallic: Float = 1.0f,
    var roughness: Float = 0.7f,
    var clearCoat: Float = 0.0f,
    val albedo: FloatArray = FloatArray(3)
)