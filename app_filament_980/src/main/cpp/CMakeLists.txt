cmake_minimum_required(VERSION 3.10)
project(filament)

add_library(
        hello_filament SHARED hello_filament.cpp
        ## Android
        ${FILAMENT_OFFICIAL_REPO_DIR}/android/common/NioUtils.cpp
        ${FILAMENT_OFFICIAL_REPO_DIR}/android/common/CallbackUtils.cpp
        ${FILAMENT_OFFICIAL_REPO_DIR}/libs/utils/src/Path.cpp
        ## MaterialGenerator (replaced by JitShaderProvider)
        ${FILAMENT_OFFICIAL_REPO_DIR}/libs/gltfio/src/JitShaderProvider.cpp
)
set_property(TARGET hello_filament PROPERTY CXX_STANDARD 17)

#Find Android Native Log lib with others libs
find_library(log-lib log)
find_library(android-lib android)

#Find .h files
## Fund filament .h. libs
##
include_directories(${FILAMENT_OFFICIAL_REPO_DIR}/filament/backend/include/)
## filament
include_directories(${FILAMENT_OFFICIAL_REPO_DIR}/filament/include/)
## Find libs .h files
include_directories(${FILAMENT_OFFICIAL_REPO_DIR}/libs/camutils/include/)
include_directories(${FILAMENT_OFFICIAL_REPO_DIR}/libs/filabridge/include/)
include_directories(${FILAMENT_OFFICIAL_REPO_DIR}/libs/filamat/include/)
include_directories(${FILAMENT_OFFICIAL_REPO_DIR}/libs/filamentapp/include/)
include_directories(${FILAMENT_OFFICIAL_REPO_DIR}/libs/filameshio/include/)
include_directories(${FILAMENT_OFFICIAL_REPO_DIR}/libs/geometry/include/)
include_directories(${FILAMENT_OFFICIAL_REPO_DIR}/libs/gltfio/include/)
include_directories(${FILAMENT_OFFICIAL_REPO_DIR}/libs/ibl/include/)
include_directories(${FILAMENT_OFFICIAL_REPO_DIR}/libs/iblprefilter/include/)
include_directories(${FILAMENT_OFFICIAL_REPO_DIR}/libs/image/include/)
include_directories(${FILAMENT_OFFICIAL_REPO_DIR}/libs/ktxreader/include/)
include_directories(${FILAMENT_OFFICIAL_REPO_DIR}/libs/math/include/)
include_directories(${FILAMENT_OFFICIAL_REPO_DIR}/libs/mathio/include/)
include_directories(${FILAMENT_OFFICIAL_REPO_DIR}/libs/utils/include/)
## Find third_party .h files
include_directories(${FILAMENT_OFFICIAL_REPO_DIR}/third_party/hat-trie/)
include_directories(${FILAMENT_OFFICIAL_REPO_DIR}/third_party/robin-map/)
include_directories(${FILAMENT_OFFICIAL_REPO_DIR}/third_party/basisu/transcoder/)
include_directories(${FILAMENT_OFFICIAL_REPO_DIR}/third_party/stb/)
## Find android .h files
include_directories(${FILAMENT_OFFICIAL_REPO_DIR}/android/)

#Add .so / .a libs
add_library(lib_filamat SHARED IMPORTED)
set_target_properties(
        lib_filamat
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libfilamat-jni.so)

add_library(lib_filament SHARED IMPORTED)
set_target_properties(
        lib_filament
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libfilament-jni.so)

add_library(lib_gltfio SHARED IMPORTED)
set_target_properties(
        lib_gltfio
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libgltfio-jni.so)

add_library(lib_filament-utils SHARED IMPORTED)
set_target_properties(
        lib_filament-utils
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libfilament-utils-jni.so)

add_library(lib_filament-utils-lite SHARED IMPORTED)
set_target_properties(
        lib_filament-utils-lite
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libfilament-utils-jni-lite.so)


