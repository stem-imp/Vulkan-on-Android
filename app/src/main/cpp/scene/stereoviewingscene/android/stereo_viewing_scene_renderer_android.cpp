#ifdef __ANDROID__

#include "../stereo_viewing_scene_renderer.h"
#include "../../../vulkan/renderpass/msaa_wo_presentation_prenderpass.h"
#include "../../../vulkan/renderpass/color_dst_renderpass.h"
#include "../../../androidutility/assetmanager/io_asset.hpp"
#include "../../../vulkan/model/model.h"
#include "../../../vulkan/vulkan_utility.h"

#include <unordered_map>

using Vulkan::Instance;
using Vulkan::Surface;
using Vulkan::Device;
using Vulkan::Swapchain;
using Vulkan::RenderPass;
using Vulkan::MSAAWOPresentationRenderPass;
using Vulkan::ColorDestinationRenderPass;
using Vulkan::VertexComponent;
using Vulkan::VertexLayout;
using Vulkan::Model;
using std::unordered_map;
using std::max;

static unordered_map<int, int> currentFrameToImageindex;
static unordered_map<int, int> imageIndexToCurrentFrame;

StereoViewingSceneRenderer::StereoViewingSceneRenderer(void* application, uint32_t screenWidth, uint32_t screenHeight) : Renderer(application, screenWidth, screenHeight)
{
    SysInitVulkan();
    instance = new Instance();
    layerAndExtension = new LayerAndExtension();
    if (!BuildInstance(*instance, *layerAndExtension)) {
        throw runtime_error("Essential layers and extension are not available.");
    }

    surface = new Surface(window, instance->GetInstance());

    const vector<const char*> requestedExtNames = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkPhysicalDeviceFeatures requestedFeatures = {};
    requestedFeatures.samplerAnisotropy = VK_TRUE;
    requestedFeatures.sampleRateShading = VK_TRUE;
    device = new Device(SelectPhysicalDevice(*instance, *surface, requestedExtNames, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT, requestedFeatures));
    VkPhysicalDeviceFeatures featuresRequested = { .samplerAnisotropy = VK_TRUE, .sampleRateShading = VK_TRUE };
    device->BuildDevice(featuresRequested, requestedExtNames);
    _sampleCount = VK_SAMPLE_COUNT_4_BIT;
    device->RequestSampleCount(_sampleCount);

    BuildSwapchainWithDependencies();
    uint32_t size = swapchain->ImageViews().size();

    command = new Command();
    command->BuildCommandPools(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, *device);
    VkDevice d = device->LogicalDevice();
    _commandBuffers.buffers = Command::CreateCommandBuffers(command->ShortLivedGraphcisPool(),
                                                            VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                            size,
                                                            d);

    size_t concurrentFramesCount = swapchain->ConcurrentFramesCount();
    multiFrameFences.resize(concurrentFramesCount);
    imageAvailableSemaphores.resize(concurrentFramesCount);
    commandsCompleteSemaphores.resize(concurrentFramesCount);
    for (int i = 0; i < concurrentFramesCount; i++) {
        multiFrameFences[i]           = CreateFence(d);
        imageAvailableSemaphores[i]   = CreateSemaphore(d);
        commandsCompleteSemaphores[i] = CreateSemaphore(d);
    }


    // Uniform Buffers: Model and Dynamic View and Projection Transform
    VkDeviceSize modelTransformSize = sizeof(ModelTransform);
    _buffers.emplace_back(*device);
    Buffer& modelTransform = _buffers[0];
    modelTransform.BuildDefaultBuffer(modelTransformSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    modelTransform.Map();

    VkDeviceSize minUboAlignment = device->PhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
    _dynamicBufferAlignment = sizeof(ViewProjectionTransform);
    if (minUboAlignment > 0) {
        _dynamicBufferAlignment = (_dynamicBufferAlignment + minUboAlignment - 1) / minUboAlignment * minUboAlignment;
    }
    _dynamicBufferSize = _dynamicBufferAlignment * 2;
    //posix_memalign((void**)&dynamicVP, bufferSize, dynamicAlignment);
    DebugLog("minUniformBufferOffsetAlignment: %d\n", minUboAlignment);
    DebugLog("dynamic vp buffer alignment: %d\n", _dynamicBufferAlignment);
    _buffers.emplace_back(*device);
    Buffer& vpTransform = _buffers[1];
    VkMemoryRequirements memoryRequirements;
    vpTransform.BuildDefaultBuffer(_dynamicBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &memoryRequirements);
    vpTransform.Map(memoryRequirements.size);
    DebugLog("dynamic vp buffer memory size: %d\n", memoryRequirements.size);


    _application = application;
}

StereoViewingSceneRenderer::~StereoViewingSceneRenderer()
{
    DebugLog("~StereoViewingSceneRenderer()");

    uint32_t concurrentFramesCount = swapchain->ConcurrentFramesCount();
    DeleteSwapchainWithDependencies();

    ////////////////////////////////////////////////////////////////////
    VkDevice d = device->LogicalDevice();

    vkDestroySampler(d, _msaaResolvedResultSampler, nullptr), _msaaResolvedResultSampler = VK_NULL_HANDLE;

    for (auto& ts : _textureSamplers) {
        vkDestroySampler(d, ts, nullptr), ts = VK_NULL_HANDLE;
    }
    _textureSamplers.clear();
    _buffers.clear();
    _modelTextures.clear();
    _modelResources.clear();
    _textureAttribsGroup.clear();

    for (int i = 0; i < concurrentFramesCount; i++) {
        vkDestroyFence(d, multiFrameFences[i], nullptr), multiFrameFences[i] = VK_NULL_HANDLE;
        vkDestroySemaphore(d, imageAvailableSemaphores[i], nullptr), imageAvailableSemaphores[i] = VK_NULL_HANDLE;
        vkDestroySemaphore(d, commandsCompleteSemaphores[i], nullptr), commandsCompleteSemaphores[i] = VK_NULL_HANDLE;
    }

    delete command, command = nullptr;

    vkDestroyDescriptorSetLayout(d, _msaaDescriptorSetLayout, nullptr), _msaaDescriptorSetLayout = VK_NULL_HANDLE;
    vkDestroyDescriptorSetLayout(d, _multiviewDescriptorSetLayout, nullptr), _multiviewDescriptorSetLayout = VK_NULL_HANDLE;
    vkDestroyDescriptorPool(d, _descriptorPool, nullptr), _descriptorPool = VK_NULL_HANDLE;
    vkDestroyPipelineLayout(d, _msaaPipelineLayout, nullptr), _msaaPipelineLayout = VK_NULL_HANDLE;
    vkDestroyPipelineLayout(d, _multiviewPipelineLayout, nullptr), _multiviewPipelineLayout = VK_NULL_HANDLE;

    delete device           , device            = nullptr;
    delete surface          , surface           = nullptr;
    delete layerAndExtension, layerAndExtension = nullptr;
    delete instance         , instance          = nullptr;
}

void StereoViewingSceneRenderer::BuildSwapchainWithDependencies()
{
    swapchain = new Swapchain(*surface, *device);
    swapchain->getScreenExtent = [&]() -> Extent2D { return screenSize; };
    BuildSwapchain(*swapchain);

    RenderPass* msaaRenderPass;
    msaaRenderPass = new MSAAWOPresentationRenderPass(*device);
    msaaRenderPass->getFormat = [this]() -> VkFormat { return swapchain->Format(); };
    msaaRenderPass->getSampleCount = [this]() -> VkSampleCountFlagBits { return _sampleCount; };
    msaaRenderPass->CreateRenderPass();
    renderPasses.push_back(msaaRenderPass);
    RenderPass* swapchainRenderPass;
    swapchainRenderPass = new ColorDestinationRenderPass(*device);
    swapchainRenderPass->getFormat = [this]() -> VkFormat { return swapchain->Format(); };
    swapchainRenderPass->getSampleCount = [this]() -> VkSampleCountFlagBits { return _sampleCount; };
    swapchainRenderPass->CreateRenderPass();
    renderPasses.push_back(swapchainRenderPass);

    uint32_t size = swapchain->ImageViews().size();
    for (int i = 0 ; i < size; i++) {
        framebuffers.push_back(*device);
    }

    // BuildMSAAImage
    // BuildMSAADepthImage
    _lMsaaResolvedImages.resize(size, VK_NULL_HANDLE);
    _lMsaaResolvedMemories.resize(size, VK_NULL_HANDLE);
    _lMsaaResolvedViews.resize(size, VK_NULL_HANDLE);
    _rMsaaResolvedImages.resize(size, VK_NULL_HANDLE);
    _rMsaaResolvedMemories.resize(size, VK_NULL_HANDLE);
    _rMsaaResolvedViews.resize(size, VK_NULL_HANDLE);
    // BuildMSAAResolvedImages
    for (int i = 0; i < size; i++) {
        _lMsaaFramebuffers.push_back(*device);
        _rMsaaFramebuffers.push_back(*device);
    }

    // BuildMSAAResolvedResultSampler

    _lDescriptorSets.resize(size, VK_NULL_HANDLE);
    _rDescriptorSets.resize(size, VK_NULL_HANDLE);
    // BuildMSAADescriptorSetLayout
    // BuildMultiviewDescriptorSetLayout
    // BuildDescriptorPool
    // BuildMSAADescriptorSet
    // BuildMultiViewDescriptorSet
    // BuildMSAAPipeline
    // BuildMultiviewPipeline
}

void StereoViewingSceneRenderer::DeleteSwapchainWithDependencies()
{
    VkDevice d = device->LogicalDevice();

    vkDeviceWaitIdle(d);

    vkDestroyImageView(d, _lMsaaView, nullptr)        , _lMsaaView         = VK_NULL_HANDLE;
    vkDestroyImage(    d, _lMsaaImage, nullptr)       , _lMsaaImage        = VK_NULL_HANDLE;
    vkFreeMemory(      d, _lMsaaImageMemory, nullptr) , _lMsaaImageMemory  = VK_NULL_HANDLE;
    vkDestroyImageView(d, _lDepthView, nullptr)       , _lDepthView        = VK_NULL_HANDLE;
    vkDestroyImage(    d, _lDepthImage, nullptr)      , _lDepthImage       = VK_NULL_HANDLE;
    vkFreeMemory(      d, _lDepthImageMemory, nullptr), _lDepthImageMemory = VK_NULL_HANDLE;
    vkDestroyImageView(d, _rMsaaView, nullptr)        , _rMsaaView         = VK_NULL_HANDLE;
    vkDestroyImage(    d, _rMsaaImage, nullptr)       , _rMsaaImage        = VK_NULL_HANDLE;
    vkFreeMemory(      d, _rMsaaImageMemory, nullptr) , _rMsaaImageMemory  = VK_NULL_HANDLE;
    vkDestroyImageView(d, _rDepthView, nullptr)       , _rDepthView        = VK_NULL_HANDLE;
    vkDestroyImage(    d, _rDepthImage, nullptr)      , _rDepthImage       = VK_NULL_HANDLE;
    vkFreeMemory(      d, _rDepthImageMemory, nullptr), _rDepthImageMemory = VK_NULL_HANDLE;

    for (auto& view : _lMsaaResolvedViews) {
        vkDestroyImageView(d, view, nullptr), view = VK_NULL_HANDLE;
    }
    _lMsaaResolvedViews.clear();
    for (auto& image : _lMsaaResolvedImages) {
        vkDestroyImage(d, image, nullptr), image = VK_NULL_HANDLE;
    }
    _lMsaaResolvedImages.clear();
    for (auto& mem : _lMsaaResolvedMemories) {
        vkFreeMemory(d, mem, nullptr), mem = VK_NULL_HANDLE;
    }
    _lMsaaResolvedMemories.clear();

    for (auto& view : _rMsaaResolvedViews) {
        vkDestroyImageView(d, view, nullptr), view = VK_NULL_HANDLE;
    }
    _rMsaaResolvedViews.clear();
    for (auto& image : _rMsaaResolvedImages) {
        vkDestroyImage(d, image, nullptr), image = VK_NULL_HANDLE;
    }
    _rMsaaResolvedImages.clear();
    for (auto& mem : _rMsaaResolvedMemories) {
        vkFreeMemory(d, mem, nullptr), mem = VK_NULL_HANDLE;
    }
    _rMsaaResolvedMemories.clear();

    _lMsaaFramebuffers.clear();
    _rMsaaFramebuffers.clear();

    vkFreeDescriptorSets(d, _descriptorPool, 1, &_msaaDescriptorSet);
    for (auto& ds : _lDescriptorSets) {
        vkFreeDescriptorSets(d, _descriptorPool, 1, &ds);
    }
    _lDescriptorSets.clear();
    for (auto& ds : _rDescriptorSets) {
        vkFreeDescriptorSets(d, _descriptorPool, 1, &ds);
    }
    _rDescriptorSets.clear();

    vkDestroyPipeline(d, _msaaPipeline, nullptr), _msaaPipeline = VK_NULL_HANDLE;
    vkDestroyPipeline(d, _multiviewPipeline, nullptr), _multiviewPipeline = VK_NULL_HANDLE;

    framebuffers.clear();

    for (auto& r : renderPasses) {
        delete r, r = nullptr;
    }
    renderPasses.clear();

    delete swapchain, swapchain = nullptr;
}

void StereoViewingSceneRenderer::RenderImpl()
{
    if (!device->SharedGraphicsAndPresentQueueFamily()) {
        return;
    }

    if (orientationChanged) {
        DebugLog("Orientation changed.");
        orientationChanged = false;
        //RebuildSwapchain();
        return;
    }

    VkDevice d = device->LogicalDevice();

    vkWaitForFences(d, 1, &multiFrameFences[currentFrameIndex], true, UINT64_MAX);
    vkResetFences(d, 1, &multiFrameFences[currentFrameIndex]);
    if (currentFrameToImageindex.count(currentFrameIndex) > 0) {
        _lMsaaFramebuffers[currentFrameToImageindex[currentFrameIndex]].ReleaseFramebuffer();
        _rMsaaFramebuffers[currentFrameToImageindex[currentFrameIndex]].ReleaseFramebuffer();
        framebuffers[currentFrameToImageindex[currentFrameIndex]].ReleaseFramebuffer();
        currentFrameToImageindex.erase(currentFrameIndex);
    }

    uint32_t imageIndex;
    vkAcquireNextImageKHR(d, swapchain->GetSwapchain(), UINT64_MAX, imageAvailableSemaphores[currentFrameIndex], VK_NULL_HANDLE, &imageIndex);
    if (imageIndexToCurrentFrame.count(imageIndex) > 0) {
        vkWaitForFences(d, 1, &multiFrameFences[imageIndexToCurrentFrame[imageIndex]], true, UINT64_MAX);
        vkResetFences(d, 1, &multiFrameFences[imageIndexToCurrentFrame[imageIndex]]);
        _lMsaaFramebuffers[imageIndex].ReleaseFramebuffer();
        _rMsaaFramebuffers[imageIndex].ReleaseFramebuffer();
        framebuffers[imageIndex].ReleaseFramebuffer();
        currentFrameToImageindex.erase(imageIndexToCurrentFrame[imageIndex]);
        imageIndexToCurrentFrame.erase(imageIndex);
    }

    vector<VkImageView> attachments = { _lMsaaView, _lDepthView, _lMsaaResolvedViews[imageIndex] };
    const VkExtent2D& e = swapchain->Extent();
    _lMsaaFramebuffers[imageIndex].CreateSwapchainFramebuffer(renderPasses[0]->GetRenderPass(), attachments, e);
    attachments = { _rMsaaView, _rDepthView, _rMsaaResolvedViews[imageIndex] };
    _rMsaaFramebuffers[imageIndex].CreateSwapchainFramebuffer(renderPasses[0]->GetRenderPass(), attachments, e);
    attachments = { swapchain->ImageViews()[imageIndex] };
    framebuffers[imageIndex].CreateSwapchainFramebuffer(renderPasses[1]->GetRenderPass(), attachments, e);
    currentFrameToImageindex[currentFrameIndex] = imageIndex;
    imageIndexToCurrentFrame[imageIndex] = currentFrameIndex;

    BuildCommandBuffers(imageIndex);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphores[currentFrameIndex];
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_commandBuffers.buffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &commandsCompleteSemaphores[currentFrameIndex];
    VK_CHECK_RESULT(vkQueueSubmit(device->FamilyQueues().graphics.queue, 1, &submitInfo, multiFrameFences[currentFrameIndex]));

    QueuePresent(&swapchain->GetSwapchain(), &imageIndex, *device, 1, &commandsCompleteSemaphores[currentFrameIndex]);

    currentFrameIndex = (currentFrameIndex + 1) % swapchain->ConcurrentFramesCount();
}

void StereoViewingSceneRenderer::UpdateUniformBuffers(const vector<ModelTransform>& modelTransforms,
                                                      const vector<int>& modelTransformSizes,
                                                      const ViewProjectionTransform& lViewProj,
                                                      const ViewProjectionTransform& rViewProj,
                                                      int viewProjSize)
{
    Buffer& modelTransform = _buffers[0];
    std::copy((uint8_t*)&modelTransforms[0], (uint8_t*)&modelTransforms[0] + modelTransformSizes[0], (uint8_t*)modelTransform.mapped);

    Buffer& vpTransform = _buffers[1];
    ViewProjectionTransform* base = (ViewProjectionTransform*)vpTransform.mapped;
    std::copy((uint8_t*)&lViewProj, (uint8_t*)&lViewProj + viewProjSize, (uint8_t*)base);

    base++;

    std::copy((uint8_t*)&rViewProj, (uint8_t*)&rViewProj + viewProjSize, (uint8_t*)base);
}

void StereoViewingSceneRenderer::UploadModels(const vector<Model>& models)
{
    android_app* app = (android_app*)_application;
    string filePath = string(app->activity->externalDataPath) + string("/cuberb1k/");
    Texture::TextureAttribs textureAttribs;
    for (const auto& m : models) {
        for (const auto& n : m.Materials()) {
            for (const auto& it: n.textures) {
                aiTextureType type = (aiTextureType)it.first;
                DebugLog("Texture Type: %d", type);
                for (const auto& str : it.second) {
                    uint8_t* imageData = stbi_load((string(filePath) + str).c_str(),
                                                   (int*)&textureAttribs.width,
                                                   (int*)&textureAttribs.height,
                                                   (int*)&textureAttribs.channelsPerPixel,
                                                   STBI_rgb_alpha);
                    textureAttribs.sRGB = true;
                    textureAttribs.mipmapLevels = (uint32_t)(floor(log2(max(textureAttribs.width, textureAttribs.height)))) + 1;
                    _textureAttribsGroup.emplace_back(textureAttribs);
                    _modelTextures.emplace_back(*device);
                    _modelTextures[_modelTextures.size() - 1].BuildTexture2D(textureAttribs, imageData, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *command);
                    _textureSamplers.push_back(VK_NULL_HANDLE);
                }
            }
        }
        _modelResources.emplace_back(*device);
        _modelResources[_modelResources.size() - 1].UploadToGPU(m, *command);
    }
}

void StereoViewingSceneRenderer::BuildTextureSamplers()
{
    float samplerAnisotropy = device->FeaturesEnabled().samplerAnisotropy ? 8 : 1;
    device->RequsetSamplerAnisotropy(samplerAnisotropy);
    VkDevice d = device->LogicalDevice();
    for (int i = 0; i < _textureAttribsGroup.size(); i++) {
        VkSamplerCreateInfo samplerInfo = SamplerCreateInfo(_textureAttribsGroup[i].samplerMipmapMode,
                                                            _textureAttribsGroup[i].mipmapLevels,
                                                            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                                            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                                            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                                            samplerAnisotropy);
        VK_CHECK_RESULT(vkCreateSampler(d, &samplerInfo, nullptr, &_textureSamplers[i]));
    }
}

void StereoViewingSceneRenderer::BuildMSAAImage(VkSampleCountFlagBits sampleCount, int eye)
{
    VkFormat colorFormat = swapchain->Format();
    VkExtent2D extent = swapchain->Extent();
    VkImageCreateInfo imageInfo = ImageCreateInfo(colorFormat,
                                                  { extent.width, extent.height, 1},
                                                  1,
                                                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
                                                  VK_IMAGE_TILING_OPTIMAL,
                                                  VK_IMAGE_LAYOUT_UNDEFINED,
                                                  sampleCount);
    VkDevice d = device->LogicalDevice();
    bool leftEye = (eye == 0);
    VkImage& image = leftEye ? _lMsaaImage : _rMsaaImage;
    VkDeviceMemory& memory = leftEye ? _lMsaaImageMemory : _rMsaaImageMemory;
    VkImageView& imageView = leftEye ? _lMsaaView : _rMsaaView;
    VK_CHECK_RESULT(vkCreateImage(d, &imageInfo, nullptr, &image));

    VkMemoryRequirements memoryRequirements;
    uint32_t memoryTypeIndex;
    VkMemoryAllocateInfo memoryAllocateInfo = MemoryAllocateInfo(image,
                                                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                                 *device, memoryRequirements,
                                                                 memoryTypeIndex);
    VK_CHECK_RESULT(vkAllocateMemory(d, &memoryAllocateInfo, nullptr, &memory));
    VK_CHECK_RESULT(vkBindImageMemory(d, image, memory, 0));

    VkImageViewCreateInfo imageViewInfo = ImageViewCreateInfo(image, colorFormat, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
    VK_CHECK_RESULT(vkCreateImageView(d, &imageViewInfo, nullptr, &imageView));
}

void StereoViewingSceneRenderer::BuildMSAADepthImage(RenderPass *msaaRenderPass, VkSampleCountFlagBits sampleCount, int eye)
{
    VkFormat depthFormat = ((MSAAWOPresentationRenderPass*)msaaRenderPass)->DepthFormat();
    VkImageAspectFlags depthImageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (depthFormat == VK_FORMAT_D16_UNORM_S8_UINT || depthFormat == VK_FORMAT_D24_UNORM_S8_UINT || depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT) {
        depthImageAspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    VkExtent2D extent2D = swapchain->Extent();
    VkImageCreateInfo imageInfo = ImageCreateInfo(depthFormat,
                                                  { extent2D.width, extent2D.height, 1 },
                                                  1,
                                                  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                                  VK_IMAGE_TILING_OPTIMAL,
                                                  VK_IMAGE_LAYOUT_UNDEFINED,
                                                  sampleCount);
    VkDevice d = device->LogicalDevice();
    bool leftEye = (eye == 0);
    VkImage& image = leftEye ? _lDepthImage : _rDepthImage;
    VkDeviceMemory& memory = leftEye ? _lDepthImageMemory : _rDepthImageMemory;
    VkImageView& view = leftEye ? _lDepthView : _rDepthView;
    VK_CHECK_RESULT(vkCreateImage(d, &imageInfo, nullptr, &image));

    VkMemoryRequirements memoryRequirements;
    uint32_t memoryTypeIndex;
    VkMemoryAllocateInfo memoryAllocateInfo = MemoryAllocateInfo(image,
                                                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                                 *device, memoryRequirements,
                                                                 memoryTypeIndex);
    VK_CHECK_RESULT(vkAllocateMemory(d, &memoryAllocateInfo, nullptr, &memory));
    VK_CHECK_RESULT(vkBindImageMemory(d, image, memory, 0));

    VkImageViewCreateInfo imageViewInfo = ImageViewCreateInfo(image, depthFormat, { depthImageAspectFlags, 0, 1, 0, 1 });
    VK_CHECK_RESULT(vkCreateImageView(d, &imageViewInfo, nullptr, &view));
}

void StereoViewingSceneRenderer::BuildMSAAResolvedImages(int eye)
{
    VkFormat colorFormat = swapchain->Format();
    VkExtent2D extent = swapchain->Extent();
    VkImageCreateInfo imageInfo = ImageCreateInfo(colorFormat,
                                                  { extent.width, extent.height, 1},
                                                  1,
                                                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    bool leftEye = (eye == 0);
    vector<VkImage>& resolvedImages = leftEye ? _lMsaaResolvedImages : _rMsaaResolvedImages;
    vector<VkDeviceMemory>& memories = leftEye ? _lMsaaResolvedMemories : _rMsaaResolvedMemories;
    vector<VkImageView>& views = leftEye ? _lMsaaResolvedViews : _rMsaaResolvedViews;

    VkDevice d = device->LogicalDevice();
    VkMemoryRequirements memoryRequirements;
    uint32_t memoryTypeIndex;
    int size = swapchain->ImageViews().size();
    for (int i = 0; i < size; i++) {
        VK_CHECK_RESULT(vkCreateImage(d, &imageInfo, nullptr, &resolvedImages[i]));

        VkMemoryAllocateInfo memoryAllocateInfo = MemoryAllocateInfo(resolvedImages[i],
                                                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                                     *device,
                                                                     memoryRequirements,
                                                                     memoryTypeIndex);
        VK_CHECK_RESULT(vkAllocateMemory(d, &memoryAllocateInfo, nullptr, &memories[i]));
        VK_CHECK_RESULT(vkBindImageMemory(d, resolvedImages[i], memories[i], 0));

        VkImageViewCreateInfo imageViewInfo = ImageViewCreateInfo(resolvedImages[i], colorFormat,
                                                                  {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1,
                                                                   0, 1});
        VK_CHECK_RESULT(vkCreateImageView(d, &imageViewInfo, nullptr, &views[i]));
    }
}

void StereoViewingSceneRenderer::BuildMSAAResolvedResultSampler()
{
    VkSamplerCreateInfo samplerInfo = SamplerCreateInfo(VK_SAMPLER_MIPMAP_MODE_NEAREST,
                                                        1,
                                                        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                                        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                                        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                                        1);
    VK_CHECK_RESULT(vkCreateSampler(device->LogicalDevice(), &samplerInfo, nullptr, &_msaaResolvedResultSampler));
}

void StereoViewingSceneRenderer::BuildMSAADescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding modelTransformBinding = {};
    modelTransformBinding.binding            = 0;
    modelTransformBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    modelTransformBinding.descriptorCount    = 1;
    modelTransformBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    modelTransformBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding viewProjTransformBinding = {};
    viewProjTransformBinding.binding            = 1;
    viewProjTransformBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    viewProjTransformBinding.descriptorCount    = 1;
    viewProjTransformBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    viewProjTransformBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding textureSamplerBinding = {};
    textureSamplerBinding.binding            = 2;
    textureSamplerBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureSamplerBinding.descriptorCount    = 1;
    textureSamplerBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    textureSamplerBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding bindings[] = { modelTransformBinding, viewProjTransformBinding, textureSamplerBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 3;
    layoutInfo.pBindings = bindings;
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->LogicalDevice(), &layoutInfo, nullptr, &_msaaDescriptorSetLayout));

    // Prepare pipeline layout.
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = PipelineLayoutCreateInfo(1, &_msaaDescriptorSetLayout, 0, nullptr);
    VK_CHECK_RESULT(vkCreatePipelineLayout(device->LogicalDevice(), &pipelineLayoutInfo, nullptr, &_msaaPipelineLayout));
}

void StereoViewingSceneRenderer::BuildMultiviewDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding textureSamplerBinding = {};
    textureSamplerBinding.binding            = 0;
    textureSamplerBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureSamplerBinding.descriptorCount    = 1;
    textureSamplerBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    textureSamplerBinding.pImmutableSamplers = nullptr;
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &textureSamplerBinding;
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->LogicalDevice(), &layoutInfo, nullptr, &_multiviewDescriptorSetLayout));

    // Prepare pipeline layout.
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = PipelineLayoutCreateInfo(1, &_multiviewDescriptorSetLayout, 0, nullptr);
    VK_CHECK_RESULT(vkCreatePipelineLayout(device->LogicalDevice(), &pipelineLayoutInfo, nullptr, &_multiviewPipelineLayout));
}

