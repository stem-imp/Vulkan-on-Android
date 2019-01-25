#ifndef MSAA_SHADER_READ_RENDER_PASS_H
#define MSAA_SHADER_READ_RENDER_PASS_H

#include "renderpass.h"
#include "../vulkan_utility.h"

namespace Vulkan
{
    class MSAAShaderReadRenderPass : public RenderPass
    {
    public:
        MSAAShaderReadRenderPass(const Device& device) : RenderPass(device) {}
        ~MSAAShaderReadRenderPass() override
        {
            DebugLog("MSAAShaderReadRenderPass");
            vkDestroyRenderPass(_device.LogicalDevice(), _renderPass, nullptr);
            _renderPass = VK_NULL_HANDLE;
        }

        VkFormat DepthFormat() const { return _depthFormat; }

    private:
        virtual void CreateRenderPassImpl() override;

        VkFormat _depthFormat;
    };
}

#endif // MSAA_SHADER_READ_RENDER_PASS_H