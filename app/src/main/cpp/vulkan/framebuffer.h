#ifndef VULKAN_FRAMEBUFFER_H
#define VULKAN_FRAMEBUFFER_H

#include "../log/log.h"
#ifdef __ANDROID__
#include "vulkan_wrapper.h"
#endif
#include "device.h"
#include "swapchain.h"

using Utility::Log;

namespace Vulkan {

    class Framebuffer
    {
    public:
        Framebuffer(const Device& device) : _device(device) { DebugLog("Framebuffer(const Device&)"); }
        Framebuffer(Framebuffer&& other);
        ~Framebuffer();

        static void CreateFrameBuffer(Framebuffer&               framebuffer,
                                      VkRenderPass               renderPass,
                                      const vector<VkImageView>& attachments,

                                      uint32_t                   layers = 1);

        bool CreateSwapchainFramebuffer(VkRenderPass               renderPass,
                                        const vector<VkImageView>& attachments,
                                        const VkExtent2D&          extent2D,

                                        uint32_t                   layers = 1);

        const VkFramebuffer GetFramebuffer() const { return _framebuffer; }

    private:
        VkFramebuffer _framebuffer = VK_NULL_HANDLE;
        VkRenderPass  _renderPass  = VK_NULL_HANDLE;
        uint32_t      _width;
        uint32_t      _height;

        const Device& _device;
    };
}

#endif // VULKAN_FRAMEBUFFER_H
