#ifdef __ANDROID__

#include "../earth_scene_renderer.h"
#include "../../../vulkan/renderpass/color_depth_renderpass.h"
#include "../../../vulkan/texture/texture.h"
#include "../../../vulkan/vulkan_utility.h"
#include "../../../androidutility/assetmanager/io_asset.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <unordered_map>
#include <android_native_app_glue.h>

using Vulkan::Instance;
using Vulkan::Surface;
using Vulkan::Device;
using Vulkan::Swapchain;
using Vulkan::RenderPass;
using Vulkan::ColorDepthRenderPass;
using Vulkan::ModelCreateInfo;
using Vulkan::VertexComponent;
using Vulkan::VertexLayout;
using Vulkan::Model;
using Vulkan::MVP;
using std::unordered_map;

using std::min;
using std::max;

static VkViewport viewport = { .x = 0.0f, .y = 0.0f, .minDepth = 0.0f, .maxDepth = 1.0f };
static VkRect2D scissorRect = { .offset = { 0, 0 } };
static unordered_map<int, int> currentFrameToImageindex;
static unordered_map<int, int> imageIndexToCurrentFrame;

EarthSceneRenderer::EarthSceneRenderer(void* application, uint32_t screenWidth, uint32_t screenHeight) : Renderer(application, screenWidth, screenHeight)
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
    VkPhysicalDeviceFeatures featuresRequested = {};
    device->BuildDevice(featuresRequested, requestedExtNames);

    swapchain = new Swapchain(*surface, *device);
    swapchain->getScreenExtent = [&]() -> Extent2D { return screenSize; };
    BuildSwapchain(*swapchain);

    RenderPass* swapchainRenderPass;
    BuildRenderPass<ColorDepthRenderPass>(swapchainRenderPass, *swapchain, *device);
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


    // Load models.
    android_app* app = (android_app*)application;
    models.emplace_back(Model());
    Model& model = models[0];
    string filePath = string(app->activity->externalDataPath) + string("/earth/");
    Texture::TextureAttribs textureAttribs;
    ModelCreateInfo modelCreateInfo = { 0.001953125f, 1.0f };
    if (model.ReadFile(filePath, string("earth.obj"), Model::DEFAULT_READ_FILE_FLAGS, &modelCreateInfo)) {
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
                    textureAttribs.mipmapLevels = (uint32_t)(floor(log2(max(textureAttribs.width, textureAttribs.height)))) + 1;
                    _modelTextures.emplace_back(*device);
                    _modelTextures[_modelTextures.size() - 1].BuildTexture2D(textureAttribs, imageData, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *command);
                }
            }
        }
        _modelResources.emplace_back(*device);
        _modelResources[_modelResources.size() - 1].UploadToGPU(model, *command);
    }


    // Prepare MVP buffer.
    VkDeviceSize mvpSize = sizeof(Vulkan::MVP);
    _buffers.emplace_back(*device);
    Buffer& mvpBuffer = _buffers[0];
    mvpBuffer.BuildDefaultBuffer(mvpSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    mvpBuffer.Map();


    // Prepare sampler.
    float samplerAnisotropy = device->FeaturesEnabled().samplerAnisotropy ? 8 : 0;
    device->RequsetSamplerAnisotropy(samplerAnisotropy);
    VkSamplerCreateInfo samplerInfo = SamplerCreateInfo(textureAttribs.samplerMipmapMode,
                                                        textureAttribs.mipmapLevels,
                                                        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                                                        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                                                        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                                                        samplerAnisotropy);
    vkCreateSampler(d, &samplerInfo, nullptr, &_diffuseSampler);


    // Prepare depth image.
    VkFormat depthFormat = ((ColorDepthRenderPass*)swapchainRenderPass)->DepthFormat();
    VkImageAspectFlags depthImageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (depthFormat == VK_FORMAT_D16_UNORM_S8_UINT || depthFormat == VK_FORMAT_D24_UNORM_S8_UINT || depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT) {
        depthImageAspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    VkExtent2D extent2D = swapchain->Extent();
    VkImageCreateInfo imageInfo = ImageCreateInfo(depthFormat,
                                                  { extent2D.width, extent2D.height, 1 },
                                                  1,
                                                  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
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

    vector<VkCommandBuffer> cmds = Command::CreateAndBeginCommandBuffers(command->ShortLivedGraphcisPool(),
                                                                         VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                                         1,
                                                                         VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                                                                         d);
    PipelineBarrierParameters pipelineBarrierParameters = {};
    pipelineBarrierParameters.commandBuffer = cmds[0];
    pipelineBarrierParameters.imageMemoryBarrierCount = 1;
    VkImageMemoryBarrier barrier = ImageMemoryBarrier(0,
                                                      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                                                      VK_IMAGE_LAYOUT_UNDEFINED,
                                                      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                      _depthImage,
                                                      { depthImageAspectFlags, 0, 1, 0, 1 });
    pipelineBarrierParameters.pImageMemoryBarriers = &barrier;
    TransitionImageLayout(&pipelineBarrierParameters);
    Command::EndAndSubmitCommandBuffer(pipelineBarrierParameters.commandBuffer, device->FamilyQueues().graphics.queue, command->ShortLivedGraphcisPool(), d);


    // Prepare descriptor related stuff.
    BuildDescriptorSetLayout();
    BuildDescriptorPool();
    BuildDescriptorSet();


    // Prepare graphics pipeline.
    vector<char*> vertFile, fragFile;
    AndroidNative::Open<char*>("shaders/triangle.vert.spv", app, vertFile);
    AndroidNative::Open<char*>("shaders/triangle.frag.spv", app, fragFile);
    VkShaderModuleCreateInfo vertModule = ShaderModuleCreateInfo(vertFile);
    VkShaderModuleCreateInfo fragModule = ShaderModuleCreateInfo(fragFile);
    VkShaderModule vertexShader;
    VK_CHECK_RESULT(vkCreateShaderModule(d, &vertModule, nullptr, &vertexShader));
    VkShaderModule fragmentShader;
    VK_CHECK_RESULT(vkCreateShaderModule(d, &fragModule, nullptr, &fragmentShader));

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader),
        PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader)
    };

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

    VkViewport viewport = Viewport(extent2D.width, extent2D.height);
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = extent2D;
    VkPipelineViewportStateCreateInfo viewportState = PipelineViewport(&viewport, &scissor);

    VkPipelineRasterizationStateCreateInfo rasterization = Rasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

    VkPipelineMultisampleStateCreateInfo multisample = {};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    device->RequestSampleCount(samples);
    multisample.rasterizationSamples = samples;
    multisample.sampleShadingEnable = (samples == VK_SAMPLE_COUNT_1_BIT) ? VK_FALSE : VK_TRUE;

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
                                                                               parameters);
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(d, VK_NULL_HANDLE, 1, &graphicsPipeline, nullptr, &_pipeline));

    vkDestroyShaderModule(d, vertexShader, nullptr);
    vkDestroyShaderModule(d, fragmentShader, nullptr);
}

