#ifndef VULKAN_SURFACE_H
#define VULKAN_SURFACE_H

#ifdef __ANDROID__
#include "vulkan_wrapper.h"
#endif
#include <vector>

using std::vector;

namespace Vulkan
{
    class Surface {
    public:
        typedef struct SurfaceSupportInfo {
            VkSurfaceCapabilitiesKHR capabilities;
            vector<VkSurfaceFormatKHR> formats;
            vector<VkPresentModeKHR> presentModes;
        } SurfaceSupportInfo;

#ifdef __ANDROID__
        Surface(ANativeWindow* nativeWindow, const VkInstance instance);
#endif
        ~Surface();

        SurfaceSupportInfo QuerySurfaceSupport(VkPhysicalDevice physicalDevice) const;

        VkSurfaceKHR GetSurface() const { return _surface; }
    private:
        VkSurfaceKHR     _surface;
        const VkInstance _instance;
    };
}

#endif // VULKAN_SURFACE_H
