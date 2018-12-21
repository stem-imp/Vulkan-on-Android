#ifndef VULKAN_SWAPCHAIN_H
#define VULKAN_SWAPCHAIN_H

#ifdef __ANDROID__
#include "vulkan_wrapper.h"
#endif
#include "surface.h"
#include "device.h"
#include "../data_type.h"
#include <vector>
#include <memory>
#include <functional>

using std::vector;

#ifdef __ANDROID__
#define SWAPCHAIN_EXTENT_PREPROCESS(extent) \
{ \
   extent.width = extent.height = UINT32_MAX; \
}
#else
#define SWAPCHAIN_EXTENT_PREPROCESS(extent)
#endif

namespace Vulkan
{
    class Swapchain {
    public:
        Swapchain(const Surface& surface, const Device& device) : _swapchain(VK_NULL_HANDLE), _surface(surface), _device(device) {}
        ~Swapchain();
        void CreateSwapChain();
        void CreateImageViews(uint32_t baseArrayLayer = 0, uint32_t layerCount = 1);

        std::function<Extent2D(void)> getScreenExtent;
        std::function<void(void)>     onSwapchainDestroy;

        const VkSwapchainKHR& GetSwapchain() const { return _swapchain; }
        const VkExtent2D& Extent() const { return _extent2D; }
        VkFormat Format() const { return _surfaceFormat.format; }
        uint32_t ConcurrentFramesCount() { return _concurrentFramesCount; }
        const vector<VkImageView>& ImageViews() const { return _imageViews; }

        bool orientationChanged;
    private:
        // Base Variables
        VkSwapchainKHR _swapchain;

        const Vulkan::Surface& _surface;
        const Vulkan::Device&  _device;
        Device::QueuePair      _graphics;
        Device::QueuePair      _present;


        // Derived Variables
        VkExtent2D          _extent2D;
        VkSurfaceFormatKHR  _surfaceFormat;
        uint32_t            _concurrentFramesCount;
        uint32_t            _swapChainImagesCount;
        vector<VkImage>     _images;
        vector<VkImageView> _imageViews;
    };
}

#endif // VULKAN_SWAPCHAIN_H
