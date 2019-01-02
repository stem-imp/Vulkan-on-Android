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
#include "command.h"

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
using Vulkan::Command;
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
bool BuildInstance(Instance& instance,
                   LayerAndExtension&  layerAndExtension,
                   vector<const char*> requestedInstanceExtensionNames = { VK_KHR_SURFACE_EXTENSION_NAME },
                   vector<const char*> requestedInstanceLayerNames     = { LayerAndExtension::GOOGLE_THREADING_LAYER, LayerAndExtension::GOOGLE_UNIQUE_OBJECT_LAYER,
                                                                           LayerAndExtension::LUNARG_CORE_VALIDATION_LAYER, LayerAndExtension::LUNARG_OBJECT_TRACKER_LAYER,
                                                                           LayerAndExtension::LUNARG_PARAMETER_VALIDATION_LAYER });


// ==== Device ==== //
Device SelectPhysicalDevice(const Instance& instance, Surface& surface, vector<const char*> extensionNamesRequested = { VK_KHR_SWAPCHAIN_EXTENSION_NAME }, VkQueueFlags queuesRequested = (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT), VkPhysicalDeviceFeatures requestedFeatures = {});
bool IsPhysicalDeviceSuitable(Device&                         device,
                              VkQueueFlags                    queuesRequested,
                              const vector<const char*>&      deviceExtensionNamesRequested,
                              const VkPhysicalDeviceFeatures& featuresRequested,
                              Surface&                        surface,
                              bool                            needPresent = true);
uint32_t MapMemoryTypeToIndex(VkPhysicalDevice physicalDevice, uint32_t memoryTypeBits, VkMemoryPropertyFlags requestedProperties);
VkFormat FindDeviceSupportedFormat(const vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, const VkPhysicalDevice physicalDevice);


