#ifndef VULKAN_COLOR_DST_RENDERPASS_H
#define VULKAN_COLOR_DST_RENDERPASS_H

#include "renderpass.h"

namespace Vulkan
{
    class ColorDestinationRenderPass : public RenderPass {
    public:
        ColorDestinationRenderPass(const Device& device) : RenderPass(device) {}
        ~ColorDestinationRenderPass() override;
    private:
        virtual void CreateRenderPassImpl() override;
    };
}

#endif // VULKAN_COLOR_DST_RENDERPASS_H
