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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
             log-cpp

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/cpp/log.cpp )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

target_link_libraries( # Specifies the target library.
                       log-cpp

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/main/glm)
add_definitions("-DGLM_FORCE_SIZE_T_LENGTH -DGLM_FORCE_RADIANS -DGLM_FORCE_DEPTH_ZERO_TO_ONE")

# Import the CMakeLists.txt for the glm library
add_subdirectory(src/main/glm)

set(VULKAN_INCLUDE_DIR ${ANDROID_NDK}/sources/third_party/vulkan/src/include)
set(VULKAN_COMMON_DIR ${ANDROID_NDK}/sources/third_party/vulkan/src/common)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_DEBUG_EXTENTIONS -DVK_USE_PLATFORM_ANDROID_KHR")
add_definitions("-DSTB_IMAGE_IMPLEMENTATION")
include_directories(${VULKAN_INCLUDE_DIR})
include_directories(${VULKAN_COMMON_DIR})

add_library( # Sets the name of the library.
             main

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             ${VULKAN_COMMON_DIR}/vulkan_wrapper.cpp
             src/main/cpp/layer_extension.cpp
             src/main/cpp/vulkan_main.cpp
             src/main/cpp/event_loop.cpp
             src/main/cpp/main.cpp )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       main
                       log-cpp
                       android
                       native_app_glue )