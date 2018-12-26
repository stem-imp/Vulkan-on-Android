#include "../log/log.h"
#include "surface.h"

using Vulkan::Surface;
using Utility::Log;

Surface::~Surface()
{
    DebugLog("~Surface()");
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
}

Surface::SurfaceSupportInfo Surface::QuerySurfaceSupport(VkPhysicalDevice physicalDevice) const
{
    SurfaceSupportInfo info;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, _surface, &info.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _surface, &formatCount, nullptr);
    if (formatCount != 0) {
        info.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _surface, &formatCount, info.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        info.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _surface, &presentModeCount, info.presentModes.data());
    }

    return info;
}
