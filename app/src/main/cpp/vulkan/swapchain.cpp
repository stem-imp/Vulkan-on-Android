#include "swapchain.h"
#include "surface.h"
#include "device.h"
#include "vulkan_workflow.h"
#include "vulkan_utility.h"
#include <memory>

using Vulkan::Surface;
using Vulkan::Device;
using Vulkan::Swapchain;
using std::min;
using std::max;

Swapchain::~Swapchain()
{
    if (_swapchain != VK_NULL_HANDLE) {
        DebugLog("~Swapchain() vkDestroySwapchainKHR()");
        VkDevice device = _device.LogicalDevice();
        for (uint32_t i = 0; i < _swapChainImagesCount; i++) {
            vkDestroyImageView(device, _imageViews[i], nullptr);
        }
        _imageViews.clear();
        vkDestroySwapchainKHR(device, _swapchain, nullptr);
        _swapchain = VK_NULL_HANDLE;
        _images.clear();
    } else {
        DebugLog("~Swapchain()");
    }
}

void Swapchain::CreateSwapChain()
{
    const Surface& surface = _surface;
    const Device& device = _device;
    Vulkan::Surface::SurfaceSupportInfo supportInfo = surface.QuerySurfaceSupport(device.PhysicalDevice());
    _surfaceFormat = ChooseSwapSurfaceFormat(supportInfo.formats);
    VkPresentModeKHR presentMode = ChooseSwapchainPresentMode(supportInfo.presentModes, supportInfo.capabilities);
    assert(getScreenExtent);
    Extent2D viewportSize = getScreenExtent();
    if (orientationChanged) {
        SWAPCHAIN_EXTENT_PREPROCESS(supportInfo.capabilities.currentExtent);
    }
    VkExtent2D extent = ChooseSwapExtent(supportInfo.capabilities, viewportSize.width, viewportSize.height);
    _extent2D = { extent.width, extent.height };
    DebugLog("Adjusted Swapchain Size: (%d, %d)", extent.width, extent.height);

    _concurrentFramesCount = 0;
    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
        _concurrentFramesCount = 3;
    } else if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
        _concurrentFramesCount = 1;
    } else if (presentMode == VK_PRESENT_MODE_FIFO_KHR) {
        _concurrentFramesCount = 2;
    }
    uint32_t clamp = max(supportInfo.capabilities.minImageCount, min(_concurrentFramesCount, supportInfo.capabilities.maxImageCount));
    if (_concurrentFramesCount != clamp) {
        Log::Info("Concurrent frame count changes from %d to %d.", _concurrentFramesCount, clamp);
        _concurrentFramesCount = clamp;
        if (_concurrentFramesCount == 3) {
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        } else if (_concurrentFramesCount == 1) {
            presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        } else if (_concurrentFramesCount == 2) {
            presentMode = VK_PRESENT_MODE_FIFO_KHR;
        }
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext = nullptr;
    swapchainCreateInfo.flags = 0; // VkSwapchainCreateFlagBitsKHR
    swapchainCreateInfo.surface = surface.GetSurface();
    swapchainCreateInfo.minImageCount = _concurrentFramesCount;
    swapchainCreateInfo.imageFormat = _surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = _surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = extent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = 1;
    _graphics.index = device.FamilyQueues().graphics.index;
    _graphics.queue = device.FamilyQueues().graphics.queue;
    _present.index = device.FamilyQueues().present.index;
    _present.queue = device.FamilyQueues().present.queue;
    uint32_t queueFamilyIndices[] = { (uint32_t)_graphics.index, (uint32_t)_present.index };
    if (_graphics.index != _present.index) {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }

    swapchainCreateInfo.pQueueFamilyIndices = (uint32_t*)&_graphics.index;
    Log::Info("supportedTransforms: %d", supportInfo.capabilities.supportedTransforms);
    if (supportInfo.capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        swapchainCreateInfo.preTransform = supportInfo.capabilities.currentTransform;
    }
    // My testing device(Google Pixel XL) only supports VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR.
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for (auto& compositeAlphaFlag : compositeAlphaFlags) {
        if (supportInfo.capabilities.supportedCompositeAlpha & compositeAlphaFlag) {
            swapchainCreateInfo.compositeAlpha = compositeAlphaFlag;
            break;
        };
    }
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    VkSwapchainKHR oldSwapchain = _swapchain;
    _swapchain = VK_NULL_HANDLE;
    swapchainCreateInfo.oldSwapchain = oldSwapchain;
    VK_CHECK_RESULT(vkCreateSwapchainKHR(device.LogicalDevice(), &swapchainCreateInfo, nullptr, &_swapchain));

    if (oldSwapchain != VK_NULL_HANDLE) {
        for (uint32_t i = 0; i < _imageViews.size(); i++) {
            vkDestroyImageView(device.LogicalDevice(), _imageViews[i], nullptr);
            _imageViews[i] = VK_NULL_HANDLE;
        }
        vkDestroySwapchainKHR(device.LogicalDevice(), oldSwapchain, nullptr);
        oldSwapchain = VK_NULL_HANDLE;
    }
}

void Swapchain::CreateImageViews(uint32_t baseArrayLayer, uint32_t layerCount)
{
    const Device& device = _device;
    _swapChainImagesCount = 0;
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device.LogicalDevice(), _swapchain, &_swapChainImagesCount, nullptr));
    _images.resize(_swapChainImagesCount);
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device.LogicalDevice(), _swapchain, &_swapChainImagesCount, _images.data()));

    _imageViews.resize(_swapChainImagesCount);
    for (uint32_t i = 0; i < _swapChainImagesCount; i++) {
        _imageViews[i] = CreateImageView(_images[i],
                                         VK_IMAGE_VIEW_TYPE_2D,
                                         _surfaceFormat.format,
                                         VK_IMAGE_ASPECT_COLOR_BIT,
                                         0, // baseMipLevel
                                         1, // levelCount
                                         baseArrayLayer,
                                         layerCount,
                                         device.LogicalDevice());
    }
}