void StereoViewingSceneRenderer::BuildDescriptorPool()
{
    uint32_t numOfImageViews = swapchain->ImageViews().size();
    VkDescriptorPoolSize poolSizes[] = { {}, {}, {} };
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    poolSizes[1].descriptorCount = 1 * 2; // per eye
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[2].descriptorCount = 1 + numOfImageViews * 2; // 1(model texture) + numOfImageViews(temporary rendering results) * 2(per eye)
    VkDescriptorPoolCreateInfo descriptorPool = {};
    descriptorPool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPool.poolSizeCount = 3;
    descriptorPool.pPoolSizes = poolSizes;
    descriptorPool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptorPool.maxSets = 1 + numOfImageViews * 2; // 1(temporary rendering) + numOfImageViews(frames of displaying temporary rendering results) * 2(per eye)
    VK_CHECK_RESULT(vkCreateDescriptorPool(device->LogicalDevice(), &descriptorPool, nullptr, &_descriptorPool));
}

void StereoViewingSceneRenderer::BuildMSAADescriptorSet()
{
    VkDescriptorSetAllocateInfo descriptorSet = {};
    descriptorSet.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSet.descriptorPool              = _descriptorPool;
    descriptorSet.descriptorSetCount          = 1;
    descriptorSet.pSetLayouts                 = &_msaaDescriptorSetLayout;

    VkDescriptorBufferInfo modelTransform = {};
    modelTransform.buffer = _buffers[0].GetBuffer();
    modelTransform.offset = 0;
    modelTransform.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo viewProjTransform = {};
    viewProjTransform.buffer = _buffers[1].GetBuffer();
    viewProjTransform.offset = 0;
    viewProjTransform.range = VK_WHOLE_SIZE;

    VkDescriptorImageInfo diffuseImage = {};
    diffuseImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    diffuseImage.imageView = _modelTextures[0].ImageView();
    diffuseImage.sampler = _textureSamplers[0];

    VK_CHECK_RESULT(vkAllocateDescriptorSets(device->LogicalDevice(), &descriptorSet, &_msaaDescriptorSet));

    VkWriteDescriptorSet descriptorWrites[] = { {}, {}, {} };
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = _msaaDescriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &modelTransform;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = _msaaDescriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = &viewProjTransform;

    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = _msaaDescriptorSet;
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pImageInfo = &diffuseImage;

    vkUpdateDescriptorSets(device->LogicalDevice(), 3, descriptorWrites, 0, nullptr);
}

