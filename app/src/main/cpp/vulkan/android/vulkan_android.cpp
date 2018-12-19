#include "../vulkan_utility.h"
#include "android_native_app_glue.h"

// ==== Instance ==== //
uint32_t GetAPIVersion()
{
    // NDK r18
    return VK_MAKE_VERSION(1, 0, 69);
}

void AppendInstanceExtension(std::vector<const char*>& instanceExtensionNames)
{
    instanceExtensionNames.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
}