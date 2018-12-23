#include "vulkan_utility.h"
#include "device.h"
#include <string>

using Vulkan::Device;
using std::vector;

// ==== Instance ==== //
bool BuildInstance(Instance& instance, LayerAndExtension& layerAndExtension, vector<const char*> requestedInstanceExtensionNames, vector<const char*> requestedInstanceLayerNames)
{
    AppendInstanceExtension(requestedInstanceExtensionNames);

    if (layerAndExtension.enableValidationLayers) {
        requestedInstanceExtensionNames.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        layerAndExtension.EnableInstanceLayers(requestedInstanceLayerNames);
    }
    if (!layerAndExtension.EnableInstanceExtensions(requestedInstanceExtensionNames)) {
        return false;
    }
    instance.CreateInstance(layerAndExtension);
    if (layerAndExtension.enableValidationLayers) {
        if (!layerAndExtension.HookDebugReportExtension(instance.GetInstance())) {
            DebugLog("Enable debugging feature failed.");
        }
    }
    return true;
}




// ==== Device ==== //
static bool DeviceFeaturesCompare(const VkPhysicalDeviceFeatures& requested, const VkPhysicalDeviceFeatures& supported);

Device SelectPhysicalDevice(const Instance& instance, Surface& surface, vector<const char*> extensionNamesRequested, VkQueueFlags queuesRequested, VkPhysicalDeviceFeatures requestedFeatures)
{
    // Find one GPU to use:
    // On Android, every GPU device is equal -- supporting
    // graphics/compute/present
    uint32_t gpuCount = 0;
    vector<VkPhysicalDevice> tmpGpus;
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance.GetInstance(), &gpuCount, nullptr));
    tmpGpus.resize(gpuCount);
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance.GetInstance(), &gpuCount, tmpGpus.data()));

    for (const auto& d : tmpGpus) {
        Device device(d);
        const vector<const char*> enabledInstanceLayers = instance.LayersEnabled();
        device.EnumerateExtensions(enabledInstanceLayers);
        if (IsPhysicalDeviceSuitable(device, queuesRequested, extensionNamesRequested, requestedFeatures, surface)) {
            device.queueFlags = queuesRequested;
            return device;
        }
    }
    throw runtime_error("Could not find suitable physical device.");
}

bool IsPhysicalDeviceSuitable(Device&                         device,
                              VkQueueFlags                    queuesRequested,
                              const vector<const char*>&      deviceExtensionNamesRequested,
                              const VkPhysicalDeviceFeatures& featuresRequested,
                              Surface&                        surface,
                              bool                            needPresent)
{
    // Check Requested Queues
    VkPhysicalDevice physicalDevice = device.PhysicalDevice();
    VkQueueFlags queueFound = 0;
    if (queuesRequested & VK_QUEUE_GRAPHICS_BIT) {
        if (device.GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT, surface.GetSurface()) != -1) {
            queueFound = (queueFound | VK_QUEUE_GRAPHICS_BIT);
        }
    }
    if (queuesRequested & VK_QUEUE_COMPUTE_BIT) {
        if (device.GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT) != -1) {
            queueFound = (queueFound | VK_QUEUE_COMPUTE_BIT);
        }
    }
    if (queuesRequested & VK_QUEUE_TRANSFER_BIT) {
        if (device.GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT) != -1) {
            queueFound = (queueFound | VK_QUEUE_TRANSFER_BIT);
        }
    }
    if (queuesRequested & VK_QUEUE_SPARSE_BINDING_BIT) {
        if (device.GetQueueFamilyIndex(VK_QUEUE_SPARSE_BINDING_BIT) != -1) {
            queueFound = (queueFound | VK_QUEUE_SPARSE_BINDING_BIT);
        }
    }
    if (queueFound != queuesRequested) {
        return false;
    }
    if (needPresent && device.FamilyQueues().present.index == -1) {
        return false;
    }

    // Check Requested Extensions
    if (!device.EnableDeviceExtensions(deviceExtensionNamesRequested)) {
        return false;
    }

    // Check Surface Capabilities
    Surface::SurfaceSupportInfo supportInfo = surface.QuerySurfaceSupport(physicalDevice);
    if (supportInfo.formats.empty() || supportInfo.presentModes.empty()) {
        return false;
    }

    // Check Physical Device Features
    const VkPhysicalDeviceFeatures& featuresSupported = device.FeaturesSupported();
    if (!DeviceFeaturesCompare(featuresRequested, featuresSupported)) {
        return false;
    }

    return true;
}