EarthSceneRenderer::~EarthSceneRenderer()
{
    DebugLog("~EarthSceneRenderer()");

    VkDevice d = device->LogicalDevice();

    vkDeviceWaitIdle(d);

    vkDestroySampler(d, _diffuseSampler, nullptr), _diffuseSampler = VK_NULL_HANDLE;
    _buffers.clear();
    _modelTextures.clear();
    _modelResources.clear();

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

void EarthSceneRenderer::BuildDescriptorSetLayout()
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

    VkDescriptorSetLayoutBinding samplerBinding1 = {};
    samplerBinding1.binding                      = 2;
    samplerBinding1.descriptorType               = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding1.descriptorCount              = 1;
    samplerBinding1.stageFlags                   = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerBinding1.pImmutableSamplers           = nullptr;

    VkDescriptorSetLayoutBinding binding[] = { uboBinding, samplerBinding, samplerBinding1 };
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 3;
    layoutInfo.pBindings = binding;
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->LogicalDevice(), &layoutInfo, nullptr, &_descriptorSetLayout));

    // Prepare pipeline layout.
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = PipelineLayoutCreateInfo(1, &_descriptorSetLayout, 0, nullptr);
    VK_CHECK_RESULT(vkCreatePipelineLayout(device->LogicalDevice(), &pipelineLayoutInfo, nullptr, &_pipelineLayout));
}

