/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include "../includes/ibl/IBL.h"

#include <fstream>
#include <string>
#include <iostream>

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <filament/Engine.h>
#include <filament/IndexBuffer.h>
#include <filament/IndirectLight.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <filament/Texture.h>
#include <filament/Skybox.h>

#include "stb_image.h"

#include "../../android/Path.h"

using namespace filament;
using namespace math;
using namespace utils;

IBL::IBL(Engine& engine) : mEngine(engine) {
}

IBL::~IBL() {
    mEngine.destroy(mIndirectLight);
    mEngine.destroy(mTexture);
    mEngine.destroy(mSkybox);
    mEngine.destroy(mSkyboxTexture);
}

bool IBL::loadFromDirectory(AAssetManager *assetManager, const utils::Path& path) {
    // Read spherical harmonics
    Path sh(Path::concat(path, "sh.txt"));
    AAsset* asset = AAssetManager_open(assetManager, sh.getPath().c_str(), AASSET_MODE_RANDOM);
    if (asset) {
        const void* buf = AAsset_getBuffer(asset);
        off_t size = AAsset_getLength(asset);
        if (buf) {
            struct membuf : std::streambuf {
                membuf(char *base, std::ptrdiff_t n) {
                    this->setg(base, base, base + n);
                }
            };
            membuf sbuf((char *)buf, size);
            std::istream shReader(&sbuf);

            shReader >> std::skipws;
            std::string line;
            for (size_t i = 0; i < 9; i++) {
                std::getline(shReader, line);
                int n = sscanf(line.c_str(), "(%f,%f,%f)", &mBands[i].r, &mBands[i].g, &mBands[i].b);
                if (n != 3)
                    return false;
            }
        }
        AAsset_close(asset);
    }

    // Read mip-mapped cubemap
    if (!loadCubemapLevel(&mTexture, assetManager, path, 0, "m0_")) return false;

    size_t numLevels = mTexture->getLevels();
    for (size_t i = 1; i<numLevels; i++) {
        std::string levelPrefix = "m";
        levelPrefix += std::to_string(i) + "_";
        if (!loadCubemapLevel(&mTexture, assetManager, path, i, levelPrefix))
            return false;
    }

    if (!loadCubemapLevel(&mSkyboxTexture, assetManager, path)) return false;

    mIndirectLight = IndirectLight::Builder()
            .reflections(mTexture)
            .irradiance(3, mBands)
            .intensity(30000.0f)
            .build(mEngine);

    mSkybox = Skybox::Builder().environment(mSkyboxTexture).build(mEngine);

    return true;
}

bool IBL::loadCubemapLevel(filament::Texture **texture,
                           AAssetManager *assetManager, const utils::Path &path, size_t level,
                           std::string const &levelPrefix) const {
    static const char* faceSuffix[6] = { "px", "nx", "py", "ny", "pz", "nz" };

    size_t size = 0;
    size_t numLevels = 1;

    { // this is just a scope to avoid variable name hidding below
        int w, h;
        std::string faceName = levelPrefix + faceSuffix[0] + ".rgbm";
        Path facePath(Path::concat(path, faceName));

        AAsset* asset = AAssetManager_open(assetManager, facePath.getPath().c_str(), AASSET_MODE_RANDOM);
        if (!asset) {
            std::cerr << "The face " << faceName << " does not exist" << std::endl;
            return false;
        }

        const void *buf = AAsset_getBuffer(asset);
        off_t len = AAsset_getLength(asset);
        stbi_info_from_memory((const stbi_uc *) buf, (int) len, &w, &h, nullptr);
        AAsset_close(asset);

        if (w != h) {
            std::cerr << "with != height" << std::endl;
            return false;
        }

        size = (size_t)w;
        if (levelPrefix != "") {
            numLevels = (size_t) std::log2(size) + 1;
        }

        if (level == 0) {
            *texture = Texture::Builder()
                    .width((uint32_t) size)
                    .height((uint32_t) size)
                    .levels((uint8_t) numLevels)
                    .format(Texture::InternalFormat::UNUSED)
                    .sampler(Texture::Sampler::SAMPLER_CUBEMAP)
                    .build(mEngine);
        }
    }


    // RGBM encoding: 4 bytes per pixel
    const size_t faceSize = size * size * 4;

    Texture::FaceOffsets offsets;
    Texture::PixelBufferDescriptor buffer(
            malloc(faceSize * 6), faceSize * 6,
            Texture::Format::UNUSED, Texture::Type::UBYTE,
            (Texture::PixelBufferDescriptor::Callback) &free);

    bool success = true;
    uint8_t* p = static_cast<uint8_t*>(buffer.buffer);

    for (size_t j = 0; j < 6; j++) {
        offsets[j] = faceSize * j;

        std::string faceName = levelPrefix + faceSuffix[j] + ".rgbm";
        Path facePath(Path::concat(path, faceName));

        AAsset* asset = AAssetManager_open(assetManager, facePath.getPath().c_str(), AASSET_MODE_RANDOM);
        if (!asset) {
            std::cerr << "The face " << faceName << " does not exist" << std::endl;
            success = false;
            break;
        }

        int w, h, n;
        const void *buf = AAsset_getBuffer(asset);
        off_t len = AAsset_getLength(asset);
        unsigned char* data = stbi_load_from_memory((const stbi_uc *)buf, (int)len, &w, &h, &n, 4);
        AAsset_close(asset);

        if (w != h || w != size) {
            std::cerr << "Face " << faceName << "has a wrong size " << w << " x " << h <<
                    ", instead of " << size << " x " << size << std::endl;
            success = false;
            break;
        }

        if (data == nullptr || n != 4) {
            std::cerr << "Could not decode face " << faceName << std::endl;
            success = false;
            break;
        }
        memcpy(p + offsets[j], data, size_t(w * h * 4));
        stbi_image_free(data);
    }

    if (!success) return false;

    (*texture)->setImage(mEngine, level, std::move(buffer), offsets);

    return true;
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
