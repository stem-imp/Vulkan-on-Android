#ifndef MSAA_RENDER_PASS_H
#define MSAA_RENDER_PASS_H

#include "renderpass.h"

namespace Vulkan
{
    class MSAARenderPass : public RenderPass
    {
    public:
        MSAARenderPass(const Device& device) : RenderPass(device) {}
        ~MSAARenderPass() override;

        VkFormat DepthFormat() const { return _depthFormat; }

    protected:
        void CreateRenderPassFlow(VkImageLayout finalLayout, VkImageLayout resolvedFinalLayout);
    private:
        virtual void CreateRenderPassImpl() override;

        VkFormat _depthFormat;
    };
}

#endif // MSAA_RENDER_PASS_H