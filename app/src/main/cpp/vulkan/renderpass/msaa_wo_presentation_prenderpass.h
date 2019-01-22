#ifndef MSAA_WO_PRESENT_RENDER_PASS_H
#define MSAA_WO_PRESENT_RENDER_PASS_H

#include "msaa_renderpass.h"
#include "../vulkan_utility.h"

namespace Vulkan
{
    class MSAAWOPresentationRenderPass : public MSAARenderPass
    {
    public:
        MSAAWOPresentationRenderPass(const Device& device) : MSAARenderPass(device) {}
        ~MSAAWOPresentationRenderPass() override
        {
            DebugLog("~MSAAWOPresentationRenderPass()");
            vkDestroyRenderPass(_device.LogicalDevice(), _renderPass, nullptr);
            _renderPass = VK_NULL_HANDLE;
        }

    private:
        virtual void CreateRenderPassImpl() override { CreateRenderPassFlow(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL); }
    };
}

#endif // MSAA_WO_PRESENT_RENDER_PASS_H