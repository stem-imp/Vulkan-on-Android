#ifdef __ANDROID__

#include "empty_scene_renderer_vulkan_android.h"
#include "../../../vulkan/renderpass/color_dst_renderpass.h"
//#include "../../../vulkan/texture.h"
#include "../../../vulkan/vulkan_utility.h"
#include "../../../vulkan/vulkan_workflow.h"

using Vulkan::Instance;
using Vulkan::Surface;
using Vulkan::Device;
using Vulkan::Swapchain;
using Vulkan::RenderPass;
using Vulkan::ColorDestinationRenderPass;
using Vulkan::Command;

EmptySceneRenderer::EmptySceneRenderer(void* genericWindow, uint32_t screenWidth, uint32_t screenheight) : Renderer(genericWindow, screenWidth, screenheight)
{
    SysInitVulkan();
    instance = new Instance();
    layerAndExtension = new LayerAndExtension();
    if (!BuildInstance(*instance, *layerAndExtension)) {
        throw runtime_error("Essential layers and extension are not available.");
    }

    surface = new Surface(window, instance->GetInstance());

    device    = new Device(SelectPhysicalDevice(*instance, *surface));
    const vector<const char*> requestedExtNames = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    if (!device->EnableDeviceExtensions(requestedExtNames)) {
        throw runtime_error("Requested device extensions are not supported.");
    }
    VkPhysicalDeviceFeatures featuresRequested = {};
    device->CreateDevice(featuresRequested, requestedExtNames);
    device->GetFamilyQueues();

    swapchain = new Swapchain(*surface, *device);
    swapchain->getScreenExtent = [&]() -> Extent2D { return screenSize; };
    BuildSwapchain(*swapchain);

    RenderPass* swapchainRenderPass;
    BuildRenderPass<ColorDestinationRenderPass>(swapchainRenderPass, *swapchain, *device);
    renderPasses.push_back(swapchainRenderPass);

    CreateFramebuffers(framebuffers, *swapchain, swapchainRenderPass, *device);

    command = new Command();
    command->BuildCommandPools(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, *device);

    const VkDevice d = device->LogicalDevice();

    size_t size = swapchain->ImageViews().size();
    _commandBuffers.buffers = Command::CreateCommandBuffers(command->GeneralGraphcisPool(),
                                                            VK_COMMAND_BUFFER_LEVEL_PRIMARY, size,
                                                            d);

    size_t concurrentFramesCount = swapchain->ConcurrentFramesCount();
    multiFrameFences.resize(concurrentFramesCount);
    imageAvailableSemaphores.resize(concurrentFramesCount);
    for (int i = 0; i < concurrentFramesCount; i++) {
        multiFrameFences[i]          = CreateFence(d, 0);
        imageAvailableSemaphores[i]  = CreateSemaphore(d);
    }

//    Vulkan::Texture::defaultTransitionImageLayout = [=](VkPipelineStageFlags sourceStage,
//                                                        VkPipelineStageFlags destinationStage,
//                                                        VkAccessFlags        srcAccessMask,
//                                                        VkAccessFlags        dstAccessMask,
//                                                        VkImageLayout        oldLayout,
//                                                        VkImageLayout        newLayout,
//                                                        VkImage              image,
//                                                        VkImageAspectFlags   aspectMask)
//    {
//        VkCommandPool pool = command->ShortLivedTransferPool();
//        VkCommandBuffer buffer = Command::CreateAndBeginCommandBuffers(pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1, 0, d);
//        TransitionImageLayout(sourceStage, destinationStage, srcAccessMask, dstAccessMask, oldLayout, newLayout, image, aspectMask, buffer);
//        Command::EndAndSubmitCommandBuffer(buffer, device->FamilyQueues().transfer.queue, pool, d);
//    };
}

