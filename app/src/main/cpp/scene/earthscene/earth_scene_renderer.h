#ifndef EARTH_SCENE_RENDERER_H
#define EARTH_SCENE_RENDERER_H

#include "../../renderer.h"
#include "../../vulkan/model.h"

using Vulkan::Command;
using Vulkan::Model;

class EarthSceneRenderer : public Renderer
{
public:
    EarthSceneRenderer(void* genericWindow, uint32_t screenWidth, uint32_t screenHeight);
    ~EarthSceneRenderer() override;

    void RenderImpl() override;

    // ==== Vulkan ==== //
    Model LoadModel(const string&               filename,
                      const Vulkan::VertexLayout& layout,

                      Vulkan::ModelCreateInfo*    modelInfo     = nullptr,
                      int                         readFileFlags = Model::DEFAULT_READ_FILE_FLAGS);
private:
    // ==== Vulkan ==== //
    void BuildCommandBuffer(int index);

    Command::CommandBuffers _commandBuffers;
    VkImage _depthImage;
    VkImageView _depthView;
    VkDeviceMemory _depthImageMemory;
};

#endif // EARTH_SCENE_RENDERER_H