void StereoViewingSceneRenderer::BuildMultiViewDescriptorSet(int eye)
{
    VkDescriptorSetAllocateInfo descriptorSet = {};
    descriptorSet.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSet.descriptorPool              = _descriptorPool;
    descriptorSet.descriptorSetCount          = 1;
    descriptorSet.pSetLayouts                 = &_multiviewDescriptorSetLayout;

    VkDescriptorImageInfo msaaResolvedResult = {};
    msaaResolvedResult.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    msaaResolvedResult.sampler = _msaaResolvedResultSampler;

    int size = swapchain->ImageViews().size();
    bool leftEye = (eye == 0);
    vector<VkDescriptorSet>& descriptorSets = leftEye ? _lDescriptorSets : _rDescriptorSets;
    vector<VkImageView>& views = leftEye ? _lMsaaResolvedViews : _rMsaaResolvedViews;
    for (int i = 0; i < size; i++) {
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device->LogicalDevice(), &descriptorSet, &descriptorSets[i]));

        VkWriteDescriptorSet descriptorWrites[] = { {} };
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[0].descriptorCount = 1;
        msaaResolvedResult.imageView = views[i];
        descriptorWrites[0].pImageInfo = &msaaResolvedResult;

        vkUpdateDescriptorSets(device->LogicalDevice(), 1, descriptorWrites, 0, nullptr);
    }
}

