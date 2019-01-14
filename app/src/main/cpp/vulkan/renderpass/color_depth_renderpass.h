#ifndef VULKAN_COLOR_DEPTH_RENDERPASS_H
#define VULKAN_COLOR_DEPTH_RENDERPASS_H

#include "renderpass.h"

namespace Vulkan
{
    class ColorDepthRenderPass : public RenderPass {
    public:
        ColorDepthRenderPass(const Device& device) : RenderPass(device) {}
        ~ColorDepthRenderPass() override;

        VkFormat DepthFormat() const { return _depthFormat; }
    private:
        virtual void CreateRenderPassImpl() override;

        VkFormat _depthFormat;
    };
}

#endif // VULKAN_COLOR_DEPTH_RENDERPASS_H
