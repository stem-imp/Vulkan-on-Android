#ifndef MSAA_SCENE_RENDERER_H
#define MSAA_SCENE_RENDERER_H

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

class MSAASceneRenderer : public Renderer
{
public:
    MSAASceneRenderer(void* application, uint32_t screenWidth, uint32_t screenHeight);
    ~MSAASceneRenderer() override;

    void RenderImpl() override;

    void Update(float elapsedTime);

    typedef struct MVP {
        mat4 model;
        mat4 view;
        mat4 projection;
        mat4 mInvTranspose;
    } MVP;

    typedef struct Lighting {
        vec3 worldSpaceCameraPos;
        alignas(16) vec3 worldSpaceLightPos;
    } Lighting;

    vector<Model> models;
private:
    // ==== Vulkan ==== //
    void BuildMSAAImage(VkSampleCountFlagBits sampleCount);
    void BuildDepthImage(RenderPass* swapchainRenderPass, VkSampleCountFlagBits sampleCount);
    void BuildDescriptorSetLayout();
    void BuildDescriptorPool();
    void BuildDescriptorSet();
    void BuildGraphicsPipeline(void* application, VkSampleCountFlagBits sampleCount);
    void BuildCommandBuffer(int index);
    void RebuildSwapchain();

    Command::CommandBuffers _commandBuffers;

    void* _application;

    vector<ModelResource> _modelResources;
    vector<Texture2D>     _modelTextures;

    vector<Buffer>        _buffers;

    VkSampler             _diffuseSampler;

    VkImage               _msaaImage;
    VkImageView           _msaaView;
    VkDeviceMemory        _msaaImageMemory;
    VkImage               _depthImage;
    VkImageView           _depthView;
    VkDeviceMemory        _depthImageMemory;

    VkDescriptorSetLayout _descriptorSetLayout;
    VkDescriptorPool      _descriptorPool;
    VkDescriptorSet       _descriptorSet;

    VkPipelineLayout      _pipelineLayout;
    VkPipeline            _pipeline;
};

#endif