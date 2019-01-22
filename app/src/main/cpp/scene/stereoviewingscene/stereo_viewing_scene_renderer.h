#ifndef STEREO_VIEWING_SCENE_RENDERER_H
#define STEREO_VIEWING_SCENE_RENDERER_H

#include "../../renderer.h"
#include "../../vulkan/model/model.h"
#include "../../vulkan/model/model_resource.h"
#include "../../vulkan/texture/texture.h"
#include <vector>

using Vulkan::Command;
using Vulkan::VertexLayout;
using Vulkan::ModelResource;
using Vulkan::Texture;
using Vulkan::Texture2D;
using std::vector;

class StereoViewingSceneRenderer : public Renderer
{
public:
    StereoViewingSceneRenderer(void* application, uint32_t screenWidth, uint32_t screenHeight);
    ~StereoViewingSceneRenderer() override;

    void RenderImpl() override;

    void UpdateUniformBuffers(const vector<ModelTransform>& modelTransforms,
                              const vector<int>& modelTransformSizes,
                              const ViewProjectionTransform& lViewProj,
                              const ViewProjectionTransform& rViewProj,
                              int viewProjSize);

    void UploadModels(const vector<Vulkan::Model>& models);
    void BuildTextureSamplers();

    void BuildMSAAImage(VkSampleCountFlagBits sampleCount, int eye);
    void BuildMSAADepthImage(RenderPass* msaaRenderPass, VkSampleCountFlagBits sampleCount, int eye);
    void BuildMSAAResolvedImages(int eye);
    void BuildMSAAResolvedResultSampler();

    void BuildMSAADescriptorSetLayout();
    void BuildMultiviewDescriptorSetLayout();
    void BuildDescriptorPool();
    void BuildMSAADescriptorSet();
    void BuildMultiViewDescriptorSet(int eye);
    void BuildMSAAPipeline(void* application, const VertexLayout& vertexLayout, VkSampleCountFlagBits sampleCount);
    void BuildMultiviewPipeline(void* application, const VertexLayout& vertexLayout);

    void BuildCommandBuffers(int index);

    //========================temp
//    void BuildTempDescriptorSetLayout(void* app);
//    void BuildTempGraphicsPipeline();
    //========================temp

    VkSampleCountFlagBits SampleCount() { return _sampleCount; }

    void SetVertexLayouts(vector<VertexLayout>&& vertexLayouts) { _vertexLayouts = std::forward<vector<VertexLayout>>(vertexLayouts); }

    const vector<Vulkan::RenderPass*>& RenderPasses() const { return renderPasses; }

//    typedef struct MVP {
//        mat4 model;
//        mat4 view;
//        mat4 projection;
//        mat4 mInvTranspose;
//    } MVP;

    typedef struct Lighting {
        vec3 worldSpaceCameraPos;
        alignas(16) vec3 worldSpaceLightPos;
    } Lighting;

//    vector<Model> models;
private:
    // ==== Vulkan ==== //
//    void BuildMSAAImage(VkSampleCountFlagBits sampleCount);
//    void BuildMSAADepthImage(RenderPass* swapchainRenderPass, VkSampleCountFlagBits sampleCount);
//    void BuildDescriptorSetLayout();
//    void BuildDescriptorPool();
//    void BuildDescriptorSet();
//    void BuildGraphicsPipeline(void* application, VkSampleCountFlagBits sampleCount);
//    void BuildCommandBuffer(int index);
    void BuildSwapchainWithDependencies();
    void DeleteSwapchainWithDependencies();
    void RebuildSwapchain();

    void* _application;

    vector<ModelResource>           _modelResources;
    vector<Texture2D>               _modelTextures;
    vector<Texture::TextureAttribs> _textureAttribsGroup;
    vector<VkSampler>               _textureSamplers;

    vector<Buffer> _buffers;
    size_t         _dynamicBufferAlignment;
    size_t         _dynamicBufferSize;

    VkSampleCountFlagBits _sampleCount = VK_SAMPLE_COUNT_1_BIT;

    VkImage        _lMsaaImage        = VK_NULL_HANDLE, _rMsaaImage        = VK_NULL_HANDLE;
    VkImageView    _lMsaaView         = VK_NULL_HANDLE, _rMsaaView         = VK_NULL_HANDLE;
    VkDeviceMemory _lMsaaImageMemory  = VK_NULL_HANDLE, _rMsaaImageMemory  = VK_NULL_HANDLE;
    VkImage        _lDepthImage       = VK_NULL_HANDLE, _rDepthImage       = VK_NULL_HANDLE;
    VkImageView    _lDepthView        = VK_NULL_HANDLE, _rDepthView        = VK_NULL_HANDLE;
    VkDeviceMemory _lDepthImageMemory = VK_NULL_HANDLE, _rDepthImageMemory = VK_NULL_HANDLE;
    vector<VkImage>        _lMsaaResolvedImages  , _rMsaaResolvedImages;
    vector<VkDeviceMemory> _lMsaaResolvedMemories, _rMsaaResolvedMemories;
    vector<VkImageView>    _lMsaaResolvedViews   , _rMsaaResolvedViews;
    vector<Framebuffer>    _lMsaaFramebuffers    , _rMsaaFramebuffers;
    VkSampler _msaaResolvedResultSampler = VK_NULL_HANDLE;

    VkDescriptorSetLayout   _msaaDescriptorSetLayout      = VK_NULL_HANDLE;
    VkDescriptorSetLayout   _multiviewDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool        _descriptorPool               = VK_NULL_HANDLE;
    VkDescriptorSet         _msaaDescriptorSet            = VK_NULL_HANDLE;
    vector<VkDescriptorSet> _lDescriptorSets, _rDescriptorSets;

    vector<VertexLayout> _vertexLayouts;

    VkPipelineLayout _msaaPipelineLayout      = VK_NULL_HANDLE;
    VkPipelineLayout _multiviewPipelineLayout = VK_NULL_HANDLE;
    VkPipeline       _msaaPipeline            = VK_NULL_HANDLE;
    VkPipeline       _multiviewPipeline       = VK_NULL_HANDLE;

    Command::CommandBuffers _commandBuffers;
};

#endif // STEREO_VIEWING_SCENE_RENDERER_H