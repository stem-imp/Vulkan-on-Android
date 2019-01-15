#ifdef __ANDROID__

#include "../msaa_scene_renderer.h"
#include "../../../vulkan/renderpass/msaa_renderpass.h"
#include "../../../androidutility/assetmanager/io_asset.hpp"
#include "../../../vulkan/vulkan_utility.h"
#include "glm/gtc/matrix_transform.hpp"

#include <unordered_map>

using Vulkan::Instance;
using Vulkan::Surface;
using Vulkan::Device;
using Vulkan::Swapchain;
using Vulkan::RenderPass;
using Vulkan::MSAARenderPass;
using Vulkan::Model;
using Vulkan::ModelCreateInfo;
using Vulkan::VertexComponent;
using Vulkan::VertexLayout;
//using Vulkan::MVP;
using std::unordered_map;

using std::min;
using std::max;

static MSAASceneRenderer::Lighting lighting;
static unordered_map<int, int> currentFrameToImageindex;
static unordered_map<int, int> imageIndexToCurrentFrame;

MSAASceneRenderer::MSAASceneRenderer(void* application, uint32_t screenWidth, uint32_t screenHeight) : Renderer(application, screenWidth, screenHeight)
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
    device = new Device(SelectPhysicalDevice(*instance, *surface, requestedExtNames, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT, requestedFeatures));
    VkPhysicalDeviceFeatures featuresRequested = { .samplerAnisotropy = VK_TRUE };
    device->BuildDevice(featuresRequested, requestedExtNames);

    swapchain = new Swapchain(*surface, *device);
    swapchain->getScreenExtent = [&]() -> Extent2D { return screenSize; };
    BuildSwapchain(*swapchain);

    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_4_BIT;
    device->RequestSampleCount(samples);
    RenderPass* swapchainRenderPass;
    swapchainRenderPass = new MSAARenderPass(*device);
    swapchainRenderPass->getFormat = [this]() -> VkFormat { return swapchain->Format(); };
    swapchainRenderPass->getSampleCount = [samples]() -> VkSampleCountFlagBits { return samples; };
    swapchainRenderPass->CreateRenderPass();
    renderPasses.push_back(swapchainRenderPass);

    int size = swapchain->ImageViews().size();
    for (int i = 0 ; i < size; i++) {
        framebuffers.push_back(*device);
    }

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


    // Load Model
    _application = application;
    android_app* app = (android_app*)application;
    models.emplace_back(Model());
    Model& model = models[0];
    string filePath = string(app->activity->externalDataPath) + string("/cuberb1k/");
    Texture::TextureAttribs textureAttribs;
    ModelCreateInfo modelCreateInfo = { 0.015625f, 1.0f, true, true };
    unsigned int flags = aiProcess_Triangulate |
                         aiProcess_GenNormals |
                         aiProcess_GenUVCoords |
                         aiProcess_ValidateDataStructure |
                         aiProcess_RemoveRedundantMaterials |
                         aiProcess_FixInfacingNormals |
                         aiProcess_FindDegenerates |
                         aiProcess_FindInvalidData |
                         aiProcess_OptimizeMeshes |
                         aiProcess_OptimizeGraph |
                         aiProcess_FlipUVs;
    if (model.ReadFile(filePath, string("cube.obj"), flags, &modelCreateInfo)) {
        for (const auto& n : model.Materials()) {
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
                    _modelTextures.emplace_back(*device);
                    _modelTextures[_modelTextures.size() - 1].BuildTexture2D(textureAttribs, imageData, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *command);
                }
            }
        }
        _modelResources.emplace_back(*device);
        _modelResources[_modelResources.size() - 1].UploadToGPU(model, *command);
    }


    // Prepare MVP & lighting buffer.
    VkDeviceSize mvpSize = sizeof(Vulkan::MVP);
    _buffers.emplace_back(*device);
    Buffer& mvpBuffer = _buffers[0];
    mvpBuffer.BuildDefaultBuffer(mvpSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    mvpBuffer.Map();

    _buffers.emplace_back(*device);
    Buffer& lightPosBuffer = _buffers[1];
    lightPosBuffer.BuildDefaultBuffer(sizeof(Lighting), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    lightPosBuffer.Map();


    // Prepare sampler.
    float samplerAnisotropy = device->FeaturesEnabled().samplerAnisotropy ? 8 : 1;
    device->RequsetSamplerAnisotropy(samplerAnisotropy);
    VkSamplerCreateInfo samplerInfo = SamplerCreateInfo(textureAttribs.samplerMipmapMode,
                                                        textureAttribs.mipmapLevels,
                                                        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                                        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                                        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                                        samplerAnisotropy);
    vkCreateSampler(d, &samplerInfo, nullptr, &_diffuseSampler);


    BuildMSAAImage(samples);
    BuildDepthImage(swapchainRenderPass, samples);


    // Prepare descriptor related stuff.
    BuildDescriptorSetLayout();
    BuildDescriptorPool();
    BuildDescriptorSet();

    BuildGraphicsPipeline(application, samples);
}

MSAASceneRenderer::~MSAASceneRenderer()
{
    DebugLog("~MSAASceneRenderer()");

    VkDevice d = device->LogicalDevice();

    vkDeviceWaitIdle(d);

    vkDestroySampler(d, _diffuseSampler, nullptr), _diffuseSampler = VK_NULL_HANDLE;
    _buffers.clear();
    _modelTextures.clear();
    _modelResources.clear();

    vkDestroyImageView(d, _msaaView, nullptr), _msaaView = VK_NULL_HANDLE;
    vkDestroyImage(d, _msaaImage, nullptr), _msaaImage = VK_NULL_HANDLE;
    vkFreeMemory(d, _msaaImageMemory, nullptr), _msaaImageMemory = VK_NULL_HANDLE;
    vkDestroyImageView(d, _depthView, nullptr), _depthView = VK_NULL_HANDLE;
    vkDestroyImage(d, _depthImage, nullptr), _depthImage = VK_NULL_HANDLE;
    vkFreeMemory(d, _depthImageMemory, nullptr), _depthImageMemory = VK_NULL_HANDLE;

    for (int i = 0; i < swapchain->ConcurrentFramesCount(); i++) {
        vkDestroyFence(d, multiFrameFences[i], nullptr), multiFrameFences[i] = VK_NULL_HANDLE;
        vkDestroySemaphore(d, imageAvailableSemaphores[i], nullptr), imageAvailableSemaphores[i] = VK_NULL_HANDLE;
        vkDestroySemaphore(d, commandsCompleteSemaphores[i], nullptr), commandsCompleteSemaphores[i] = VK_NULL_HANDLE;
    }

    delete command, command = nullptr;

    vkDestroyDescriptorSetLayout(d, _descriptorSetLayout, nullptr), _descriptorSetLayout = VK_NULL_HANDLE;
    vkDestroyDescriptorPool(d, _descriptorPool, nullptr), _descriptorPool = VK_NULL_HANDLE;
    vkDestroyPipelineLayout(d, _pipelineLayout, nullptr), _pipelineLayout = VK_NULL_HANDLE;

    vkDestroyPipeline(d, _pipeline, nullptr), _pipeline = VK_NULL_HANDLE;

    framebuffers.clear();

    for (auto& r : renderPasses) {
        delete r, r = nullptr;
    }
    renderPasses.clear();

    delete swapchain        , swapchain         = nullptr;
    delete device           , device            = nullptr;
    delete surface          , surface           = nullptr;
    delete layerAndExtension, layerAndExtension = nullptr;
    delete instance         , instance          = nullptr;
}

void MSAASceneRenderer::BuildCommandBuffer(int index)
{
    RenderPass* swapchainRenderPass = renderPasses[0];
    Command::BeginCommandBuffer(_commandBuffers.buffers[index], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass        = swapchainRenderPass->GetRenderPass();
    renderPassBeginInfo.framebuffer       = framebuffers[index].GetFramebuffer();
    renderPassBeginInfo.renderArea.offset = { 0, 0 };
    renderPassBeginInfo.renderArea.extent = swapchain->Extent();
    renderPassBeginInfo.clearValueCount   = 2;

    vector<VkClearValue> clearValues = { { 0.03125f, 0.0625f, 0.15625f, 0.0f }, { 1.0f, 0 } };
    renderPassBeginInfo.pClearValues = clearValues.data();
    vkCmdBeginRenderPass(_commandBuffers.buffers[index], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(_commandBuffers.buffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(_commandBuffers.buffers[index], 0, 1, &_modelResources[0].VertexBuffer().GetBuffer(), offsets);
    vkCmdBindIndexBuffer(_commandBuffers.buffers[index], _modelResources[0].IndexBuffer().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(_commandBuffers.buffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &_descriptorSet, 0, nullptr);
    vkCmdDrawIndexed(_commandBuffers.buffers[index], _modelResources[0].IndicesCount(), 1, 0, 0, 0);

    vkCmdEndRenderPass(_commandBuffers.buffers[index]);
    VK_CHECK_RESULT(vkEndCommandBuffer(_commandBuffers.buffers[index]));
}

void MSAASceneRenderer::RenderImpl()
{
    if (!device->SharedGraphicsAndPresentQueueFamily()) {
        return;
    }

    if (orientationChanged) {
        DebugLog("Orientation changed.");
        orientationChanged = false;
        RebuildSwapchain();
        return;
    }

    VkDevice d = device->LogicalDevice();

    vkWaitForFences(d, 1, &multiFrameFences[currentFrameIndex], true, UINT64_MAX);
    vkResetFences(d, 1, &multiFrameFences[currentFrameIndex]);
    if (currentFrameToImageindex.count(currentFrameIndex) > 0) {
        framebuffers[currentFrameToImageindex[currentFrameIndex]].ReleaseFramebuffer();
        currentFrameToImageindex.erase(currentFrameIndex);
    }

    uint32_t imageIndex;
    vkAcquireNextImageKHR(d, swapchain->GetSwapchain(), UINT64_MAX, imageAvailableSemaphores[currentFrameIndex], VK_NULL_HANDLE, &imageIndex);
    if (imageIndexToCurrentFrame.count(imageIndex) > 0) {
        vkWaitForFences(d, 1, &multiFrameFences[imageIndexToCurrentFrame[imageIndex]], true, UINT64_MAX);
        vkResetFences(d, 1, &multiFrameFences[imageIndexToCurrentFrame[imageIndex]]);
        framebuffers[imageIndex].ReleaseFramebuffer();
        currentFrameToImageindex.erase(imageIndexToCurrentFrame[imageIndex]);
        imageIndexToCurrentFrame.erase(imageIndex);
    }

    vector<VkImageView> attachments = { _msaaView, _depthView, swapchain->ImageViews()[imageIndex] };
    Extent2D e = swapchain->getScreenExtent();
    framebuffers[imageIndex].CreateSwapchainFramebuffer(renderPasses[0]->GetRenderPass(), attachments, { e.width, e.height });
    currentFrameToImageindex[currentFrameIndex] = imageIndex;
    imageIndexToCurrentFrame[imageIndex] = currentFrameIndex;

    BuildCommandBuffer(imageIndex);

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

void MSAASceneRenderer::BuildMSAAImage(VkSampleCountFlagBits sampleCount)
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
    VK_CHECK_RESULT(vkCreateImage(d, &imageInfo, nullptr, &_msaaImage));

    VkMemoryRequirements memoryRequirements;
    uint32_t memoryTypeIndex;
    VkMemoryAllocateInfo memoryAllocateInfo = MemoryAllocateInfo(_msaaImage,
                                                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                                 *device, memoryRequirements,
                                                                 memoryTypeIndex);
    VK_CHECK_RESULT(vkAllocateMemory(d, &memoryAllocateInfo, nullptr, &_msaaImageMemory));
    VK_CHECK_RESULT(vkBindImageMemory(d, _msaaImage, _msaaImageMemory, 0));

    VkImageViewCreateInfo imageViewInfo = ImageViewCreateInfo(_msaaImage, colorFormat, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
    VK_CHECK_RESULT(vkCreateImageView(d, &imageViewInfo, nullptr, &_msaaView));
}

void MSAASceneRenderer::BuildDepthImage(RenderPass* swapchainRenderPass, VkSampleCountFlagBits sampleCount)
{
    VkFormat depthFormat = ((MSAARenderPass*)swapchainRenderPass)->DepthFormat();
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
    VK_CHECK_RESULT(vkCreateImage(d, &imageInfo, nullptr, &_depthImage));

    VkMemoryRequirements memoryRequirements;
    uint32_t memoryTypeIndex;
    VkMemoryAllocateInfo memoryAllocateInfo = MemoryAllocateInfo(_depthImage,
                                                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                                 *device, memoryRequirements,
                                                                 memoryTypeIndex);
    VK_CHECK_RESULT(vkAllocateMemory(d, &memoryAllocateInfo, nullptr, &_depthImageMemory));
    VK_CHECK_RESULT(vkBindImageMemory(d, _depthImage, _depthImageMemory, 0));

    VkImageViewCreateInfo imageViewInfo = ImageViewCreateInfo(_depthImage, depthFormat, { depthImageAspectFlags, 0, 1, 0, 1 });
    VK_CHECK_RESULT(vkCreateImageView(d, &imageViewInfo, nullptr, &_depthView));
}

void MSAASceneRenderer::BuildDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboBinding = {};
    uboBinding.binding            = 0;
    uboBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount    = 1;
    uboBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    uboBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerBinding = {};
    samplerBinding.binding            = 1;
    samplerBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount    = 1;
    samplerBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerBinding.pImmutableSamplers = nullptr;

//    VkDescriptorSetLayoutBinding samplerBinding1 = {};
//    samplerBinding1.binding                      = 2;
//    samplerBinding1.descriptorType               = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//    samplerBinding1.descriptorCount              = 1;
//    samplerBinding1.stageFlags                   = VK_SHADER_STAGE_FRAGMENT_BIT;
//    samplerBinding1.pImmutableSamplers           = nullptr;

    VkDescriptorSetLayoutBinding uboBinding1 = {};
    uboBinding1.binding            = 2;
    uboBinding1.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding1.descriptorCount    = 1;
    uboBinding1.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;;
    uboBinding1.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding binding[] = { uboBinding, samplerBinding, uboBinding1 };
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 3;
    layoutInfo.pBindings = binding;
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->LogicalDevice(), &layoutInfo, nullptr, &_descriptorSetLayout));

    // Prepare pipeline layout.
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = PipelineLayoutCreateInfo(1, &_descriptorSetLayout, 0, nullptr);
    VK_CHECK_RESULT(vkCreatePipelineLayout(device->LogicalDevice(), &pipelineLayoutInfo, nullptr, &_pipelineLayout));
}

void MSAASceneRenderer::BuildDescriptorPool()
{
    VkDescriptorPoolSize poolSizes[] = { {}, {} };
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 1;
    VkDescriptorPoolCreateInfo descriptorPool = {};
    descriptorPool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPool.poolSizeCount = 2;
    descriptorPool.pPoolSizes = poolSizes;
    descriptorPool.maxSets = 1;
    VK_CHECK_RESULT(vkCreateDescriptorPool(device->LogicalDevice(), &descriptorPool, nullptr, &_descriptorPool));
}

void MSAASceneRenderer::BuildDescriptorSet()
{
    VkDescriptorSetAllocateInfo descriptorSet = {};
    descriptorSet.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSet.descriptorPool              = _descriptorPool;
    descriptorSet.descriptorSetCount          = 1;
    descriptorSet.pSetLayouts                 = &_descriptorSetLayout;
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device->LogicalDevice(), &descriptorSet, &_descriptorSet));

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = _buffers[0].GetBuffer(); // MVP
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;

//    VkDescriptorBufferInfo bufferDynamicInfo = {};
//    bufferDynamicInfo.buffer = dynamicVPBuff;
//    bufferDynamicInfo.offset = 0;
//    bufferDynamicInfo.range = VK_WHOLE_SIZE;

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = _modelTextures[0].ImageView(); // diffuse texture
    imageInfo.sampler = _diffuseSampler;

//    VkDescriptorImageInfo imageInfo1 = {};
//    imageInfo1.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//    imageInfo1.imageView = _modelTextures[0].ImageView(); // normal texture
//    imageInfo1.sampler = _diffuseSampler;

    VkDescriptorBufferInfo bufferInfo1 = {};
    bufferInfo1.buffer = _buffers[1].GetBuffer(); // lighting
    bufferInfo1.offset = 0;
    bufferInfo1.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet descriptorWrites[] = { {}, {}, {} };
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = _descriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

//    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//    descriptorWrites[1].dstSet = descriptor[eye].sets[0];
//    descriptorWrites[1].dstBinding = 1;
//    descriptorWrites[1].dstArrayElement = 0;
//    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
//    descriptorWrites[1].descriptorCount = 1;
//    descriptorWrites[1].pBufferInfo = &bufferDynamicInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = _descriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;

//    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//    descriptorWrites[2].dstSet = _descriptorSet;
//    descriptorWrites[2].dstBinding = 2;
//    descriptorWrites[2].dstArrayElement = 0;
//    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//    descriptorWrites[2].descriptorCount = 1;
//    descriptorWrites[2].pImageInfo = &imageInfo1;

    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = _descriptorSet;
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = &bufferInfo1;

    vkUpdateDescriptorSets(device->LogicalDevice(), 3, descriptorWrites, 0, nullptr);
}

void MSAASceneRenderer::BuildGraphicsPipeline(void* application, VkSampleCountFlagBits sampleCount)
{
    android_app* app = (android_app*)application;
    vector<char*> vertFile, fragFile;
    AndroidNative::Open<char*>("shaders/blinn_phong_shading_wo_normal_map.vert.spv", app, vertFile);
    AndroidNative::Open<char*>("shaders/blinn_phong_shading_wo_normal_map.frag.spv", app, fragFile);
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

    Model& model = models[0];
    const VertexLayout& vertexLayout = model.VertexLayouts()[0];
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
    scissor.offset = {0, 0};
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
                                                                               _pipelineLayout,
                                                                               (*renderPasses[0]).GetRenderPass(),
                                                                               &parameters);
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(d, VK_NULL_HANDLE, 1, &graphicsPipeline, nullptr, &_pipeline));

    vkDestroyShaderModule(d, vertexShader, nullptr);
    vkDestroyShaderModule(d, fragmentShader, nullptr);
}

