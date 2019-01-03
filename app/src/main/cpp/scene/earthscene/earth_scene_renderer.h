#ifndef EARTH_SCENE_RENDERER_H
#define EARTH_SCENE_RENDERER_H

#include "../../renderer.h"
#include "../../vulkan/model/model.h"
#include "../../vulkan/model/model_resource.h"
#include "../../vulkan/texture/texture.h"
#include <vector>

using Vulkan::Command;
using Vulkan::Model;
using Vulkan::ModelResource;
using Vulkan::Texture;
using Vulkan::Texture2D;
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

    vector<ModelResource> _models;
    vector<Texture2D>     _modelTextures;

    Command::CommandBuffers _commandBuffers;
    VkImage                 _depthImage;
    VkImageView             _depthView;
    VkDeviceMemory          _depthImageMemory;
};

#endif // EARTH_SCENE_RENDERER_H