cmake_minimum_required(VERSION 3.4.1)
include(ExternalProject)

set(NDK_SRC_DIR ${ANDROID_NDK}/sources)
set(SRC_DIR ${NDK_SRC_DIR}/third_party/vulkan/src)
set(LAYER_DIR ${SRC_DIR}/build-android/generated)

set(CMAKE_CXX_clone "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror -Wno-unused-function \
                     -Wno-unused-const-variable -Wno-c++98-compat-pedantic")
add_definitions("-DVK_PROTOTYPES")
add_definitions("-DVK_USE_PLATFORM_ANDROID_KHR")

# build layer_utils as a static lib
if(${ANDROID_NDK_REVISION} LESS 17)
    include_directories(${SRC_DIR}/include
            ${SRC_DIR}/layers
            ${SRC_DIR}/loader)
    add_library(layer_utils STATIC
            ${SRC_DIR}/layers/vk_layer_config.cpp
            ${SRC_DIR}/layers/vk_layer_extension_utils.cpp
            ${SRC_DIR}/layers/vk_layer_utils.cpp)
else()
    set(CMAKE_CXX_clone1 "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
    include_directories(${SRC_DIR}/include
            ${LAYER_DIR}/include
            ${SRC_DIR}/layers
            ${SRC_DIR}/loader)
    add_library(layer_utils STATIC
            ${SRC_DIR}/layers/vk_layer_config.cpp
            ${SRC_DIR}/layers/vk_layer_extension_utils.cpp
            ${SRC_DIR}/layers/vk_layer_utils.cpp
            ${SRC_DIR}/layers/vk_format_utils.cpp)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_clone1}")
endif()

# build core_validation layers which including shader validation
if(${ANDROID_NDK_REVISION} LESS 17)
    set(core_validation_SRCS
            ${SRC_DIR}/layers/core_validation.cpp
            ${SRC_DIR}/layers/vk_layer_table.cpp )
    if(NOT (${ANDROID_NDK_REVISION} LESS 13))
        list(APPEND core_validation_SRCS
                ${SRC_DIR}/layers/descriptor_sets.cpp
                ${LAYER_DIR}/include/vk_safe_struct.cpp)
    endif(NOT (${ANDROID_NDK_REVISION} LESS 13))

    # piggyback on the previous CXX_FLAGS
    include_directories(${LAYER_DIR}/include ${SRC_DIR}/../glslang)
    add_library(VkLayer_core_validation SHARED ${core_validation_SRCS})
    target_link_libraries(VkLayer_core_validation log android layer_utils)
else()
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
    #get_filename_component(EXTERNAL_DIR "${SRC_DIR}/../../shaderc/third_party" ABSOLUTE)
    set(core_validation_SRCS
            ${SRC_DIR}/layers/core_validation.cpp
            ${SRC_DIR}/layers/vk_layer_table.cpp
            #${LAYER_DIR}/include/vk_safe_struct.cpp
            #${SRC_DIR}/layers/convert_to_renderpass2.cpp
            ${SRC_DIR}/layers/descriptor_sets.cpp
            ${SRC_DIR}/layers/buffer_validation.cpp
            ${SRC_DIR}/layers/shader_validation.cpp
            ${SRC_DIR}/layers/xxhash.c)
    add_library(VkLayer_core_validation SHARED ${core_validation_SRCS})
    target_include_directories(VkLayer_core_validation PRIVATE
            ${SRC_DIR}/include
            ${SRC_DIR}/layers
            ${LAYER_DIR}/include
            ${SRC_DIR}/loader
            #${EXTERNAL_DIR}/glslang
            #${EXTERNAL_DIR}/spirv-tools/include
            )
    target_link_libraries(VkLayer_core_validation log layer_utils)
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_clone1}")
endif()


# ndk-r13 validation layers use shaderc, need to compile it
include_directories(${NDK_SRC_DIR}/third_party/shaderc/third_party/glslang
        ${NDK_SRC_DIR}/third_party/shaderc/third_party/spirv-tools/include)

set(SHADERC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/build/shaderc)
if((${ANDROID_NDK_REVISION} GREATER_EQUAL 13) AND (${ANDROID_NDK_REVISION} LESS 18))
    set(lib_shaderc ${SHADERC_OUT_DIR}/gnustl_static/${ANDROID_ABI}/libshaderc.a)