EmptySceneRenderer::~EmptySceneRenderer()
{
    DebugLog("~EmptySceneRenderer()");

    VkDevice d = device->LogicalDevice();

    vkDeviceWaitIdle(d);

    for (int i = 0; i < swapchain->ConcurrentFramesCount(); i++) {
        vkDestroyFence(d, multiFrameFences[i], nullptr);
        vkDestroySemaphore(d, imageAvailableSemaphores[i], nullptr);
    }

    delete command, command = nullptr;

    framebuffers.clear();

    for (auto& r : renderPasses) {
        delete r;
        r = nullptr;
    }
    renderPasses.clear();

    delete swapchain        , swapchain         = nullptr;
    delete device           , device            = nullptr;
    delete surface          , surface           = nullptr;
    delete layerAndExtension, layerAndExtension = nullptr;
    delete instance         , instance          = nullptr;
}

void EmptySceneRenderer::BuildCommandBuffers()
{
    RenderPass* swapchainRenderPass = renderPasses[0];
    const VkDevice d = device->LogicalDevice();
    size_t size = swapchain->ImageViews().size();
    for (int i = 0; i < size; i++) {
        Command::BeginCommandBuffer(_commandBuffers.buffers[i], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, d);
        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass        = swapchainRenderPass->GetRenderPass();
        renderPassBeginInfo.framebuffer       = framebuffers[i].GetFramebuffer();
        renderPassBeginInfo.renderArea.offset = { 0, 0 };
        renderPassBeginInfo.renderArea.extent = swapchain->Extent();
        renderPassBeginInfo.clearValueCount   = 1;
        VkClearValue clearValue;
        clearValue.color = { 0.125f, 0.75f, 1.0f, 1.0f };
        renderPassBeginInfo.pClearValues      = &clearValue;
        vkCmdBeginRenderPass(_commandBuffers.buffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdEndRenderPass(_commandBuffers.buffers[i]);
        VK_CHECK_RESULT(vkEndCommandBuffer(_commandBuffers.buffers[i]));
    }
}

void EmptySceneRenderer::RenderImpl()
{
    if (!device->SharedGraphicsAndPresentQueueFamily()) {
        return;
    }
    if (orientationChanged) {
        DebugLog("Orientation changed.");
        orientationChanged = false;
        RebuildSwapchainWithDependencies();
        return;
    }

    VkDevice d = device->LogicalDevice();

    BuildCommandBuffers();

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(d, swapchain->GetSwapchain(), UINT64_MAX, imageAvailableSemaphores[currentFrameIndex], VK_NULL_HANDLE, &imageIndex);
    if (result != VK_SUCCESS) {
        DebugLog("vkAcquireNextImageKHR: %d", result);
        RebuildSwapchainWithDependencies();
        return;
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphores[currentFrameIndex];
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_commandBuffers.buffers[imageIndex];
    VK_CHECK_RESULT(vkQueueSubmit(device->FamilyQueues().graphics.queue, 1, &submitInfo, multiFrameFences[currentFrameIndex]));

    vkWaitForFences(d, 1, &multiFrameFences[currentFrameIndex], true, UINT64_MAX);
    vkResetFences(d, 1, &multiFrameFences[currentFrameIndex]);
    vkResetCommandBuffer(_commandBuffers.buffers[imageIndex], 0);

    result = QueuePresent(&swapchain->GetSwapchain(), &imageIndex, *device, VK_NULL_HANDLE, 0);
    if (result != VK_SUCCESS) {
        DebugLog("vkQueuePresentKHR: %d", result);
        RebuildSwapchainWithDependencies();
        return;
    }

    currentFrameIndex = (currentFrameIndex + 1) % swapchain->ConcurrentFramesCount();
}

void EmptySceneRenderer::RebuildSwapchainWithDependencies()
{
    const VkDevice d = device->LogicalDevice();

    vkDeviceWaitIdle(d);

    // Destroy
    framebuffers.clear();
    for (auto& r : renderPasses) {
        delete r;
    }

    renderPasses.clear();


    // Create
    BuildSwapchain(*swapchain);

    RenderPass* swapchainRenderPass;
    BuildRenderPass<ColorDestinationRenderPass>(swapchainRenderPass, *swapchain, *device);
    renderPasses.push_back(swapchainRenderPass);

    CreateFramebuffers(framebuffers, *swapchain, swapchainRenderPass, *device);
}

#endif