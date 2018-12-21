#ifndef VULKAN_RENDERPASS_H
#define VULKAN_RENDERPASS_H

#ifdef __ANDROID__
#include "vulkan_wrapper.h"
#endif
#include "../device.h"
#include <functional>

using std::function;

namespace Vulkan
{
    class RenderPass {
    public:
        RenderPass(const Device& device) : _device(device) {}
        virtual ~RenderPass() = 0;

        void CreateRenderPass();
        // Get format of VkSurfaceFormat.
        function<VkFormat(void)> getFormat;

        const VkRenderPass GetRenderPass() const { return _renderPass; }
    protected:

        // Base Variables
        VkRenderPass _renderPass;
        const Device& _device;
        VkFormat _format;
    private:
        virtual void CreateRenderPassImpl() = 0;
    };
}

#endif // VULKAN_RENDERPASS_H
