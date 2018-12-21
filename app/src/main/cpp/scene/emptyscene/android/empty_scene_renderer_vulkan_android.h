#include "../../../renderer.h"

using Vulkan::Command;

class EmptySceneRenderer : public Renderer
{
public:
    EmptySceneRenderer(void* genericWindow, uint32_t screenWidth, uint32_t screenHeight);
    ~EmptySceneRenderer() override;

    void RenderImpl() override;

    // ==== Vulkan ==== //
    void RebuildSwapchainWithDependencies();

private:
    void BuildCommandBuffers();

    Command::CommandBuffers _commandBuffers;
};