bool DeviceFeaturesCompare(const VkPhysicalDeviceFeatures& requested, const VkPhysicalDeviceFeatures& supported)
{
    bool pass = true;
    pass = pass && (!requested.robustBufferAccess || supported.robustBufferAccess);
    pass = pass && (!requested.fullDrawIndexUint32 || supported.fullDrawIndexUint32);
    pass = pass && (!requested.imageCubeArray || supported.imageCubeArray);
    pass = pass && (!requested.independentBlend || supported.independentBlend);
    pass = pass && (!requested.geometryShader || supported.geometryShader);
    pass = pass && (!requested.tessellationShader || supported.tessellationShader);
    pass = pass && (!requested.sampleRateShading || supported.sampleRateShading);
    pass = pass && (!requested.dualSrcBlend || supported.dualSrcBlend);
    pass = pass && (!requested.logicOp || supported.logicOp);
    pass = pass && (!requested.multiDrawIndirect || supported.multiDrawIndirect);
    pass = pass && (!requested.drawIndirectFirstInstance || supported.drawIndirectFirstInstance);
    pass = pass && (!requested.depthClamp || supported.depthClamp);
    pass = pass && (!requested.depthBiasClamp || supported.depthBiasClamp);
    pass = pass && (!requested.fillModeNonSolid || supported.fillModeNonSolid);
    pass = pass && (!requested.depthBounds || supported.depthBounds);
    pass = pass && (!requested.wideLines || supported.wideLines);
    pass = pass && (!requested.largePoints || supported.largePoints);
    pass = pass && (!requested.alphaToOne || supported.alphaToOne);
    pass = pass && (!requested.multiViewport || supported.multiViewport);
    pass = pass && (!requested.samplerAnisotropy || supported.samplerAnisotropy);
    pass = pass && (!requested.textureCompressionETC2 || supported.textureCompressionETC2);
    pass = pass && (!requested.textureCompressionASTC_LDR || supported.textureCompressionASTC_LDR);
    pass = pass && (!requested.textureCompressionBC || supported.textureCompressionBC);
    pass = pass && (!requested.occlusionQueryPrecise || supported.occlusionQueryPrecise);
    pass = pass && (!requested.pipelineStatisticsQuery || supported.pipelineStatisticsQuery);
    pass = pass && (!requested.vertexPipelineStoresAndAtomics || supported.vertexPipelineStoresAndAtomics);
    pass = pass && (!requested.fragmentStoresAndAtomics || supported.fragmentStoresAndAtomics);
    pass = pass && (!requested.shaderTessellationAndGeometryPointSize || supported.shaderTessellationAndGeometryPointSize);
    pass = pass && (!requested.shaderImageGatherExtended || supported.shaderImageGatherExtended);
    pass = pass && (!requested.shaderStorageImageExtendedFormats || supported.shaderStorageImageExtendedFormats);
    pass = pass && (!requested.shaderStorageImageMultisample || supported.shaderStorageImageMultisample);
    pass = pass && (!requested.shaderStorageImageReadWithoutFormat || supported.shaderStorageImageReadWithoutFormat);
    pass = pass && (!requested.shaderStorageImageWriteWithoutFormat || supported.shaderStorageImageWriteWithoutFormat);
    pass = pass && (!requested.shaderUniformBufferArrayDynamicIndexing || supported.shaderUniformBufferArrayDynamicIndexing);
    pass = pass && (!requested.shaderSampledImageArrayDynamicIndexing || supported.shaderSampledImageArrayDynamicIndexing);
    pass = pass && (!requested.shaderStorageBufferArrayDynamicIndexing || supported.shaderStorageBufferArrayDynamicIndexing);
    pass = pass && (!requested.shaderStorageImageArrayDynamicIndexing || supported.shaderStorageImageArrayDynamicIndexing);
    pass = pass && (!requested.shaderClipDistance || supported.shaderClipDistance);
    pass = pass && (!requested.shaderCullDistance || supported.shaderCullDistance);
    pass = pass && (!requested.shaderFloat64 || supported.shaderFloat64);
    pass = pass && (!requested.shaderInt64 || supported.shaderInt64);
    pass = pass && (!requested.shaderInt16 || supported.shaderInt16);
    pass = pass && (!requested.shaderResourceResidency || supported.shaderResourceResidency);
    pass = pass && (!requested.shaderResourceMinLod || supported.shaderResourceMinLod);
    pass = pass && (!requested.sparseBinding || supported.sparseBinding);
    pass = pass && (!requested.sparseResidencyBuffer || supported.sparseResidencyBuffer);
    pass = pass && (!requested.sparseResidencyImage2D || supported.sparseResidencyImage2D);
    pass = pass && (!requested.sparseResidencyImage3D || supported.sparseResidencyImage3D);
    pass = pass && (!requested.sparseResidency2Samples || supported.sparseResidency2Samples);
    pass = pass && (!requested.sparseResidency4Samples || supported.sparseResidency4Samples);
    pass = pass && (!requested.sparseResidency8Samples || supported.sparseResidency8Samples);
    pass = pass && (!requested.sparseResidency16Samples || supported.sparseResidency16Samples);
    pass = pass && (!requested.sparseResidencyAliased || supported.sparseResidencyAliased);
    pass = pass && (!requested.variableMultisampleRate || supported.variableMultisampleRate);
    pass = pass && (!requested.inheritedQueries || supported.inheritedQueries);
    return pass;
}

