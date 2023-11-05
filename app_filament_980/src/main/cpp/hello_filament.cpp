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

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <fstream>

#include <jni.h>

#include <android/native_window_jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>

// #include "filamentapp/IBL.h"
#include <filamat/MaterialBuilder.h>
#include <filament/Box.h>
#include <filament/Camera.h>
#include <filament/Color.h>
#include <filament/Engine.h>
#include <filament/Fence.h>
#include <filament/IndexBuffer.h>
#include <filament/IndirectLight.h>
#include <filament/LightManager.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <filament/RenderableManager.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/Skybox.h>
#include <filament/Stream.h>
#include <filament/SwapChain.h>
#include <filament/Texture.h>
#include <filament/TextureSampler.h>
#include <filament/TransformManager.h>
#include <filament/VertexBuffer.h>
#include <filament/View.h>
#include <filament/Viewport.h>

#include <math/mat4.h>
#include <math/vec3.h>

#include <utils/EntityManager.h>
#include "utils/JobSystem.h"
#include <gltfio/MaterialProvider.h>
#include <gltfio/ResourceLoader.h>
#include <gltfio/AssetLoader.h>
#include <math/mat2.h>

#include "common/NioUtils.h"
#include "utils/Path.h"

#include "stb_image.h"

#define FILAMENT_TAG "HelloFilament"
#define LOGD(...) (__android_log_print(ANDROID_LOG_DEBUG, FILAMENT_TAG, __VA_ARGS__))

using namespace filament;
using namespace filamat;
using namespace math;
using namespace utils;
using namespace gltfio;

static Texture::Sampler STREAM_SAMPLER_TYPE = Texture::Sampler::SAMPLER_EXTERNAL;

static constexpr float ONE_RPM = (2 * float(M_PI) / 60);
static float const g_omega = ONE_RPM / 16;
static float g_angle = 0;

// Filament resources
static Engine* g_engine = nullptr;
static Renderer* g_renderer = nullptr;
static SwapChain* g_swapChain = nullptr;
static Stream* g_camera_stream = nullptr;

static const Material* g_default_material = nullptr;
static MaterialInstance* g_default_mi = nullptr;

static const Material* g_textured_material = nullptr;
static MaterialInstance* g_textured_mi = nullptr;

static Scene* g_scene = nullptr;
gltfio::FilamentAsset* filamentAsset;
static Entity currentModel;
static Entity g_light;
static Entity g_light1;
static Entity g_light2;
static Entity g_light3;
static Entity g_light4;
// static IBL* g_ibl = nullptr;

static View* g_view = nullptr;
static Entity g_camera_entity;
static const Material* g_camera_material = nullptr;
static MaterialInstance* g_camera_mi = nullptr;
static Camera* g_camera;

struct Mesh;
static constexpr size_t MESH_COUNT = 1;
static std::vector<Mesh *> g_meshes;

struct Header {
    uint32_t version;
    uint32_t parts;
    Box      aabb;
    uint32_t interleaved;
    uint32_t offsetPosition;
    uint32_t stridePosition;
    uint32_t offsetTangents;
    uint32_t strideTangents;
    uint32_t offsetColor;
    uint32_t strideColor;
    uint32_t offsetUV0;
    uint32_t strideUV0;
    uint32_t offsetUV1;
    uint32_t strideUV1;
    uint32_t vertexCount;
    uint32_t vertexSize;
    uint32_t indexType;
    uint32_t indexCount;
    uint32_t indexSize;
};

struct Part {
    uint32_t offset;
    uint32_t indexCount;
    uint32_t minIndex;
    uint32_t maxIndex;
    uint32_t materialID;
    Box      aabb;
};

struct Mesh {
    Entity renderable;
    VertexBuffer* vertexBuffer = nullptr;
    IndexBuffer* indexBuffer = nullptr;
    Texture* textures[5] = {nullptr, nullptr, nullptr, nullptr};
};

static void setParameterFromAsset(Texture **texture, AAssetManager *assetManager, const Path &path,
                                  std::string name, TextureSampler const &sampler,
                                  Texture::InternalFormat internalFormat);

static Mesh* decodeMesh(void const* data, off_t offset, MaterialInstance* mi);

static Texture *decodeTexture(void const *data, off_t len, Texture::InternalFormat internalFormat);