void StereoViewingSceneRenderer::BuildMSAAPipeline(void* application, const VertexLayout& vertexLayout, VkSampleCountFlagBits sampleCount)
{
    android_app* app = (android_app*)application;
    vector<char*> vertFile, fragFile;
    AndroidNative::Open<char*>("shaders/vr/blinn_phong_shading_wo_normal_map.vert.spv", app, vertFile);
    AndroidNative::Open<char*>("shaders/vr/blinn_phong_shading_wo_normal_map.frag.spv", app, fragFile);
    VkShaderModuleCreateInfo vertModule = ShaderModuleCreateInfo(vertFile);
    VkShaderModuleCreateInfo fragModule = ShaderModuleCreateInfo(fragFile);
    VkShaderModule vertexShader;
    VkDevice d = device->LogicalDevice();
    VK_CHECK_RESULT(vkCreateShaderModule(d, &vertModule, nullptr, &vertexShader));
    VkShaderModule fragmentShader;
    VK_CHECK_RESULT(vkCreateShaderModule(d, &fragModule, nullptr, &fragmentShader));

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader),
        PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader)
    };

    VkVertexInputBindingDescription vertexInputBinding = {};
    vertexInputBinding.binding                         = 0;
    vertexInputBinding.stride                          = vertexLayout.Stride();
    vertexInputBinding.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;
    vector<VkVertexInputAttributeDescription> vertexAttributes;
    VkFormat format;
    for (uint32_t i = 0; i < vertexLayout.components.size(); i++) {
        vertexAttributes.emplace_back();
        vertexAttributes[i].location = i;
        vertexAttributes[i].binding  = 0;
        if (vertexLayout.components[i] != VertexComponent::VERTEX_COMPONENT_UV) {
            format = VK_FORMAT_R32G32B32_SFLOAT;
        } else {
            format = VK_FORMAT_R32G32_SFLOAT;
        }
        vertexAttributes[i].format = format;
        vertexAttributes[i].offset = vertexLayout.offsets[i];
    }
    VkPipelineVertexInputStateCreateInfo pipelineVertexInput = PipelineVertexInputStateCreateInfo(1, &vertexInputBinding, vertexAttributes.size(), vertexAttributes.data());

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology                               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable                 = VK_FALSE;

    // pTessellationState  ignored.

    VkExtent2D extent2D = swapchain->Extent();
    VkViewport viewport = Viewport(extent2D.width, extent2D.height);
    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = extent2D;
    VkPipelineViewportStateCreateInfo viewportState = PipelineViewport(&viewport, &scissor);

    VkPipelineRasterizationStateCreateInfo rasterization = Rasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

    VkPipelineMultisampleStateCreateInfo multisample = {};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = sampleCount;
    multisample.sampleShadingEnable = (sampleCount == VK_SAMPLE_COUNT_1_BIT) ? VK_FALSE : VK_TRUE;

    VkPipelineDepthStencilStateCreateInfo depthStencil = DepthStencil();

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask                      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable                         = VK_FALSE;
    float blendConstants[] = { VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO };
    VkPipelineColorBlendStateCreateInfo colorBlend = ColorBlend(VK_FALSE, VK_LOGIC_OP_NO_OP, 1, &colorBlendAttachment, blendConstants);

    // pDynamicState ignored.

    GraphicsPipelineInfoParameters parameters = GraphicsPipelineInfoParameters();
    parameters.pMultisampleState  = &multisample;
    parameters.pDepthStencilState = &depthStencil;
    VkGraphicsPipelineCreateInfo graphicsPipeline = GraphicsPipelineCreateInfo(2,
                                                                               shaderStages,
                                                                               &pipelineVertexInput,
                                                                               &inputAssembly,
                                                                               &viewportState,
                                                                               &rasterization,
                                                                               &colorBlend,
                                                                               _msaaPipelineLayout,
                                                                               (*renderPasses[0]).GetRenderPass(),
                                                                               &parameters);
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(d, VK_NULL_HANDLE, 1, &graphicsPipeline, nullptr, &_msaaPipeline));

    vkDestroyShaderModule(d, vertexShader, nullptr);
    vkDestroyShaderModule(d, fragmentShader, nullptr);
}

