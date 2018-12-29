#ifndef RENDERER_H
#define RENDERER_H

// ==== Vulkan ==== //
#include "vulkan/instance.h"
#include "vulkan/surface.h"
#include "vulkan/device.h"
#include "vulkan/swapchain.h"
#include "vulkan/renderpass/renderpass.h"
#include "vulkan/framebuffer.h"
#include "vulkan/command.h"
#include <vector>
#include <stdexcept>

using std::runtime_error;

class Renderer
{
public:
    virtual ~Renderer() { DebugLog("~Renderer()"); };

    void Render()
    {
        RenderImpl();
    }

    void SetScreenSize(uint32_t width, uint32_t height) { screenSize.width = width, screenSize.height = height; }

    void ChangeOrientation()
    {
        swapchain->orientationChanged = orientationChanged = true;
    }
protected:
    Renderer(void* app, uint32_t screenWidth, uint32_t screenheight);

    virtual void RenderImpl() = 0;

#ifdef __ANDROID__
    ANativeWindow* window;
#endif
    Extent2D screenSize = { 0, 0 };

    // ==== Vulkan ==== //
    static void SysInitVulkan()
    {
        if (!InitVulkan()) {
            throw runtime_error("Vulkan is unavailable, install Vulkan and re-start.");
        }
    }

    LayerAndExtension*          layerAndExtension;
    Vulkan::Instance*           instance;
    Vulkan::Surface*            surface;
    Vulkan::Device*             device;
    Vulkan::Swapchain*          swapchain;
    vector<Vulkan::RenderPass*> renderPasses;
    vector<Vulkan::Framebuffer> framebuffers;
    Vulkan::Command*            command;

    uint32_t            currentFrameIndex;
    vector<VkFence>     multiFrameFences;
    vector<VkSemaphore> imageAvailableSemaphores;
    vector<VkSemaphore> commandsCompleteSemaphores;

    bool orientationChanged = false;
};

#endif // RENDERER_H