elseif((${ANDROID_NDK_REVISION} GREATER 17))
    set(lib_shaderc ${SHADERC_OUT_DIR}/c++_static/${ANDROID_ABI}/libshaderc.a)
endif()
set(SHADERC_SRC_DIR ${NDK_SRC_DIR}/third_party/shaderc)

set(NDK_TOOLCHAIN_CFG ${ANDROID_TOOLCHAIN})
string(TOLOWER "${NDK_TOOLCHAIN_CFG}" NDK_TOOLCHAIN_CFG)
if(${ANDROID_TOOLCHAIN} STREQUAL "gcc")
    set(NDK_TOOLCHAIN_CFG "4.9")
endif()

if(CMAKE_HOST_WIN32)
    ExternalProject_Add(BuildShaderc
            SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ${ANDROID_NDK}/ndk-build.cmd --jobs=8 NDK_DEBUG=1
            NDK_PROJECT_PATH=${SHADERC_SRC_DIR}
            NDK_LIBS_OUT=${SHADERC_OUT_DIR}
            APP_ABI=${ANDROID_ABI}
            APP_PLATFORM=${ANDROID_PLATFORM}
            APP_STL=${ANDROID_STL}
            NDK_TOOLCHAIN_VERSION=${NDK_TOOLCHAIN_CFG}
            APP_BUILD_SCRIPT=${SHADERC_SRC_DIR}/Android.mk
            libshaderc_combined
            LOG_CONFIGURE 1
            BUILD_ALWAYS 1
            LOG_BUILD    1
            BUILD_BYPRODUCTS ${lib_shaderc}
            INSTALL_COMMAND "")
else(CMAKE_HOST_WIN32)
    ExternalProject_Add(BuildShaderc
            SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
            CONFIGURE_COMMAND mkdir -p ${SHADERC_OUT_DIR}
            BUILD_COMMAND ${ANDROID_NDK}/ndk-build --jobs=8 NDK_DEBUG=1
            NDK_PROJECT_PATH=${SHADERC_SRC_DIR}
            NDK_LIBS_OUT=${SHADERC_OUT_DIR}
            APP_ABI=${ANDROID_ABI}
            APP_PLATFORM=${ANDROID_PLATFORM}
            APP_STL=${ANDROID_STL}
            NDK_TOOLCHAIN_VERSION=${NDK_TOOLCHAIN_CFG}
            APP_BUILD_SCRIPT=${SHADERC_SRC_DIR}/Android.mk
            libshaderc_combined     # make the combined lib to use
            BUILD_ALWAYS 1    # Force build
            BUILD_BYPRODUCTS ${lib_shaderc}
            INSTALL_COMMAND "")
endif(CMAKE_HOST_WIN32)

add_dependencies(VkLayer_core_validation BuildShaderc)
target_link_libraries(VkLayer_core_validation ${lib_shaderc})

# device_limit layer is for ndk-r12 only, NDK-R13 merged it into core_validation
if(${ANDROID_NDK_REVISION} LESS 13)
    add_library(VkLayer_device_limits SHARED
            ${SRC_DIR}/layers/device_limits.cpp
            ${SRC_DIR}/layers/vk_layer_table.cpp)
    target_link_libraries(VkLayer_device_limits log android layer_utils)
endif(${ANDROID_NDK_REVISION} LESS 13)

if(${ANDROID_NDK_REVISION} LESS 17)
    add_library(VkLayer_image SHARED
            ${SRC_DIR}/layers/image.cpp
            ${SRC_DIR}/layers/vk_layer_table.cpp)
    target_link_libraries(VkLayer_image log android layer_utils)
endif()

if(${ANDROID_NDK_REVISION} LESS 17)
    add_library(VkLayer_parameter_validation SHARED
            ${SRC_DIR}/layers/parameter_validation.cpp
            ${SRC_DIR}/layers/vk_layer_table.cpp)
    target_link_libraries(VkLayer_parameter_validation log android layer_utils)
else()
    add_library(VkLayer_parameter_validation SHARED
            ${SRC_DIR}/layers/parameter_validation_utils.cpp
            ${LAYER_DIR}/include/parameter_validation.cpp
            ${SRC_DIR}/layers/vk_layer_table.cpp)
    target_link_libraries(VkLayer_parameter_validation PRIVATE log layer_utils)
