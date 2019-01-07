#ifndef VULKAN_RENDER_PASS_H
#define VULKAN_RENDER_PASS_H

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
        virtual ~RenderPass() = 0;

        void CreateRenderPass();
        // Get format of VkSurfaceFormat.
        function<VkFormat(void)> getFormat;
        function<VkSampleCountFlagBits(void)> getSampleCount;

        VkRenderPass GetRenderPass() const { return _renderPass; }
    protected:
        RenderPass(const Device& device) : _device(device) {}

        // Base Variables
        VkRenderPass _renderPass;
        const Device& _device;
    private:
        virtual void CreateRenderPassImpl() = 0;
    };
}

#endif // VULKAN_RENDER_PASS_H