add_library(libbackend SHARED IMPORTED)
set_target_properties(
        libbackend
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libbackend.a)

add_library(libbluevk SHARED IMPORTED)
set_target_properties(
        libbluevk
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libbluevk.a)

add_library(libcamutils SHARED IMPORTED)
set_target_properties(
        libcamutils
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libcamutils.a)

add_library(libdracodec SHARED IMPORTED)
set_target_properties(
        libdracodec
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libdracodec.a)

add_library(libfilabridge SHARED IMPORTED)
set_target_properties(
        libfilabridge
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libfilabridge.a)

add_library(libfilaflat SHARED IMPORTED)
set_target_properties(
        libfilaflat
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libfilaflat.a)

add_library(libfilamat SHARED IMPORTED)
set_target_properties(
        libfilamat
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libfilamat.a)

add_library(libfilamat_lite SHARED IMPORTED)
set_target_properties(
        libfilamat_lite
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libfilamat_lite.a)

add_library(libfilament SHARED IMPORTED)
set_target_properties(
        libfilament
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libfilament.a)

add_library(libfilameshio SHARED IMPORTED)
set_target_properties(
        libfilameshio
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libfilameshio.a)

add_library(libgeometry SHARED IMPORTED)
set_target_properties(
        libgeometry
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libgeometry.a)

add_library(libgltfio_core SHARED IMPORTED)
set_target_properties(
        libgltfio_core
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libgltfio_core.a)

add_library(libgltfio_resources SHARED IMPORTED)
set_target_properties(
        libgltfio_resources
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libgltfio_resources.a)

add_library(libgltfio_resources_lite SHARED IMPORTED)
set_target_properties(
        libgltfio_resources_lite
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libgltfio_resources_lite.a)

add_library(libibl SHARED IMPORTED)
set_target_properties(
        libibl
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libibl.a)

add_library(libimage SHARED IMPORTED)
set_target_properties(
        libimage
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libimage.a)

add_library(libmeshoptimizer SHARED IMPORTED)
set_target_properties(
        libmeshoptimizer
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libmeshoptimizer.a)

add_library(libshaders SHARED IMPORTED)
set_target_properties(
        libshaders
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libshaders.a)

add_library(libsmol-v SHARED IMPORTED)
set_target_properties(
        libsmol-v
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libsmol-v.a)

add_library(libutils SHARED IMPORTED)
set_target_properties(
        libutils
        PROPERTIES IMPORTED_LOCATION
        ${FILAMENT_DIR}/so/${ANDROID_ABI}/libutils.a)


#Link Android specified libs
target_link_libraries(hello_filament ${log-lib} ${android-lib})
target_link_libraries(hello_filament lib_filament libfilament libfilamat)
target_link_libraries(hello_filament libbackend)
target_link_libraries(hello_filament libfilaflat)
target_link_libraries(hello_filament libgeometry)
target_link_libraries(hello_filament libibl)
target_link_libraries(hello_filament libbluevk)
target_link_libraries(hello_filament libcamutils)
target_link_libraries(hello_filament libfilameshio)
target_link_libraries(hello_filament libimage)
target_link_libraries(hello_filament libmeshoptimizer)
target_link_libraries(hello_filament lib_gltfio)
target_link_libraries(hello_filament libgltfio_core)
target_link_libraries(hello_filament libgltfio_resources)
target_link_libraries(hello_filament libdracodec)
target_link_libraries(hello_filament libutils)
target_link_libraries(hello_filament lib_filament-utils)
target_link_libraries(hello_filament libfilabridge)
target_link_libraries(hello_filament libsmol-v)
target_link_libraries(hello_filament libshaders)

set(COMPILER_FLAGS
        -Wno-extern-c-compat
        $<$<NOT:$<PLATFORM_ID:Linux>>:-Wno-address-of-packed-member>
        $<$<CONFIG:Release>:-ffast-math>)
