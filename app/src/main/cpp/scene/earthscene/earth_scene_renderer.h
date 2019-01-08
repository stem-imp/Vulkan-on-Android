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

    void UpdateMVP(float elapsedTime);

    vector<Model> models;
private:
    // ==== Vulkan ==== //
    void BuildDepthImage(RenderPass* swapchainRenderPass, VkSampleCountFlagBits sampleCount);
    void BuildDescriptorSetLayout();
    void BuildDescriptorPool();
    void BuildDescriptorSet();
    void BuildGraphicsPipeline(void* application, VkSampleCountFlagBits sampleCount);
    void BuildCommandBuffer(int index);
    void RebuildSwapchain();

    void* _application;

    vector<ModelResource> _modelResources;
    vector<Texture2D>     _modelTextures;
    VkSampler             _diffuseSampler;
    vector<Buffer>        _buffers;

    VkDescriptorSetLayout _descriptorSetLayout;
    VkDescriptorPool      _descriptorPool;
    VkDescriptorSet       _descriptorSet;

    VkPipelineLayout      _pipelineLayout;
    VkPipeline            _pipeline;

    Command::CommandBuffers _commandBuffers;
    VkImage                 _depthImage;
    VkImageView             _depthView;
    VkDeviceMemory          _depthImageMemory;
};

#endif // EARTH_SCENE_RENDERER_H