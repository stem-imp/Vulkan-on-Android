#ifndef EARTH_SCENE_RENDERER_H
#define EARTH_SCENE_RENDERER_H

#include "../../renderer.h"
#include "../../vulkan/model.h"
#include <vector>

using Vulkan::Command;
using Vulkan::Model;
using std::vector;

class EarthSceneRenderer : public Renderer
{
public:
    EarthSceneRenderer(void* application, uint32_t screenWidth, uint32_t screenHeight);
    ~EarthSceneRenderer() override;

    void RenderImpl() override;
private:
    // ==== Vulkan ==== //
    void BuildCommandBuffer(int index);

    vector<Model> _models;

    Command::CommandBuffers _commandBuffers;
    VkImage                 _depthImage;
    VkImageView             _depthView;
    VkDeviceMemory          _depthImageMemory;
};

#endif // EARTH_SCENE_RENDERER_H