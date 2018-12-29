#ifndef EMPTY_SCENE_RENDERER_H
#define EMPTY_SCENE_RENDERER_H

#include "../../renderer.h"

using Vulkan::Command;

class EmptySceneRenderer : public Renderer
{
public:
    EmptySceneRenderer(void* genericWindow, uint32_t screenWidth, uint32_t screenHeight);
    ~EmptySceneRenderer() override;

    void RenderImpl() override;

    // ==== Vulkan ==== //
    //void RebuildSwapchainWithDependencies();
    void RebuildSwapchain();

private:
    void BuildCommandBuffer(int index);

    Command::CommandBuffers _commandBuffers;
};

#endif // EMPTY_SCENE_RENDERER_H