#ifndef VULKAN_UTILITY_H
#define VULKAN_UTILITY_H

#include "../log/log.h"
#ifdef __ANDROID__
#include "vulkan_wrapper.h"
#endif
#include "../data_type.h"
#include "instance.h"
#include "surface.h"
#include "device.h"
#include "swapchain.h"
#include "renderpass/renderpass.h"
#include "framebuffer.h"

#include <string>
#include <vector>
#include <stdexcept>

using Utility::Log;
using Vulkan::Instance;
using Vulkan::Surface;
using Vulkan::Device;
using Vulkan::Swapchain;
using Vulkan::RenderPass;
using Vulkan::Framebuffer;
using std::runtime_error;

#define VK_CHECK_RESULT(func)											                \
{																		                \
    VkResult res = (func);                                                              \
    if (res != VK_SUCCESS) {                                                            \
        Log::Error("%s: code[%d], file[%s], line[%d]", #func, res, __FILE__, __LINE__); \
        throw runtime_error("");                                                        \
    }                                                                                   \
}


// ==== Instance ==== //
uint32_t GetAPIVersion();
void AppendInstanceExtension(std::vector<const char*>& instanceExtensionNames);
void BuildInstance(Instance& instance,
                   LayerAndExtension& layerAndExt,
                   vector<const char*> instExtNames = { VK_KHR_SURFACE_EXTENSION_NAME },
                   vector<const char*> instLayerNames = { LayerAndExtension::GOOGLE_THREADING_LAYER, LayerAndExtension::GOOGLE_UNIQUE_OBJECT_LAYER,
                                                          LayerAndExtension::LUNARG_CORE_VALIDATION_LAYER, LayerAndExtension::LUNARG_OBJECT_TRACKER_LAYER,
                                                          LayerAndExtension::LUNARG_PARAMETER_VALIDATION_LAYER });


// ==== Device ==== //
Device SelectPhysicalDevice(const Instance& instance, Surface& surface, vector<const char*> extensionNamesRequested = { VK_KHR_SWAPCHAIN_EXTENSION_NAME }, VkQueueFlags queuesRequested = (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT), VkPhysicalDeviceFeatures requestedFeatures = {});
bool IsPhysicalDeviceSuitable(Device& device, VkQueueFlags queuesRequested, const vector<const char*>& deviceExtensionNamesRequested, const VkPhysicalDeviceFeatures& featuresRequested, Surface& surface);
uint32_t MapMemoryTypeToIndex(VkPhysicalDevice physicalDevice, uint32_t memoryTypeBits, VkMemoryPropertyFlags requestedProperties);


// ==== Swapchain ==== //
VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const vector<VkSurfaceFormatKHR>& formatsSupported);
VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);
VkPresentModeKHR ChooseSwapchainPresentMode(const vector<VkPresentModeKHR>& availablePresentModes, const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
void BuildSwapchain(Vulkan::Swapchain& swapchain);
VkResult QueuePresent(const VkSwapchainKHR* pSwapchains,
                      const uint32_t*       pImageIndices,

                      const Device&         device,

                      const VkSemaphore*    pWaitSemaphores    = nullptr,
                      uint32_t              waitSemaphoreCount = 0,
                      uint32_t              swapchainCount     = 1,
                      VkResult*             pResults           = nullptr,
                      const void*           pNext              = nullptr);
VkImageView CreateImageView(VkImage                image,
                            VkImageViewType        viewType,
                            VkFormat               format,
                            VkImageAspectFlags     aspectMask,
                            uint32_t               baseMipLevel,
                            uint32_t               levelCount,
                            uint32_t               baseArrayLayer,
                            uint32_t               layerCount,

                            VkDevice               device,

                            const void*            pNext = nullptr,
                            VkImageViewCreateFlags flags = 0,
                            VkComponentSwizzle     r     = VK_COMPONENT_SWIZZLE_IDENTITY,
                            VkComponentSwizzle     g     = VK_COMPONENT_SWIZZLE_IDENTITY,
                            VkComponentSwizzle     b     = VK_COMPONENT_SWIZZLE_IDENTITY,
                            VkComponentSwizzle     a     = VK_COMPONENT_SWIZZLE_IDENTITY);


// ==== RenderPass ==== //
template <typename T>
void BuildRenderPass(RenderPass*& renderPass, const Vulkan::Swapchain& swapchain, const Device& device)
{
    renderPass = new T(device);
    renderPass->getFormat = [&swapchain]() -> VkFormat { return swapchain.Format(); };
    renderPass->CreateRenderPass();
}


// ==== Framebuffer ==== //
VkFramebuffer CreateFramebuffer(VkRenderPass renderPass,
                                uint32_t attachmentCount,
                                const VkImageView *pAttachments,
                                uint32_t width,
                                uint32_t height,
                                uint32_t layers,

                                VkDevice device,

                                const void *pNext = nullptr,
                                VkFramebufferCreateFlags flags = 0);
void CreateFramebuffers(vector<Framebuffer>& framebuffers,const Swapchain& swapchain, const RenderPass* renderPass, const Device& device);


VkFence CreateFence(VkDevice device, VkFenceCreateFlags flags = VK_FENCE_CREATE_SIGNALED_BIT, const void* pNext = nullptr);
VkSemaphore CreateSemaphore(VkDevice device, VkSemaphoreCreateFlags flags = 0, const void* pNext = nullptr);


//typedef struct GraphicsPipelineInfoParameters {
//    const VkPipelineDepthStencilStateCreateInfo*  pDepthStencilState = nullptr;
//    const VkPipelineMultisampleStateCreateInfo*   pMultisampleState  = nullptr;
//    const VkPipelineDynamicStateCreateInfo*       pDynamicState      = nullptr;
//    uint32_t                                      subpass            = 0;
//    const VkPipelineTessellationStateCreateInfo*  pTessellationState = nullptr;
//    VkPipeline                                    basePipelineHandle = VK_NULL_HANDLE;
//    int32_t                                       basePipelineIndex  = -1;
//    const void*                                   pNext              = nullptr;
//    VkPipelineCreateFlags                         flags              = 0;
//} GraphicsPipelineInfoParameters;
//VkGraphicsPipelineCreateInfo CreateGraphicsPipelineInfo(uint32_t                                      stageCount,
//                                                        const VkPipelineShaderStageCreateInfo*        pStages,
//                                                        const VkPipelineVertexInputStateCreateInfo*   pVertexInputState,
//                                                        const VkPipelineInputAssemblyStateCreateInfo* pInputAssemblyState,
//                                                        const VkPipelineViewportStateCreateInfo*      pViewportState,
//                                                        const VkPipelineRasterizationStateCreateInfo* pRasterizationState,
//                                                        const VkPipelineColorBlendStateCreateInfo*    pColorBlendState,
//                                                        VkPipelineLayout                              layout,
//                                                        VkRenderPass                                  renderPass,
//                                                        GraphicsPipelineInfoParameters                optionalParameters = {});
//VkPipeline CreateGraphicsPipelines(VkDevice                            device,
//                                   const VkGraphicsPipelineCreateInfo* pCreateInfos,
//                                   uint32_t                            createInfoCount = 1,
//                                   VkPipelineCache                     pipelineCache   = VK_NULL_HANDLE,
//                                   const VkAllocationCallbacks*        pAllocator      = nullptr);
//
//VkPipeline BuildDefaultGraphicsPipeline()
//{
//
//}
//
//
//VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice);
//VkImage CreateTexture2DImage(VkFormat          format,
//                             const Extent2D&   extent,
//                             uint32_t          mipLevels,
//                             VkImageUsageFlags usage,
//                             VkDevice          device);
//void TransitionImageLayout(VkPipelineStageFlags sourceStage,
//                           VkPipelineStageFlags destinationStage,
//                           VkAccessFlags        srcAccessMask,
//                           VkAccessFlags        dstAccessMask,
//                           VkImageLayout        oldLayout,
//                           VkImageLayout        newLayout,
//                           VkImage              image,
//                           VkImageAspectFlags   aspectMask,
//
//                           VkCommandBuffer      commandBuffer,
//
//                           const void*          pNext               = nullptr,
//                           uint32_t             srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//                           uint32_t             dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//                           uint32_t             baseMipLevel        = 0,
//                           uint32_t             levelCount          = 1,
//                           uint32_t             baseArrayLayer      = 0,
//                           uint32_t             layerCount          = 1);

#endif // VULKAN_UTILITY_H