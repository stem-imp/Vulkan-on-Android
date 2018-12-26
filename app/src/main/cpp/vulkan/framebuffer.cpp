#include "framebuffer.h"
#include "vulkan_utility.h"

namespace Vulkan
{
    Framebuffer::Framebuffer(Framebuffer&& other) : _device(other._device)
    {
        _framebuffer       = other._framebuffer;
        _renderPass        = other._renderPass;
        _width             = other._width;
        _height            = other._height;
        other._framebuffer = VK_NULL_HANDLE;
        other._renderPass  = VK_NULL_HANDLE;
        other._width       = UINT32_MAX;
        other._height      = UINT32_MAX;
    }

    Framebuffer::~Framebuffer()
    {
        if (_framebuffer) {
            DebugLog("~Framebuffer() vkDestroyFramebuffer()");
            vkDestroyFramebuffer(_device.LogicalDevice(), _framebuffer, nullptr);
            _framebuffer = VK_NULL_HANDLE;
            _renderPass = VK_NULL_HANDLE;
        } else {
            DebugLog("~Framebuffer()");
        }
    }

    void Framebuffer::CreateFrameBuffer(Framebuffer&               framebuffer,
                                        VkRenderPass               renderPass,
                                        const vector<VkImageView>& attachments,

                                        uint32_t                   layers)
    {
        framebuffer._framebuffer = ::CreateFramebuffer(renderPass,
                                                       attachments.size(),
                                                       attachments.data(),
                                                       framebuffer._width,
                                                       framebuffer._height,
                                                       layers,
                                                       framebuffer._device.LogicalDevice());
    }

    void Framebuffer::CreateSwapchainFramebuffer(VkRenderPass               renderPass,
                                                 const vector<VkImageView>& attachments,
                                                 const VkExtent2D&          extent2D,

                                                 uint32_t                   layers)
    {
        _renderPass       = renderPass;
        _width            = extent2D.width;
        _height           = extent2D.height;
        CreateFrameBuffer(*this, renderPass, attachments, layers);
    }

    void Framebuffer::ReleaseFramebuffer()
    {
        if (_framebuffer) {
            vkDestroyFramebuffer(_device.LogicalDevice(), _framebuffer, nullptr);
            _framebuffer = VK_NULL_HANDLE;
            _renderPass = VK_NULL_HANDLE;
        }
    }
}