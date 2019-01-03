# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")
include_directories(${ANDROID_NDK}/sources/android/native_app_glue)

# Export ANativeActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

# build native_app_glue as a static lib
add_library(native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

# now set up compilation options
set(CMAKE_CXX_clone "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

# android-utility-cpp-lib
add_library(android-utility-cpp-lib SHARED
        src/main/cpp/androidutility/log/log_android.cpp
        src/main/cpp/androidutility/eventloop/event_loop.cpp)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.
find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

target_link_libraries( # Specifies the target library.
        android-utility-cpp-lib android

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib} )

# vulkan
add_definitions("-DUSE_DEBUG_EXTENTIONS")
add_definitions("-DVK_USE_PLATFORM_ANDROID_KHR")
include_directories(${ANDROID_NDK}/sources/third_party/vulkan/src/include)
set(VULKAN_COMMON_DIR ${ANDROID_NDK}/sources/third_party/vulkan/src/common)
include_directories(${VULKAN_COMMON_DIR})

# glm
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_definitions("-DGLM_FORCE_SIZE_T_LENGTH -DGLM_FORCE_RADIANS -DGLM_FORCE_DEPTH_ZERO_TO_ONE -DGLM_LANG_STL11_FORCED")
add_subdirectory(include/glm)

# stb_image
#add_definitions("-DSTB_IMAGE_IMPLEMENTATION")
#add_definitions("-DSTBI_ONLY_JPEG")
#add_definitions("-DSTBI_ONLY_PNG")

add_library(assimp-lib SHARED IMPORTED)
set_target_properties(assimp-lib PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libassimp.so)
add_library(android_jniiosystem-lib SHARED IMPORTED)
set_target_properties(android_jniiosystem-lib PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libandroid_jniiosystem.a)

add_library( # Sets the name of the library.
        main

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        ${VULKAN_COMMON_DIR}/vulkan_wrapper.cpp

        src/main/cpp/vulkan/layer_extension.cpp
        src/main/cpp/vulkan/instance.cpp
        src/main/cpp/vulkan/surface.cpp
        src/main/cpp/vulkan/device.cpp
        src/main/cpp/vulkan/swapchain.cpp
        src/main/cpp/vulkan/renderpass/renderpass.cpp
        src/main/cpp/vulkan/renderpass/color_dst_renderpass.cpp
        src/main/cpp/vulkan/renderpass/color_depth_renderpass.cpp
        src/main/cpp/vulkan/framebuffer.cpp
        src/main/cpp/vulkan/command.cpp
        src/main/cpp/vulkan/buffer.cpp
        src/main/cpp/vulkan/model/model.cpp
        src/main/cpp/vulkan/model/model_resource.cpp
        src/main/cpp/vulkan/texture/texture.cpp
        src/main/cpp/vulkan/texture/texture2d.cpp

        src/main/cpp/vulkan/android/vulkan_android.cpp
        src/main/cpp/vulkan/vulkan_utility.cpp
        src/main/cpp/vulkan/android/renderer_vulkan_android.cpp

        src/main/cpp/vulkan_main.cpp
        src/main/cpp/scene/emptyscene/android/empty_scene_renderer_vulkan_android.cpp
        src/main/cpp/scene/emptyscene/android/empty_scene_android.cpp
        src/main/cpp/scene/earthscene/android/earth_scene_renderer_android.cpp
        src/main/cpp/scene/earthscene/android/earth_scene_android.cpp
        src/main/cpp/scene/scene.cpp
        src/main/cpp/main.cpp )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.
target_link_libraries( # Specifies the target library.
        main

        android
        android-utility-cpp-lib
        native_app_glue
        assimp-lib
        android_jniiosystem-lib )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_clone}")