static void destroyMesh(Mesh *mesh) {
    g_engine->destroy(mesh->vertexBuffer);
    g_engine->destroy(mesh->indexBuffer);
    g_engine->destroy(mesh->renderable);
    EntityManager::get().destroy(mesh->renderable);

    for (auto &texture : mesh->textures) {
        if (texture) {
            g_engine->destroy(texture);
            texture = nullptr;
        }
    }
}

static void destroyMeshes() {
    for (Mesh *mesh : g_meshes) {
        g_scene->remove(mesh->renderable);
        destroyMesh(mesh);
        delete mesh;
    }
    g_meshes.clear();
}

static std::ifstream::pos_type getFileSize(const char* filename) {
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}


extern "C" {

JNIEXPORT void JNICALL
Java_ru_arvrlab_hardcoreFilament_filament_HelloFilament_loadLight(JNIEnv *env, jobject type) {
    auto& em = utils::EntityManager::get();
    g_light = em.create();
    LightManager::Builder(LightManager::Type::SUN)
            .color(Color::toLinear<ACCURATE>(sRGBColor(0.98f, 0.92f, 0.89f)))
            .intensity(110000)
            .direction({ 0.7, -1, -0.8 })
            .sunAngularRadius(1.9f)
                    //.castShadows(ENABLE_SHADOWS)
            .build(*g_engine, g_light);
    g_scene->addEntity(g_light);
}

extern "C"
JNIEXPORT void JNICALL
Java_ru_arvrlab_hardcoreFilament_filament_HelloFilament_loadGlbModelWith(JNIEnv *env, jobject clazz, jobject buffer, jint remaining) {
    // TODO: implement loadGlbModelWith()
    if (filamentAsset && currentModel) {
        filamentAsset->releaseSourceData();
        g_scene->remove(currentModel);
        g_engine->destroy(currentModel);
        filamentAsset = nullptr;
    }

    //Create Asset Loader
    gltfio::MaterialProvider* materialProvider = gltfio::createJitShaderProvider(g_engine); // or createUbershaderProvider
    AssetLoader* loader = gltfio::AssetLoader::create({g_engine, materialProvider, nullptr});

    //Transofrm Buffer to Entities
    AutoBuffer buffer_auto(env, buffer, remaining);
    filamentAsset = loader->createAsset((const uint8_t *) buffer_auto.getData(), buffer_auto.getSize());
    gltfio::ResourceLoader({.engine = g_engine, .normalizeSkinningWeights = false})
            .loadResources(filamentAsset);

    currentModel = filamentAsset->getEntities()[0];
    g_scene->addEntity(currentModel);// One model
    //g_scene->addEntities(filamentAsset->getEntities(), filamentAsset->getEntityCount()); //Multiply model
}

void setParameterFromAsset(Texture **texture, AAssetManager *assetManager, const Path &path,
                           std::string name, TextureSampler const &sampler,
                           Texture::InternalFormat internalFormat) {
    Path p(Path::concat(path, name + ".png"));
    AAsset* asset = AAssetManager_open(assetManager, p.getPath().c_str(), AASSET_MODE_RANDOM);
    if (asset) {
        const void *data = AAsset_getBuffer(asset);
        if (data) {
            *texture = decodeTexture(data, AAsset_getLength(asset), internalFormat);
            g_textured_mi->setParameter(name.c_str(), *texture, sampler);
        }
        AAsset_close(asset);
    }
}

Texture *decodeTexture(void const *data, off_t len, Texture::InternalFormat internalFormat) {
    int w, h, n;

    int c = 0;
    Texture::Format format;
    switch (internalFormat) {
        case Texture::InternalFormat::RGBA8:
            format = Texture::Format::RGBA;
            c = 4;
            break;
        case Texture::InternalFormat::SRGB8:
        case Texture::InternalFormat::RGB8:
            format = Texture::Format::RGB;
            LOGD("WARNING: Some Filament backends do not yet support 3-component textures.");
            c = 3;
            break;
        case Texture::InternalFormat::R8:
            format = Texture::Format::R;
            c = 1;
            break;
        default:
            format = Texture::Format::RGBA;
            c = 4;
    }

    unsigned char* textureData = stbi_load_from_memory((const stbi_uc *) data, (int) len, &w, &h, &n, c);

    size_t size = (size_t) (w * h * n);
    Texture::PixelBufferDescriptor pb(
            textureData, size,
            format, Texture::Type::UBYTE,
            (Texture::PixelBufferDescriptor::Callback) &stbi_image_free);

    Texture* texture = Texture::Builder()
            .width(uint32_t(w))
            .height(uint32_t(h))
            .sampler(Texture::Sampler::SAMPLER_2D)
            .format(internalFormat)
            .build(*g_engine);

    texture->setImage(*g_engine, 0, std::move(pb));

    return texture;
}


JNIEXPORT void JNICALL Java_ru_arvrlab_hardcoreFilament_filament_HelloFilament_loadMesh(
        JNIEnv* env, jobject type, jobject assets, jstring name_) {
    AAssetManager *assetManager = AAssetManager_fromJava(env, assets);

    const char* name = env->GetStringUTFChars(name_, 0);
    AAsset* asset = AAssetManager_open(assetManager, name, AASSET_MODE_RANDOM);
    env->ReleaseStringUTFChars(name_, name);

    char const* data = (char const *) AAsset_getBuffer(asset);
    if (data) {
        destroyMeshes();
        Mesh* mesh = decodeMesh(data, 0, g_default_mi);

        mesh->textures[0] = Texture::Builder()
                .sampler(STREAM_SAMPLER_TYPE)
                .format(Texture::InternalFormat::RGBA8)
                .build(*g_engine);

        TextureSampler sampler(
                TextureSampler::MagFilter::LINEAR, TextureSampler::WrapMode::CLAMP_TO_EDGE);
        //g_camera_mi->setParameter("albedo", mesh->textures[0], sampler);

        g_meshes.push_back(mesh);
        Fence::waitAndDestroy(g_engine->createFence());
    }
    AAsset_close(asset);
}

Mesh* decodeMesh(void const* data, off_t offset, MaterialInstance* mi) {
    const char* p = (const char *) data + offset;

    Mesh* mesh = nullptr;
    char magic[9];
    memcpy(magic, (const char *) p, sizeof(char) * 8);
    magic[8] = '\0';
    p += sizeof(char) * 8;

    if (!strcmp("FILAMESH", magic)) {
        Header* header = (Header*) p;
        p += sizeof(Header);

        char const* vertexData = p;
        p += header->vertexSize;

        char const* indices = p;
        p += header->indexSize;

        Part* parts = (Part*) p;
        p += header->parts * sizeof(Part);

        uint32_t materialCount = (uint32_t) *p;
        p += sizeof(uint32_t);

        std::vector<std::string> partsMaterial;
        partsMaterial.resize(materialCount);

        for (size_t i = 0; i < materialCount; i++) {
            uint32_t nameLength = (uint32_t) *p;
            p += sizeof(uint32_t);

            partsMaterial[i] = p;
            p += nameLength + 1; // null terminated
        }

        mesh = new Mesh();

        mesh->indexBuffer = IndexBuffer::Builder()
                .indexCount(header->indexCount)
                .bufferType(header->indexType ? IndexBuffer::IndexType::USHORT
                                              : IndexBuffer::IndexType::UINT)
                .build(*g_engine);

        mesh->indexBuffer->setBuffer(
                *g_engine, IndexBuffer::BufferDescriptor(indices, header->indexSize));

        VertexBuffer::Builder vbb;
        vbb.vertexCount(header->vertexCount)
                .bufferCount(1)
                .normalized(VertexAttribute::TANGENTS);

        mesh->vertexBuffer = vbb
                .attribute(VertexAttribute::POSITION, 0, VertexBuffer::AttributeType::HALF4,
                           header->offsetPosition, uint8_t(header->stridePosition))
                .attribute(VertexAttribute::TANGENTS, 0, VertexBuffer::AttributeType::SHORT4,
                           header->offsetTangents, uint8_t(header->strideTangents))
                .attribute(VertexAttribute::UV0, 0, VertexBuffer::AttributeType::HALF2,
                           header->offsetUV0, uint8_t(header->strideUV0))
                .build(*g_engine);

        VertexBuffer::BufferDescriptor buffer(vertexData, header->vertexSize);
        mesh->vertexBuffer->setBufferAt(*g_engine, 0, std::move(buffer));

        mesh->renderable = EntityManager::get().create();

        RenderableManager::Builder builder(header->parts);
        builder.boundingBox(header->aabb);
        builder.castShadows(true);
        for (size_t i = 0; i < header->parts; i++) {
            builder.geometry(i, RenderableManager::PrimitiveType::TRIANGLES,
                             mesh->vertexBuffer, mesh->indexBuffer, parts[i].offset,
                             parts[i].minIndex, parts[i].maxIndex, parts[i].indexCount);
            builder.material(i, mi);
        }
        builder.build(*g_engine, mesh->renderable);

        auto& tcm = g_engine->getTransformManager();
        tcm.setTransform(tcm.getInstance(mesh->renderable), mat4f{mat3f{1.0}, float3{0.0f, 0.0f, -4.0f}});
        g_scene->addEntity(mesh->renderable);
    }
    return mesh;
}

JNIEXPORT void JNICALL Java_ru_arvrlab_hardcoreFilament_filament_HelloFilament_init(
        JNIEnv* env, jobject type, jint sampleCount, jlong sharedContext,
        jboolean useSurfaceTexture) {
    LOGD("Started");

    MaterialBuilder::init();

    Package default_material = MaterialBuilder()
            .name("My material")
            .material("void material (inout MaterialInputs material) {"
                      "  prepareMaterial(material);"
                      "  material.baseColor.rgb = float3(1.0, 0.0, 0.0);"
                      "}")
            .shading(MaterialBuilder::Shading::UNLIT)
            .targetApi(MaterialBuilder::TargetApi::OPENGL)
            .platform(MaterialBuilder::Platform::MOBILE)
            .build(g_engine->getJobSystem());

    if (default_material.isValid()) {
        std::cout << "Success!" << std::endl;
    }

    STREAM_SAMPLER_TYPE = useSurfaceTexture ? Texture::Sampler::SAMPLER_EXTERNAL
            : Texture::Sampler::SAMPLER_2D;

    if (g_engine == nullptr){
        // Engine is kept between activity launches
        #if defined (FILAMENT_DRIVER_SUPPORTS_VULKAN)
        g_engine = Engine::create(Engine::Backend::VULKAN, nullptr, nullptr);
        #else
        g_engine = Engine::create(Engine::Backend::DEFAULT, nullptr, (void *)sharedContext);
        #endif
    }

    // Create a simple colored material
    g_default_material = Material::Builder()
        .package(default_material.getData(), default_material.getSize())
        .build(*g_engine);

    g_default_mi = g_default_material->createInstance();
/*    g_default_mi->setParameter("albedo", float3{0.8f});
    g_default_mi->setParameter("metallic", 1.0f);
    g_default_mi->setParameter("roughness", 0.7f);
    g_default_mi->setParameter("clearCoat", 0.0f);*/

    g_textured_material = Material::Builder()
        .package(default_material.getData(), default_material.getSize())
        .build(*g_engine);

    g_textured_mi = g_textured_material->createInstance();

    g_camera_material = Material::Builder()
            .package(default_material.getData(), default_material.getSize())
            .build(*g_engine);

    g_camera_mi = g_camera_material->createInstance();
/*    g_camera_mi->setParameter("metallic", 1.0f);
    g_camera_mi->setParameter("roughness", 0.7f);
    g_camera_mi->setParameter("reflectance", 0.5f);*/

    auto& em = EntityManager::get();

    // Create a scene
    g_scene = g_engine->createScene();

    u_int32_t width = 1280;
    u_int32_t height = 720;

    // Create a camera looking at the origin
    g_camera_entity = em.create();
    g_camera = g_engine->createCamera(em.create());

    // Create a view that takes up the entire window
    g_view = g_engine->createView();
    g_view->setName("Main View");
    g_view->setScene(g_scene);
    g_view->setCamera(g_camera);

    //Models background
    g_renderer = g_engine->createRenderer();
    g_renderer->setClearOptions({
                                      .clearColor = {0.25f, 0.5f, 1.0f, 1.0f},
                                      .clear = true
                              });

    g_angle = 0;
}

JNIEXPORT void JNICALL Java_ru_arvrlab_hardcoreFilament_filament_HelloFilament_resize(
        JNIEnv* env, jobject type, jint width, jint height) {
    LOGD("Resizing native window to %d x %d", width, height);

    double ratio = double(width) / height;
    g_camera->setProjection(65.0, ratio, 0.1, 10.0, Camera::Fov::VERTICAL);
    g_view->setViewport({ 0, 0, uint32_t(width), uint32_t(height) });

    if (g_swapChain) {
        // TODO: should this be done by the engine, so it's synchronous with viewport updates?
        ANativeWindow *win = (ANativeWindow *) g_swapChain->getNativeWindow();
        int32_t format = ANativeWindow_getFormat(win);
        ANativeWindow_setBuffersGeometry(win, width, height, format);
    }
}

JNIEXPORT void JNICALL Java_ru_arvrlab_hardcoreFilament_filament_HelloFilament_destroy(
        JNIEnv* env, jobject type) {
    LOGD(">>> Destroy");

    destroyMeshes();

    g_engine->destroy(g_default_mi);
    g_engine->destroy(g_default_material);
    g_engine->destroy(g_textured_mi);
    g_engine->destroy(g_textured_material);
    g_engine->destroyCameraComponent(g_camera_entity);
    g_engine->destroy(g_camera_mi);
    g_engine->destroy(g_camera_material);
    g_engine->destroy(g_scene);
    g_engine->destroy(g_view);
    g_engine->destroy(g_camera_stream);

    g_engine->destroy(g_light);
    g_engine->destroy(g_light1);
    g_engine->destroy(g_light2);
    g_engine->destroy(g_light3);
    g_engine->destroy(g_light4);

    g_engine->destroy(g_renderer);


    g_default_mi = nullptr;
    g_default_material = nullptr;
    g_textured_mi = nullptr;
    g_textured_material = nullptr;
    g_camera = nullptr;
    g_camera_mi = nullptr;
    g_camera_material = nullptr;
    g_camera = nullptr;
    g_scene = nullptr;
    g_view = nullptr;
    g_camera_stream = nullptr;

    auto& em = EntityManager::get();
    em.destroy(g_light);

    g_renderer = nullptr;

    // We could destroy the engine, but we don't have to, it'll be reused next time
    // In fact we don't have to destroy any of the objects here (useful during screen rotation)
    // Note: this is an application decision.
    // Engine::destroy(&g_engine);
}

JNIEXPORT void JNICALL
Java_ru_arvrlab_hardcoreFilament_filament_HelloFilament_setSwapChain(JNIEnv *env, jobject type,
                                                                   jobject nativeWindow) {
    if (g_swapChain) {
        g_engine->destroy(g_swapChain);
        g_swapChain = nullptr;
    }
    ANativeWindow *win = ANativeWindow_fromSurface(env, nativeWindow);
    g_swapChain = g_engine->createSwapChain(win);
}

JNIEXPORT void JNICALL Java_ru_arvrlab_hardcoreFilament_filament_HelloFilament_render(
        JNIEnv *env, jobject type, jboolean objectRotation, jboolean cameraRotation) {

    if (!currentModel)
    {
        return;
    } else {
        auto &tcm = g_engine->getTransformManager();
        auto transform = mat4f::translation(float3{0, 0, -4}); //x,y,z translation
        //transform *= mat4f::scaling(float3{1,1,1});

        tcm.setTransform(tcm.getInstance(currentModel), transform);
    }

    //Rotation&translation via Matrix and Vectors
    if (objectRotation || cameraRotation) {
        g_angle += g_omega;
        auto r = mat4f::translation(float3{0, 0, -4});
        r *= mat4f::rotation(g_angle, float3{0.0f, 1.0f, 0.0f});
        if (objectRotation) {
            auto& tcm = g_engine->getTransformManager();
            tcm.setTransform(tcm.getInstance(currentModel), r);
        }
        if (cameraRotation) {
            auto c = mat4f::translation(float3{0, 0, 4});
            g_view->getCamera().setModelMatrix(r * c);
        }
    }

    /*if(!g_meshes.empty()){
        auto& tcm = g_engine->getTransformManager();
        tcm.setTransform(tcm.getInstance(g_meshes[0]->renderable), mat4f::translation(float3{0, 0, -4}));
    }*/

    if (g_renderer->beginFrame(g_swapChain)) {
        g_renderer->render(g_view);
        g_renderer->endFrame();
    }
}

JNIEXPORT void JNICALL
Java_ru_arvrlab_hardcoreFilament_filament_HelloFilament_finish(JNIEnv *env, jobject type) {
    if (g_engine) { // engine may have been destroyed already
        Fence::waitAndDestroy(g_engine->createFence());
    }
}

JNIEXPORT void JNICALL
Java_ru_arvrlab_hardcoreFilament_filament_HelloFilament_updateMaterial(JNIEnv *env, jobject type,
        jfloat metallic, jfloat roughness, jfloat clearCoat) {

    MaterialInstance *mi = g_camera_stream ? g_camera_mi : g_default_mi;
    mi->setParameter("metallic", metallic);
    mi->setParameter("roughness", roughness);
    mi->setParameter("clearCoat", clearCoat);
}

JNIEXPORT void JNICALL
Java_ru_arvrlab_hardcoreFilament_filament_HelloFilament_updateMaterialAlbedo(JNIEnv *env, jobject type,
        jfloat r, jfloat g, jfloat b) {

    MaterialInstance *mi = g_default_mi;
    mi->setParameter("albedo", Color::toLinear<ACCURATE>(sRGBColor{r, g, b}));
}

JNIEXPORT void JNICALL
Java_ru_arvrlab_hardcoreFilament_filament_HelloFilament_setCameraStream(
        JNIEnv *env, jobject type, jobject st) {

    if (g_camera_stream) {
        g_engine->destroy(g_camera_stream);
        g_camera_stream = nullptr;
    }

    if (st) {
        g_camera_stream = Stream::Builder()
                .stream(st)
                .build(*g_engine);
        g_meshes[0]->textures[0]->setExternalStream(*g_engine, g_camera_stream);
    }

    auto& rcm = g_engine->getRenderableManager();
    rcm.setMaterialInstanceAt(
            rcm.getInstance(g_meshes[0]->renderable), 0, st ? g_camera_mi : g_default_mi);
}


extern "C" JNIEXPORT void JNICALL
Java_ru_arvrlab_hardcoreFilament_filament_HelloFilament_setCameraStreamWithTexture(JNIEnv *env, jobject type,
        jlong cameraTexture, jint width, jint height) {
    if (g_camera_stream) {
        g_engine->destroy(g_camera_stream);
        g_camera_stream = nullptr;
    }

    if (cameraTexture) {
        g_camera_stream = Stream::Builder()
                .stream(reinterpret_cast<void *>(cameraTexture))
                .width((uint32_t) width)
                .height((uint32_t) height)
                .build(*g_engine);

        g_meshes[0]->textures[0]->setExternalStream(*g_engine, g_camera_stream);
    }

    auto& rcm = g_engine->getRenderableManager();
    rcm.setMaterialInstanceAt(
            rcm.getInstance(g_meshes[0]->renderable), 0, cameraTexture ? g_camera_mi : g_default_mi);
}

extern "C"
JNIEXPORT void JNICALL
Java_ru_arvrlab_hardcoreFilament_filament_HelloFilament_updateTransform(JNIEnv *env, jobject clazz) {
    /* Kotlin Base Code
        val tm = engine.transformManager
        var center = asset.boundingBox.center.let { v-> Float3(v[0], v[1], v[2]) }
        val halfExtent = asset.boundingBox.halfExtent.let { v-> Float3(v[0], v[1], v[2]) }
        val maxExtent = 2.0f * max(halfExtent)
        val scaleFactor = 2.0f / maxExtent
        center -= centerPoint / scaleFactor
        val transform = scale(Float3(scaleFactor)) * translation(-center)
        tm.setTransform(tm.getInstance(asset.root), transpose(transform).toFloatArray())
     */
    if(filamentAsset){
        TransformManager* tm = &g_engine->getTransformManager();
        auto boundingBoxCenter = filamentAsset->getBoundingBox().center();
        auto center = float3{boundingBoxCenter[0],boundingBoxCenter[1],boundingBoxCenter[2]};
        auto halfExtent = filamentAsset->getBoundingBox().extent(); // Todo: max of it
        float max = 0.0;
        //max of halfExtent
        if(halfExtent[0] > halfExtent[1] && halfExtent[0] > halfExtent[2]){
            max = halfExtent[0];
        }
        if(halfExtent[1] > halfExtent[0] && halfExtent[1] > halfExtent[2]){
            max = halfExtent[1];
        }
        if(halfExtent[2] > halfExtent[0] && halfExtent[2] > halfExtent[1]){
            max = halfExtent[2];
        }

        float maxExtent = 2.0f * max;
        float scaleFactor = 2.0f / maxExtent;
        float3 centerPoint = float3{0,0,-4};//defaults to < 0, 0, -4 >

        center -= (centerPoint / scaleFactor);

        auto scaleAsFloat3 = float3{scaleFactor, scaleFactor, scaleFactor};
        //mat4f boundingBoxCenterAsMat4 = mat4f{float4{-center,1}};
        auto scaling = mat4f::scaling(scaleAsFloat3);
        auto translation = mat4f::translation(-center);
        auto transform = scaling * translation;
        auto transposeMat = transpose(transform);

        tm->setTransform(tm->getInstance(filamentAsset->getRoot()), transposeMat);
        auto checkTransform = tm->getTransform(tm->getInstance(filamentAsset->getRoot()));
        auto asFloatArray = checkTransform.asArray();

        auto camPos = g_camera->getPosition();
        auto camProjMat = g_camera->getProjectionMatrix();

    }
}}