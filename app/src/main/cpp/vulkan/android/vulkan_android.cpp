#ifdef __ANDROID__

#include "../surface.h"
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

namespace Vulkan {

    // ==== Surface ==== //
    Surface::Surface(ANativeWindow *nativeWindow, const VkInstance instance) : _instance(instance)
    {
        VkAndroidSurfaceCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.window = nativeWindow;
        VK_CHECK_RESULT(vkCreateAndroidSurfaceKHR(instance, &createInfo, nullptr, &_surface));
    }
}

#endif