void StereoViewingSceneRenderer::BuildMultiviewPipeline(void* application, const VertexLayout& vertexLayout)
{
    android_app* app = (android_app*)application;
    vector<char*> vertFile, fragFile;
    AndroidNative::Open<char*>("shaders/multiview.vert.spv", app, vertFile);
    AndroidNative::Open<char*>("shaders/multiview.frag.spv", app, fragFile);
    VkShaderModuleCreateInfo vertModule = ShaderModuleCreateInfo(vertFile);
    VkShaderModuleCreateInfo fragModule = ShaderModuleCreateInfo(fragFile);
    VkShaderModule vertexShader;
    VkDevice d = device->LogicalDevice();
    VK_CHECK_RESULT(vkCreateShaderModule(d, &vertModule, nullptr, &vertexShader));
    VkShaderModule fragmentShader;
    VK_CHECK_RESULT(vkCreateShaderModule(d, &fragModule, nullptr, &fragmentShader));

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader),
        PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader)
    };

    VkVertexInputBindingDescription vertexInputBinding = {};
    vertexInputBinding.binding                         = 0;
    vertexInputBinding.stride                          = vertexLayout.Stride();
    vertexInputBinding.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;
    vector<VkVertexInputAttributeDescription> vertexAttributes;
    VkFormat format;
    for (uint32_t i = 0; i < vertexLayout.components.size(); i++) {
        vertexAttributes.emplace_back();
        vertexAttributes[i].location = i;
        vertexAttributes[i].binding  = 0;
        if (vertexLayout.components[i] != VertexComponent::VERTEX_COMPONENT_UV) {
            format = VK_FORMAT_R32G32B32_SFLOAT;
        } else {
            format = VK_FORMAT_R32G32_SFLOAT;
        }
        vertexAttributes[i].format = format;
        vertexAttributes[i].offset = vertexLayout.offsets[i];
    }
    VkPipelineVertexInputStateCreateInfo pipelineVertexInput = PipelineVertexInputStateCreateInfo(1, &vertexInputBinding, vertexAttributes.size(), vertexAttributes.data());

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology                               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable                 = VK_FALSE;

    // pTessellationState ignored.

    VkPipelineViewportStateCreateInfo viewportState = PipelineViewport(nullptr, nullptr);

    VkPipelineRasterizationStateCreateInfo rasterization = Rasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

    VkPipelineMultisampleStateCreateInfo multisample = {};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample.sampleShadingEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil = DepthStencil();

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask                      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable                         = VK_FALSE;
    float blendConstants[] = { VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO };
    VkPipelineColorBlendStateCreateInfo colorBlend = ColorBlend(VK_FALSE, VK_LOGIC_OP_NO_OP, 1, &colorBlendAttachment, blendConstants);

    VkDynamicState dynamicStateEnables[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStateEnables;

    GraphicsPipelineInfoParameters parameters = GraphicsPipelineInfoParameters();
    parameters.pDepthStencilState = &depthStencil;
    parameters.pMultisampleState  = &multisample;
    parameters.pDynamicState      = &dynamicState;
    VkGraphicsPipelineCreateInfo graphicsPipeline = GraphicsPipelineCreateInfo(2,
                                                                               shaderStages,
                                                                               &pipelineVertexInput,
                                                                               &inputAssembly,
                                                                               &viewportState,
                                                                               &rasterization,
                                                                               &colorBlend,
                                                                               _multiviewPipelineLayout,
                                                                               (*renderPasses[1]).GetRenderPass(),
                                                                               &parameters);
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(d, VK_NULL_HANDLE, 1, &graphicsPipeline, nullptr, &_multiviewPipeline));

    vkDestroyShaderModule(d, vertexShader, nullptr);
    vkDestroyShaderModule(d, fragmentShader, nullptr);
}

