#include "color_dst_renderpass.h"
#include "../vulkan_utility.h"

using Vulkan::RenderPass;
using Vulkan::ColorDestinationRenderPass;

ColorDestinationRenderPass::~ColorDestinationRenderPass()
{
    DebugLog("~ColorDestinationRenderPass()");
    vkDestroyRenderPass(_device.LogicalDevice(), _renderPass, nullptr);
    _renderPass = VK_NULL_HANDLE;
}

void ColorDestinationRenderPass::CreateRenderPassImpl()
{
    const Device& device = _device;
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = getFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // The image will automatically be transitioned from UNDEFINED to COLOR_ATTACHMENT_OPTIMAL for rendering, then out to PRESENT_SRC_KHR at the end.
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

//    VkAttachmentDescription depthAttachment = {};
//    depthAttachment.format = FindSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, deviceInfo.physicalDevice);
//    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
//    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorReference = {};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

//    VkAttachmentReference depthReference = {};
//    depthReference.attachment = 1;
//    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;
//    subpassDescription.pDepthStencilAttachment = &depthReference;
//    subpassDescription.pDepthStencilAttachment = nullptr;

    VkAttachmentDescription attachments[] = { colorAttachment };//, depthAttachment };
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;//2;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;

    VkSubpassDependency dependencies[] = { {}, {} };
    if (device.FamilyQueues().graphics.index == device.FamilyQueues().present.index) {
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        renderPassInfo.dependencyCount = 2;
        renderPassInfo.pDependencies = dependencies;
    }
    VK_CHECK_RESULT(vkCreateRenderPass(device.LogicalDevice(), &renderPassInfo, nullptr, &_renderPass));
}