uint32_t MapMemoryTypeToIndex(VkPhysicalDevice physicalDevice, uint32_t memoryTypeBits, VkMemoryPropertyFlags requestedProperties)
{
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);

    for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++) {
        if ((memoryTypeBits & (1 << i)) && (deviceMemoryProperties.memoryTypes[i].propertyFlags & requestedProperties) == requestedProperties) {
            return i;
        }
    }
    throw runtime_error(string("Cannot find requested properties: " + std::to_string(requestedProperties)).data());
}




// ==== Swapchain ==== //
VkSurfaceFormatKHR ChooseSurfaceFormat(const vector<VkSurfaceFormatKHR>& formatsSupported,
                                       const vector<VkSurfaceFormatKHR> preferredFormats)
{
    if (formatsSupported.size() == 1 && formatsSupported[0].format == VK_FORMAT_UNDEFINED) {
        assert(preferredFormats.size());
        return preferredFormats[0];
    }

    for (const auto& preferredFormat : preferredFormats) {
        for (const auto& availableFormat : formatsSupported) {
            if (availableFormat.format == preferredFormat.format &&
                availableFormat.colorSpace == preferredFormat.colorSpace) {
                return availableFormat;
            }
        }
    }
    for (const auto& preferredFormat : preferredFormats) {
        for (const auto& availableFormat : formatsSupported) {
            if (availableFormat.format == preferredFormat.format) {
                return availableFormat;
            }
        }
    }

    return formatsSupported[0];
}

VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height)
{
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

VkPresentModeKHR ChooseSwapchainPresentMode(const vector<VkPresentModeKHR>& availablePresentModes, const VkSurfaceCapabilitiesKHR& surfaceCapabilities, bool vSync)
{
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

    if (!vSync) {
        // Choose mode of mailbox, fifo relaxed, immediate mode(in order).
        bool triBufferingCapable = surfaceCapabilities.maxImageCount >= 3 || surfaceCapabilities.maxImageCount == 0;
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR && triBufferingCapable) {
                bestMode = availablePresentMode;
                break;
            } else if (bestMode != VK_PRESENT_MODE_MAILBOX_KHR && availablePresentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR) {
                bestMode = availablePresentMode;
            } else if (bestMode != VK_PRESENT_MODE_MAILBOX_KHR && bestMode != VK_PRESENT_MODE_FIFO_RELAXED_KHR &&
                       availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            {
                bestMode = availablePresentMode;
            }
        }
    }
    return bestMode;
}

void BuildSwapchain(Swapchain& swapchain)
{
    swapchain.CreateSwapChain();
    swapchain.CreateImageViews();
}

VkResult QueuePresent(const VkSwapchainKHR* pSwapchains,
                      const uint32_t*       pImageIndices,

                      const Device&         device,

                      const VkSemaphore*    pWaitSemaphores,
                      uint32_t              waitSemaphoreCount,
                      uint32_t              swapchainCount,
                      VkResult*             pResults,
                      const void*           pNext)
{
    VkPresentInfoKHR presentInfo   = {};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext              = pNext;
    presentInfo.waitSemaphoreCount = waitSemaphoreCount;
    presentInfo.pWaitSemaphores    = pWaitSemaphores;
    presentInfo.swapchainCount     = swapchainCount;
    presentInfo.pSwapchains        = pSwapchains;
    presentInfo.pImageIndices      = pImageIndices;
    presentInfo.pResults           = pResults;
    VkResult result = vkQueuePresentKHR(device.FamilyQueues().present.queue, &presentInfo);
    return result;
}




// ==== Framebuffer ==== //
VkFramebuffer CreateFramebuffer(VkRenderPass renderPass,
                                uint32_t attachmentCount,
                                const VkImageView *pAttachments,
                                uint32_t width,
                                uint32_t height,
                                uint32_t layers,
                                VkDevice device,

                                const void *pNext,
                                VkFramebufferCreateFlags flags)
{
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext           = pNext;
    framebufferInfo.flags           = flags;
    framebufferInfo.renderPass      = renderPass;
    framebufferInfo.attachmentCount = attachmentCount;
    framebufferInfo.pAttachments    = pAttachments;
    framebufferInfo.width           = width;
    framebufferInfo.height          = height;
    framebufferInfo.layers          = layers;
    VkFramebuffer framebuffer;
    VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer));
    return framebuffer;
}

void CreateFramebuffers(vector<Framebuffer>& framebuffers,const Swapchain& swapchain, const RenderPass* renderPass, const Device& device)
{
    const vector<VkImageView> &imageViews = swapchain.ImageViews();
    size_t swapchainSize = imageViews.size();
    framebuffers.clear();
    for (int i = 0; i < swapchainSize; i++) {
        framebuffers.emplace_back(device);
        const vector<VkImageView> individuals = {imageViews[i]};
        framebuffers[i].CreateSwapchainFramebuffer(renderPass->GetRenderPass(), individuals, swapchain.Extent());
    }
}




// ==== Synchronization ==== //
VkFence CreateFence(VkDevice device, VkFenceCreateFlags flags, const void* pNext)
{
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = pNext;
    fenceInfo.flags = flags;
    VkFence fence;
    VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &fence));
    return fence;
}

VkSemaphore CreateSemaphore(VkDevice device, VkSemaphoreCreateFlags flags, const void* pNext)
{
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreInfo.flags = 0;
    semaphoreInfo.pNext = nullptr;
    VkSemaphore semaphore;
    VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore));
    return semaphore;
}