void MSAASceneRenderer::Update(float elapsedTime)
{
    MVP mvp;
    mvp.model = glm::rotate(mat4(1.0f), glm::radians(elapsedTime * 16.0f), vec3(1.5f, 0.5f, -1.0f));
    vec3 worldSpaceCameraPos = vec3(0.0f, 1.0f, 2.0f);
    mvp.view = glm::lookAt(worldSpaceCameraPos, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    mvp.projection = glm::perspective(glm::radians(90.0f), swapchain->Extent().width / (float)swapchain->Extent().height, 0.125f, 16.0f);
    mvp.projection[1][1] *= -1;
    mvp.mInvTranspose = glm::transpose(glm::inverse(mvp.model));
    std::copy((uint8_t*)&mvp, (uint8_t*)&mvp + sizeof(MVP), (uint8_t*)_buffers[0].mapped);

    lighting.worldSpaceLightPos = vec3(12.0f * cos(glm::radians(elapsedTime * 16.0f)), 4.0f, 12.0f);
    lighting.worldSpaceCameraPos = worldSpaceCameraPos;
    Buffer& lightPosBuffer = _buffers[1];
    std::copy((uint8_t*)&lighting, (uint8_t*)&lighting + sizeof(Lighting), (uint8_t*)lightPosBuffer.mapped);
}

void MSAASceneRenderer::RebuildSwapchain()
{
    DebugLog("RebuildSwapchain()");

    VkDevice d = device->LogicalDevice();

    vkDeviceWaitIdle(d);

    // Delete out-dated data.
    vkDestroyImageView(d, _msaaView, nullptr), _msaaView = VK_NULL_HANDLE;
    vkDestroyImage(d, _msaaImage, nullptr), _msaaImage = VK_NULL_HANDLE;
    vkFreeMemory(d, _msaaImageMemory, nullptr), _msaaImageMemory = VK_NULL_HANDLE;
    vkDestroyImageView(d, _depthView, nullptr), _depthView = VK_NULL_HANDLE;
    vkDestroyImage(d, _depthImage, nullptr), _depthImage = VK_NULL_HANDLE;
    vkFreeMemory(d, _depthImageMemory, nullptr), _depthImageMemory = VK_NULL_HANDLE;

    vkDestroyPipeline(d, _pipeline, nullptr), _pipeline = VK_NULL_HANDLE;

    framebuffers.clear();

    for (auto& r : renderPasses) {
        delete r, r = nullptr;
    }
    renderPasses.clear();

    delete swapchain, swapchain         = nullptr;


    // Create new data.
    swapchain = new Swapchain(*surface, *device);
    swapchain->getScreenExtent = [&]() -> Extent2D { return screenSize; };
    BuildSwapchain(*swapchain);

    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_4_BIT;
    device->RequestSampleCount(samples);
    RenderPass* swapchainRenderPass;
    swapchainRenderPass = new MSAARenderPass(*device);
    swapchainRenderPass->getFormat = [this]() -> VkFormat { return swapchain->Format(); };
    swapchainRenderPass->getSampleCount = [samples]() -> VkSampleCountFlagBits { return samples; };
    swapchainRenderPass->CreateRenderPass();
    renderPasses.push_back(swapchainRenderPass);

    int size = swapchain->ImageViews().size();
    for (int i = 0 ; i < size; i++) {
        framebuffers.push_back(*device);
    }

    BuildMSAAImage(samples);
    BuildDepthImage(swapchainRenderPass, samples);

    BuildDescriptorSetLayout();
    BuildDescriptorPool();
    BuildDescriptorSet();

    BuildGraphicsPipeline(_application, samples);
}

#endif