void EarthSceneRenderer::BuildDescriptorPool()
{
    VkDescriptorPoolSize poolSizes[] = { {}, {} };
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 2;
    VkDescriptorPoolCreateInfo descriptorPool = {};
    descriptorPool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPool.poolSizeCount = 2;
    descriptorPool.pPoolSizes = poolSizes;
    descriptorPool.maxSets = 1;
    VK_CHECK_RESULT(vkCreateDescriptorPool(device->LogicalDevice(), &descriptorPool, nullptr, &_descriptorPool));
}

void EarthSceneRenderer::BuildDescriptorSet()
{
    VkDescriptorSetAllocateInfo descriptorSet = {};
    descriptorSet.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSet.descriptorPool              = _descriptorPool;
    descriptorSet.descriptorSetCount          = 1;
    descriptorSet.pSetLayouts                 = &_descriptorSetLayout;
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device->LogicalDevice(), &descriptorSet, &_descriptorSet));

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = _buffers[0].GetBuffer();
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;

//    VkDescriptorBufferInfo bufferDynamicInfo = {};
//    bufferDynamicInfo.buffer = dynamicVPBuff;
//    bufferDynamicInfo.offset = 0;
//    bufferDynamicInfo.range = VK_WHOLE_SIZE;

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = _modelTextures[0].ImageView();
    imageInfo.sampler = _diffuseSampler;

    VkDescriptorImageInfo imageInfo1 = {};
    imageInfo1.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo1.imageView = _modelTextures[1].ImageView();
    imageInfo1.sampler = _diffuseSampler;

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

    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = _descriptorSet;
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pImageInfo = &imageInfo1;

    vkUpdateDescriptorSets(device->LogicalDevice(), 3, descriptorWrites, 0, nullptr);
}

void EarthSceneRenderer::BuildCommandBuffer(int index)
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

    vector<VkClearValue> clearValues = { { 0.0f, 0.75f, 0.25f, 0.0f }, { 1.0f, 0 } };
    renderPassBeginInfo.pClearValues = clearValues.data();
    vkCmdBeginRenderPass(_commandBuffers.buffers[index], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(_commandBuffers.buffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(_commandBuffers.buffers[index], 0, 1, &_modelResources[0].VertexBuffer().GetBuffer(), offsets);
    vkCmdBindIndexBuffer(_commandBuffers.buffers[index], _modelResources[0].IndexBuffer().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(_commandBuffers.buffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &_descriptorSet, 0, nullptr);
    vkCmdDrawIndexed(_commandBuffers.buffers[index], _modelResources[0].IndicesCount(), 1, 0, 0, 0);

    //viewport.width = swapchain->Extent().width;
    //viewport.height = swapchain->Extent().height;
    //vkCmdSetViewport(_commandBuffers.buffers[index], 0, 1, &viewport);
    //scissorRect.extent.width = (uint32_t)viewport.width;
    //scissorRect.extent.height = (uint32_t)viewport.height;
    //vkCmdSetScissor(_commandBuffers.buffers[index], 0, 1, &scissorRect);

    vkCmdEndRenderPass(_commandBuffers.buffers[index]);
    VK_CHECK_RESULT(vkEndCommandBuffer(_commandBuffers.buffers[index]));
}

void EarthSceneRenderer::RenderImpl()
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

    vector<VkImageView> attachments = { swapchain->ImageViews()[imageIndex], _depthView };
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

void EarthSceneRenderer::UpdateMVP(float elapsedTime)
{
    MVP mvp;
    mvp.model = glm::rotate(mat4(1.0f), glm::radians(elapsedTime * 0.125f), vec3(0.0f, 1.0f, 0.0f));
    mvp.view = glm::lookAt(vec3(0.0f, 0.0f, 32.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    mvp.projection = glm::perspective(glm::radians(90.0f), swapchain->Extent().width / (float)swapchain->Extent().height, 0.125f, 96.0f);
    mvp.projection[1][1] *= -1;
    std::copy((uint8_t*)&mvp, (uint8_t*)&mvp + sizeof(MVP), (uint8_t*)_buffers[0].mapped);
}

#endif