//// ==== Graphics Pipeline ==== //
//VkGraphicsPipelineCreateInfo CreateGraphicsPipelineInfo(uint32_t                                      stageCount,
//                                                        const VkPipelineShaderStageCreateInfo*        pStages,
//                                                        const VkPipelineVertexInputStateCreateInfo*   pVertexInputState,
//                                                        const VkPipelineInputAssemblyStateCreateInfo* pInputAssemblyState,
//                                                        const VkPipelineViewportStateCreateInfo*      pViewportState,
//                                                        const VkPipelineRasterizationStateCreateInfo* pRasterizationState,
//                                                        const VkPipelineColorBlendStateCreateInfo*    pColorBlendState,
//                                                        VkPipelineLayout                              layout,
//                                                        VkRenderPass                                  renderPass,
//                                                        GraphicsPipelineInfoParameters                optionalParameters)
//{
//    VkGraphicsPipelineCreateInfo graphicsPipelineInfo = {};
//    graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//    graphicsPipelineInfo.pNext               = optionalParameters.pNext;
//    graphicsPipelineInfo.flags               = optionalParameters.flags;
//    graphicsPipelineInfo.stageCount          = stageCount;
//    graphicsPipelineInfo.pStages             = pStages;
//    graphicsPipelineInfo.pVertexInputState   = pVertexInputState;
//    graphicsPipelineInfo.pInputAssemblyState = pInputAssemblyState;
//    graphicsPipelineInfo.pTessellationState  = optionalParameters.pTessellationState;
//    graphicsPipelineInfo.pViewportState      = pViewportState;
//    graphicsPipelineInfo.pRasterizationState = pRasterizationState;
//    graphicsPipelineInfo.pMultisampleState   = optionalParameters.pMultisampleState;
//    graphicsPipelineInfo.pDepthStencilState  = optionalParameters.pDepthStencilState;
//    graphicsPipelineInfo.pColorBlendState    = pColorBlendState;
//    graphicsPipelineInfo.pDynamicState       = optionalParameters.pDynamicState;
//    graphicsPipelineInfo.layout              = layout;
//    graphicsPipelineInfo.renderPass          = renderPass;
//    graphicsPipelineInfo.subpass             = optionalParameters.subpass;
//    graphicsPipelineInfo.basePipelineHandle  = optionalParameters.basePipelineHandle;
//    graphicsPipelineInfo.basePipelineIndex   = optionalParameters.basePipelineIndex;
//    return graphicsPipelineInfo;
//}
//
//VkPipeline CreateGraphicsPipelines(VkDevice                            device,
//                                   const VkGraphicsPipelineCreateInfo* pCreateInfos,
//                                   uint32_t                            createInfoCount,
//                                   VkPipelineCache                     pipelineCache,
//                                   const VkAllocationCallbacks*        pAllocator)
//{
//    VkPipeline pipeline;
//    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, &pipeline));
//    return pipeline;
//}
//
//
//
//
//// ==== Image ==== //
//VkFormat FindSupportedFormat(const vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice)
//{
//    for (VkFormat format : candidates) {
//        VkFormatProperties props;
//        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
//
//        if ((tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) ||
//            (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features))
//        {
//            return format;
//        }
//    }
//
//    string tilingCode = std::to_string(tiling);
//    string feature = std::to_string(features);
//    assert(("Failed to find supported format for tiling[" + tilingCode + "] and feature mask[" + feature + "].").data() && false);
//}
//
//VkImage CreateImage(VkImageType           imageType,
//                    VkFormat              format,
//                    VkExtent3D            extent,
//                    uint32_t              mipLevels,
//                    uint32_t              arrayLayers,
//                    VkSampleCountFlagBits samples,
//                    VkImageTiling         tiling,
//                    VkImageUsageFlags     usage,
//                    VkSharingMode         sharingMode,
//                    uint32_t              queueFamilyIndexCount,
//                    const uint32_t*       pQueueFamilyIndices,
//                    VkImageLayout         initialLayout,
//
//                    VkDevice              device,
//
//                    const void*           pNext = nullptr,
//                    VkImageCreateFlags    flags = 0)
//{
//    VkImageCreateInfo imageInfo = {};
//    imageInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
//    imageInfo.pNext                 = pNext;
//    imageInfo.flags                 = flags;
//    imageInfo.imageType             = imageType;
//    imageInfo.format                = format;
//    imageInfo.extent                = extent;
//    imageInfo.mipLevels             = mipLevels;
//    imageInfo.arrayLayers           = arrayLayers;
//    imageInfo.samples               = samples;
//    imageInfo.tiling                = tiling;
//    imageInfo.usage                 = usage;
//    imageInfo.sharingMode           = sharingMode;
//    imageInfo.queueFamilyIndexCount = queueFamilyIndexCount;
//    imageInfo.pQueueFamilyIndices   = pQueueFamilyIndices;
//    imageInfo.initialLayout         = initialLayout;
//
//    VkImage image;
//    VK_CHECK_RESULT(vkCreateImage(device, &imageInfo, nullptr, &image));
//
//    return image;
//}
//
//VkImage CreateTexture2DImage(VkFormat          format,
//                             const Extent2D&   extent,
//                             uint32_t          mipLevels,
//                             VkImageUsageFlags usage,
//
//                             VkDevice        device)
//{
//    VkImage image = CreateImage(VK_IMAGE_TYPE_2D,
//                                format,
//                                { extent.width, extent.height, 1 },
//                                mipLevels,
//                                1,                                  // arrayLayers
//                                VK_SAMPLE_COUNT_1_BIT,
//                                VK_IMAGE_TILING_OPTIMAL,
//                                usage,
//                                VK_SHARING_MODE_EXCLUSIVE,
//                                1,                                  // queueFamilyIndexCount
//                                nullptr,                            // pQueueFamilyIndices
//                                VK_IMAGE_LAYOUT_UNDEFINED,
//                                device,
//                                nullptr,                            // pNext
//                                0);                                 // flags
//    return image;
//}