void StereoViewingSceneRenderer::BuildCommandBuffers(int index)
{
    RenderPass* msaaRenderPass = renderPasses[0];
    Command::BeginCommandBuffer(_commandBuffers.buffers[index], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    const VkExtent2D& extent = swapchain->Extent();

    // left eye
    VkRenderPassBeginInfo lRenderPassBegin = {};
    lRenderPassBegin.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    lRenderPassBegin.renderPass            = msaaRenderPass->GetRenderPass();
    lRenderPassBegin.framebuffer           = _lMsaaFramebuffers[index].GetFramebuffer();
    lRenderPassBegin.renderArea.offset     = { 0, 0 };
    lRenderPassBegin.renderArea.extent     = extent;
    lRenderPassBegin.clearValueCount       = 2;

    vector<VkClearValue> msaaClearValues = { { 0.03125f, 0.0625f, 0.15625f, 0.0f }, { 1.0f, 0 } };
    lRenderPassBegin.pClearValues = msaaClearValues.data();
    vkCmdBeginRenderPass(_commandBuffers.buffers[index], &lRenderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(_commandBuffers.buffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, _msaaPipeline);
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(_commandBuffers.buffers[index], 0, 1, &_modelResources[0].VertexBuffer().GetBuffer(), offsets);
    vkCmdBindIndexBuffer(_commandBuffers.buffers[index], _modelResources[0].IndexBuffer().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    uint32_t dynamicOffsets = 0;
    vkCmdBindDescriptorSets(_commandBuffers.buffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, _msaaPipelineLayout, 0, 1, &_msaaDescriptorSet, 1, &dynamicOffsets);
    uint32_t ic = _modelResources[0].IndicesCount();
    vkCmdDrawIndexed(_commandBuffers.buffers[index], _modelResources[0].IndicesCount(), 1, 0, 0, 0);
    vkCmdEndRenderPass(_commandBuffers.buffers[index]);

    // right eye
    VkRenderPassBeginInfo rRenderPassBegin = lRenderPassBegin;
    rRenderPassBegin.framebuffer = _rMsaaFramebuffers[index].GetFramebuffer();
    vkCmdBeginRenderPass(_commandBuffers.buffers[index], &rRenderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(_commandBuffers.buffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, _msaaPipeline);
    dynamicOffsets = _dynamicBufferAlignment;
    vkCmdBindDescriptorSets(_commandBuffers.buffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, _msaaPipelineLayout, 0, 1, &_msaaDescriptorSet, 1, &dynamicOffsets);
    vkCmdDrawIndexed(_commandBuffers.buffers[index], _modelResources[0].IndicesCount(), 1, 0, 0, 0);
    vkCmdEndRenderPass(_commandBuffers.buffers[index]);


    // multiview
    vector<VkClearValue> multiviewClearValues = { { 0.0f, 0.0f, 0.0f, .0f } };

    VkViewport viewport = {};
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};

    VkRenderPassBeginInfo multiviewRenderPassBegin = {};
    multiviewRenderPassBegin.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPass* resolvedImageRenderPass = renderPasses[1];
    multiviewRenderPassBegin.renderPass            = resolvedImageRenderPass->GetRenderPass();
    multiviewRenderPassBegin.framebuffer           = framebuffers[index].GetFramebuffer();
    multiviewRenderPassBegin.renderArea.offset     = { 0, 0 };
    multiviewRenderPassBegin.renderArea.extent     = extent;
    multiviewRenderPassBegin.clearValueCount       = 1;
    multiviewRenderPassBegin.pClearValues          = multiviewClearValues.data();
    vkCmdBeginRenderPass(_commandBuffers.buffers[index], &multiviewRenderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(_commandBuffers.buffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, _multiviewPipeline);

    vkCmdBindVertexBuffers(_commandBuffers.buffers[index], 0, 1, &_modelResources[1].VertexBuffer().GetBuffer(), offsets);
    vkCmdBindIndexBuffer(_commandBuffers.buffers[index], _modelResources[1].IndexBuffer().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    // left viewport
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)extent.width / 2;
    viewport.height = (float)extent.height;
    vkCmdSetViewport(_commandBuffers.buffers[index], 0, 1, &viewport);

    scissor.offset = { 0, 0 };
    scissor.extent.width = viewport.width;
    scissor.extent.height = viewport.height;
    vkCmdSetScissor(_commandBuffers.buffers[index], 0, 1, &scissor);

    vkCmdBindDescriptorSets(_commandBuffers.buffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, _multiviewPipelineLayout, 0, 1, &_lDescriptorSets[index], 0, nullptr);
    vkCmdDrawIndexed(_commandBuffers.buffers[index], _modelResources[1].IndicesCount(), 1, 0, 0, 0);

    // right viewport
    viewport.x = (float)extent.width / 2;
    viewport.y = 0.0f;
    viewport.width = (float)extent.width / 2;
    viewport.height = (float)extent.height;
    vkCmdSetViewport(_commandBuffers.buffers[index], 0, 1, &viewport);

    scissor.offset = { (int32_t)viewport.width, 0 };
    scissor.extent.width = viewport.width;
    scissor.extent.height = viewport.height;
    vkCmdSetScissor(_commandBuffers.buffers[index], 0, 1, &scissor);

    vkCmdBindDescriptorSets(_commandBuffers.buffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, _multiviewPipelineLayout, 0, 1, _rDescriptorSets.data(), 0, nullptr);
    vkCmdDrawIndexed(_commandBuffers.buffers[index], _modelResources[1].IndicesCount(), 1, 0, 0, 0);

    vkCmdEndRenderPass(_commandBuffers.buffers[index]);

    VK_CHECK_RESULT(vkEndCommandBuffer(_commandBuffers.buffers[index]));
}

void StereoViewingSceneRenderer::RebuildSwapchain()
{
    DeleteSwapchainWithDependencies();

    BuildSwapchainWithDependencies();

    BuildMSAAImage(_sampleCount, 0);
    BuildMSAAImage(_sampleCount, 1);
    BuildMSAADepthImage(renderPasses[0], _sampleCount, 0);
    BuildMSAADepthImage(renderPasses[0], _sampleCount, 1);
    BuildMSAAResolvedImages(0);
    BuildMSAAResolvedImages(1);

    BuildMSAADescriptorSetLayout();
    BuildMultiviewDescriptorSetLayout();
    BuildDescriptorPool();
    BuildMSAADescriptorSet();
    BuildMultiViewDescriptorSet(0);
    BuildMultiViewDescriptorSet(1);

    android_app* app = (android_app*)_application;
    BuildMSAAPipeline(app, _vertexLayouts[0], _sampleCount);
    BuildMultiviewPipeline(app, _vertexLayouts[1]);
}

#endif