endif()

# directory structure changed in ndk-13, object_tracker.cpp is from src/layers
if(${ANDROID_NDK_REVISION} LESS 13)
    set(object_tracker_SRCS
            ${LAYER_DIR}/layer-src/object_tracker/object_tracker.cpp
            ${SRC_DIR}/layers/vk_layer_table.cpp)
    add_library(VkLayer_object_tracker SHARED ${object_tracker_SRCS})
    target_link_libraries(VkLayer_object_tracker log android layer_utils)
elseif(${ANDROID_NDK_REVISION} LESS 17)
    set(object_tracker_SRCS
            ${SRC_DIR}/layers/object_tracker.cpp
            ${SRC_DIR}/layers/vk_layer_table.cpp)
    add_library(VkLayer_object_tracker SHARED ${object_tracker_SRCS})
    target_link_libraries(VkLayer_object_tracker log android layer_utils)
else()
    set(object_tracker_SRCS
            ${SRC_DIR}/layers/object_tracker_utils.cpp
            ${LAYER_DIR}/include/object_tracker.cpp
            ${SRC_DIR}/layers/vk_layer_table.cpp)
    add_library(VkLayer_object_tracker SHARED ${object_tracker_SRCS})
    target_link_libraries(VkLayer_object_tracker PRIVATE log layer_utils)
endif()

if(${ANDROID_NDK_REVISION} LESS 17)
    add_library(VkLayer_threading SHARED
            ${SRC_DIR}/layers/threading.cpp
            ${SRC_DIR}/layers/vk_layer_table.cpp)
    target_link_libraries(VkLayer_threading log android layer_utils)
else()
    add_library(VkLayer_threading SHARED
            ${SRC_DIR}/layers/threading.cpp
            ${SRC_DIR}/layers/vk_layer_table.cpp)
    target_link_libraries(VkLayer_threading PRIVATE log layer_utils)
endif()

# src dir structure changed in ndk-r13, unique_objects.cpp is under include/
if(${ANDROID_NDK_REVISION} LESS 13)
    set(unique_objects_SRCS
            ${LAYER_DIR}/layer-src/unique_objects/unique_objects.cpp
            ${LAYER_DIR}/layer-src/unique_objects/vk_safe_struct.cpp
            ${SRC_DIR}/layers/vk_layer_table.cpp)
    add_library(VkLayer_unique_objects SHARED  ${unique_objects_SRCS})
    target_link_libraries(VkLayer_unique_objects log android layer_utils)
elseif(${ANDROID_NDK_REVISION} LESS 14)
    set(unique_objects_SRCS
            ${LAYER_DIR}/include/unique_objects.cpp
            ${LAYER_DIR}/include/vk_safe_struct.cpp
            ${SRC_DIR}/layers/vk_layer_table.cpp)
    add_library(VkLayer_unique_objects SHARED  ${unique_objects_SRCS})
    target_link_libraries(VkLayer_unique_objects log android layer_utils)
elseif(${ANDROID_NDK_REVISION} LESS 17)
    set(unique_objects_SRCS
            ${SRC_DIR}/layers/unique_objects.cpp
            ${LAYER_DIR}/include/vk_safe_struct.cpp
            ${SRC_DIR}/layers/vk_layer_table.cpp)
    add_library(VkLayer_unique_objects SHARED  ${unique_objects_SRCS})
    target_link_libraries(VkLayer_unique_objects log android layer_utils)
else()
    set(unique_objects_SRCS
            ${SRC_DIR}/layers/unique_objects.cpp
            ${SRC_DIR}/layers/vk_layer_table.cpp)
    add_library(VkLayer_unique_objects SHARED  ${unique_objects_SRCS})
    target_link_libraries(VkLayer_unique_objects PRIVATE log layer_utils)
endif()

if(${ANDROID_NDK_REVISION} LESS 17)
    add_library(VkLayer_swapchain SHARED
            ${SRC_DIR}/layers/swapchain.cpp
            ${SRC_DIR}/layers/vk_layer_table.cpp)
    target_link_libraries(VkLayer_swapchain log android layer_utils)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_clone}")