VkImageView CreateImageView(VkImage                image,
                            VkImageViewType        viewType,
                            VkFormat               format,
                            VkImageAspectFlags     aspectMask,
                            uint32_t               baseMipLevel,
                            uint32_t               levelCount,
                            uint32_t               baseArrayLayer,
                            uint32_t               layerCount,
                            VkDevice               device,

                            const void*            pNext,
                            VkImageViewCreateFlags flags,
                            VkComponentSwizzle     r,
                            VkComponentSwizzle     g,
                            VkComponentSwizzle     b,
                            VkComponentSwizzle     a)
{
    VkImageViewCreateInfo imageViewInfo = {};
    imageViewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.pNext                           = pNext;
    imageViewInfo.flags                           = flags;
    imageViewInfo.image                           = image;
    imageViewInfo.viewType                        = viewType;
    imageViewInfo.format                          = format;
    imageViewInfo.components.r                    = r;
    imageViewInfo.components.g                    = g;
    imageViewInfo.components.b                    = b;
    imageViewInfo.components.a                    = a;
    imageViewInfo.subresourceRange.aspectMask     = aspectMask;
    imageViewInfo.subresourceRange.baseMipLevel   = baseMipLevel;
    imageViewInfo.subresourceRange.levelCount     = levelCount;
    imageViewInfo.subresourceRange.baseArrayLayer = baseArrayLayer;
    imageViewInfo.subresourceRange.layerCount     = layerCount;
    VkImageView imageView;
    VK_CHECK_RESULT(vkCreateImageView(device, &imageViewInfo, nullptr, &imageView));

    return imageView;
}

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
//                           const void*          pNext,
//                           uint32_t             srcQueueFamilyIndex,
//                           uint32_t             dstQueueFamilyIndex,
//                           uint32_t             baseMipLevel,
//                           uint32_t             levelCount,
//                           uint32_t             baseArrayLayer,
//                           uint32_t             layerCount)
//{
//    VkImageMemoryBarrier imageMemoryBarrier = {};
//    imageMemoryBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
//    imageMemoryBarrier.pNext                           = pNext;
//    imageMemoryBarrier.srcAccessMask                   = srcAccessMask;
//    imageMemoryBarrier.dstAccessMask                   = dstAccessMask;
//    imageMemoryBarrier.oldLayout                       = oldLayout;
//    imageMemoryBarrier.newLayout                       = newLayout;
//    imageMemoryBarrier.srcQueueFamilyIndex             = srcQueueFamilyIndex;
//    imageMemoryBarrier.dstQueueFamilyIndex             = dstQueueFamilyIndex;
//    imageMemoryBarrier.image                           = image;
//    imageMemoryBarrier.subresourceRange.aspectMask     = aspectMask;
//    imageMemoryBarrier.subresourceRange.baseMipLevel   = baseMipLevel;
//    imageMemoryBarrier.subresourceRange.levelCount     = levelCount;
//    imageMemoryBarrier.subresourceRange.baseArrayLayer = baseArrayLayer;
//    imageMemoryBarrier.subresourceRange.layerCount     = layerCount;
//    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
//}