// ==== Swapchain ==== //
VkSurfaceFormatKHR ChooseSurfaceFormat(const vector<VkSurfaceFormatKHR>& formatsSupported,
                                       const vector<VkSurfaceFormatKHR> preferredFormats = {
                                               {VK_FORMAT_R8G8B8A8_UNORM,
                                                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
                                               {VK_FORMAT_B8G8R8A8_UNORM,
                                                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
                                               {VK_FORMAT_R8G8B8A8_SRGB,
                                                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
                                               {VK_FORMAT_B8G8R8A8_SRGB,
                                                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
                                       });
VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);
VkPresentModeKHR ChooseSwapchainPresentMode(const vector<VkPresentModeKHR>& availablePresentModes, const VkSurfaceCapabilitiesKHR& surfaceCapabilities, bool vSync = false);
VkSwapchainCreateInfoKHR SwapchainCreateInfo(VkSurfaceKHR                  surface,
                                             uint32_t                      minImageCount,
                                             VkFormat                      imageFormat,
                                             VkColorSpaceKHR               imageColorSpace,
                                             VkExtent2D                    imageExtent,
                                             VkImageUsageFlags             imageUsage,
                                             VkSharingMode                 imageSharingMode,
                                             uint32_t                      queueFamilyIndexCount,
                                             const uint32_t*               pQueueFamilyIndices,
                                             VkSurfaceTransformFlagBitsKHR preTransform,
                                             VkCompositeAlphaFlagBitsKHR   compositeAlpha,
                                             VkPresentModeKHR              presentMode,
                                             VkBool32                      clipped,
                                             VkSwapchainKHR                oldSwapchain,

                                             uint32_t                      imageArrayLayers = 1,
                                             const void*                   pNext            = 0,
                                             VkSwapchainCreateFlagsKHR     flags            = 0);
void BuildSwapchain(Vulkan::Swapchain& swapchain);
VkResult QueuePresent(const VkSwapchainKHR* pSwapchains,
                      const uint32_t*       pImageIndices,

                      const Device&         device,

                      uint32_t              waitSemaphoreCount = 0,
                      const VkSemaphore*    pWaitSemaphores    = nullptr,
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


// ==== Buffer ==== //
//VkBuffer CreateBuffer(VkDeviceSize        size,
//                      VkBufferUsageFlags  usage,
//                      VkSharingMode       sharingMode,
//                      uint32_t            queueFamilyIndexCount,
//                      const uint32_t*     pQueueFamilyIndices,
//                      VkDevice            device,
//
//                      const void*         pNext = nullptr,
//                      VkBufferCreateFlags flags = 0);
//
//VkDeviceMemory AllocateAndBindBuffer(VkBuffer buffer,
//                                     uint32_t memoryTypeIndex,
//                                     VkDevice device,
//
//                                     const void*           pNext,
//                                     VkMemoryRequirements* memoryRequirements);


// Image
VkImageCreateInfo ImageCreateInfo(VkFormat              format,
                                  VkExtent3D            extent,
                                  uint32_t              mipLevels,
                                  VkImageUsageFlags     usage,

                                  VkImageTiling         tiling                 = VK_IMAGE_TILING_OPTIMAL,
                                  VkImageLayout         initialLayout          = VK_IMAGE_LAYOUT_UNDEFINED,
                                  VkSampleCountFlagBits samples                = VK_SAMPLE_COUNT_1_BIT,
                                  uint32_t              arrayLayers            = 1,
                                  VkImageType           imageType              = VK_IMAGE_TYPE_2D,
                                  VkImageCreateFlags    flags                  = 0,
                                  VkSharingMode         sharingMode            = VK_SHARING_MODE_EXCLUSIVE,
                                  uint32_t              queueFamilyIndexCount  = 0,
                                  const uint32_t*       pQueueFamilyIndices    = nullptr,
                                  const void*           pNext                  = nullptr);

VkImageMemoryBarrier ImageMemoryBarrier(VkAccessFlags           srcAccessMask,
                                        VkAccessFlags           dstAccessMask,
                                        VkImageLayout           oldLayout,
                                        VkImageLayout           newLayout,
                                        VkImage                 image,
                                        VkImageSubresourceRange subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
                                        uint32_t                srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                        uint32_t                dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                        const void*             pNext               = nullptr);
typedef struct PipelineBarrierParameters {
    PipelineBarrierParameters()
    {
        srcStageMask             = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        dstStageMask             = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        dependencyFlags          = 0;
        memoryBarrierCount       = 0;
        pMemoryBarriers          = nullptr;
        bufferMemoryBarrierCount = 0;
        pBufferMemoryBarriers    = nullptr;
        imageMemoryBarrierCount  = 0;
        pImageMemoryBarriers     = nullptr;
    }
    VkCommandBuffer              commandBuffer;
    VkPipelineStageFlags         srcStageMask;
    VkPipelineStageFlags         dstStageMask;
    VkDependencyFlags            dependencyFlags;
    uint32_t                     memoryBarrierCount;
    const VkMemoryBarrier*       pMemoryBarriers;
    uint32_t                     bufferMemoryBarrierCount;
    const VkBufferMemoryBarrier* pBufferMemoryBarriers;
    uint32_t                     imageMemoryBarrierCount;
    const VkImageMemoryBarrier*  pImageMemoryBarriers;
} PipelineBarrierParameters;
void TransitionImageLayout(PipelineBarrierParameters* parameters);

VkMemoryAllocateInfo MemoryAllocateInfo(VkImage               image,
                                        VkMemoryPropertyFlags requestedProperties,
                                        const Device&         device,
                                        VkMemoryRequirements& memoryRequirements,
                                        uint32_t&             memoryTypeIndex,

                                        void*                 pNext = nullptr);
VkImageViewCreateInfo ImageViewCreateInfo(VkImage                 image,
                                          VkFormat                format,
                                          VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
                                          VkImageViewType         viewType         = VK_IMAGE_VIEW_TYPE_2D,
                                          VkComponentMapping      components       = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
                                          const void*             pNext            = nullptr,
                                          VkImageViewCreateFlags  flags            = 0);


// ==== Shader ==== //
template <typename T>
VkShaderModuleCreateInfo ShaderModuleCreateInfo(vector<T>& fileContent)
{
    VkShaderModuleCreateInfo shaderModuleInfo = {};
    shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleInfo.codeSize = fileContent.size();
    shaderModuleInfo.pCode = (const uint32_t*)fileContent.data();
    return shaderModuleInfo;
}


// ==== Pipeline ==== //
typedef struct GraphicsPipelineInfoParameters {
    const VkPipelineDepthStencilStateCreateInfo* pDepthStencilState = nullptr;
    const VkPipelineMultisampleStateCreateInfo*  pMultisampleState  = nullptr;
    const VkPipelineDynamicStateCreateInfo*      pDynamicState      = nullptr;
    uint32_t                                     subpass            = 0;
    const VkPipelineTessellationStateCreateInfo* pTessellationState = nullptr;
    VkPipeline                                   basePipelineHandle = VK_NULL_HANDLE;
    int32_t                                      basePipelineIndex  = -1;
    const void*                                  pNext              = nullptr;
    VkPipelineCreateFlags                        flags              = 0;
} GraphicsPipelineInfoParameters;
VkGraphicsPipelineCreateInfo CreateGraphicsPipelineInfo(uint32_t                                      stageCount,
                                                        const VkPipelineShaderStageCreateInfo*        pStages,
                                                        const VkPipelineVertexInputStateCreateInfo*   pVertexInputState,
                                                        const VkPipelineInputAssemblyStateCreateInfo* pInputAssemblyState,
                                                        const VkPipelineViewportStateCreateInfo*      pViewportState,
                                                        const VkPipelineRasterizationStateCreateInfo* pRasterizationState,
                                                        const VkPipelineColorBlendStateCreateInfo*    pColorBlendState,
                                                        VkPipelineLayout                              layout,
                                                        VkRenderPass                                  renderPass,
                                                        GraphicsPipelineInfoParameters                optionalParameters = GraphicsPipelineInfoParameters());
VkPipeline CreateGraphicsPipelines(VkDevice                            device,
                                   const VkGraphicsPipelineCreateInfo* pCreateInfos,
                                   uint32_t                            createInfoCount = 1,
                                   VkPipelineCache                     pipelineCache   = VK_NULL_HANDLE,
                                   const VkAllocationCallbacks*        pAllocator      = nullptr);
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

#endif // VULKAN_UTILITY_H