//#ifndef VULKAN_WORKFLOW_H
//#define VULKAN_WORKFLOW_H
//
//#ifdef __ANDROID__
//#include "vulkan_wrapper.h"
//#endif
//#include "vulkan_utility.h"
//
//void DestroySwapchainDependencies(vector<RenderPass*>& renderPasses, vector<Framebuffer>& framebuffers);
//
//template <typename T>
//void BuildSwapchainWithDependencies(Swapchain& swapchain, vector<RenderPass*>& renderPasses, vector<Framebuffer>& framebuffers, const Device& device)
//{
//    vkDeviceWaitIdle(device.LogicalDevice());
//
//    DestroySwapchainDependencies(renderPasses, framebuffers);
//
//    BuildSwapchain(swapchain);
//
//    RenderPass* swapchainRenderPass;
//    BuildRenderPass<T>(swapchainRenderPass, swapchain, device);
//    renderPasses.push_back(swapchainRenderPass);
//
//    CreateFramebuffers(framebuffers, swapchain, swapchainRenderPass, device);
//}
//
//#endif // VULKAN_WORKFLOW_H