// ==== Buffer ==== //
VkBuffer CreateBuffer(VkDeviceSize        size,
                      VkBufferUsageFlags  usage,
                      VkSharingMode       sharingMode,
                      uint32_t            queueFamilyIndexCount,
                      const uint32_t*     pQueueFamilyIndices,
                      VkDevice device,

                      const void*         pNext = nullptr,
                      VkBufferCreateFlags flags = 0)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext                 = pNext;
    bufferInfo.flags                 = flags;
    bufferInfo.size                  = size;
    bufferInfo.usage                 = usage;
    bufferInfo.sharingMode           = sharingMode;
    bufferInfo.queueFamilyIndexCount = queueFamilyIndexCount;
    bufferInfo.pQueueFamilyIndices   = pQueueFamilyIndices;

    VkBuffer buffer;
    VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));

    return  buffer;
}

VkDeviceMemory AllocateAndBindBuffer(VkBuffer buffer, uint32_t memoryTypeIndex, VkDevice device, const void* pNext, VkMemoryRequirements* memoryRequirements)
{
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
    VkMemoryAllocateInfo memoryInfo = {};
    memoryInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryInfo.pNext           = pNext;
    memoryInfo.allocationSize  = memRequirements.size;
    memoryInfo.memoryTypeIndex = memoryTypeIndex;
    VkDeviceMemory deviceMemory;
    VK_CHECK_RESULT(vkAllocateMemory(device, &memoryInfo, nullptr, &deviceMemory));
    VK_CHECK_RESULT(vkBindBufferMemory(device, buffer, deviceMemory, 0));

    if (memoryRequirements) {
        *memoryRequirements = memRequirements;
    }
    return deviceMemory;
}

//void CreateDepthBuffer(SwapchainInfo &swapchainInfo, const DeviceInfo &deviceInfo, CommandInfo &commandInfo)
//{
//    VkFormat depthFormat = FindSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
//                                               VK_IMAGE_TILING_OPTIMAL,
//                                               VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
//                                               deviceInfo.physicalDevice);
//
//    CreateImage(swapchainInfo.extent.width, swapchainInfo.extent.height, 1, depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, swapchainInfo.depthImageInfo[i].image, swapchainInfo.depthImageInfo[i].memory, deviceInfo);
//    swapchainInfo.depthImageInfo[i].view = CreateImageView(swapchainInfo.depthImageInfo[i].image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1, deviceInfo.logicalDevices[0]);
//    TransitionImageLayout(swapchainInfo.depthImageInfo[i].image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, 1, commandInfo, deviceInfo);
//}