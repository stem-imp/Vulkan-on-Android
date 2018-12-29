#ifdef __ANDROID__

#include "../earth_scene_renderer.h"
#include "../../../vulkan/vulkan_utility.h"
#include "../../../vulkan/renderpass/color_depth_renderpass.h"
#include <unordered_map>
#include <android_native_app_glue.h>

using Vulkan::Instance;
using Vulkan::Surface;
using Vulkan::Device;
using Vulkan::Swapchain;
using Vulkan::RenderPass;
using Vulkan::ColorDepthRenderPass;
using Vulkan::Component;
using std::unordered_map;

static void ProcessNode(aiNode* node, const aiScene* scene);
static void ProcessMesh(aiMesh* mesh, const aiScene* scene);
static void LoadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName);

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
    device = new Device(SelectPhysicalDevice(*instance, *surface, requestedExtNames));
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

    // Load models.
    const aiScene* scene;
    android_app* app = (android_app*)application;
    if (Model::ReadFile(string(app->activity->externalDataPath) + string("/earth/earth.obj"), scene)) {
        ProcessNode(scene->mRootNode, scene);
        _models.emplace_back(*device);
        Vulkan::VertexLayout vertexLayout = vector<Component>{ Component::VERTEX_COMPONENT_POSITION,
                                                               Component::VERTEX_COMPONENT_NORMAL,
                                                               Component::VERTEX_COMPONENT_UV };
        _models[_models.size() - 1].UploadToGPU(scene, vertexLayout, *command);
    }

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
                                                  VK_IMAGE_TILING_OPTIMAL,
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

    VkImageViewCreateInfo imageViewCreateInfo = ImageViewCreateInfo(_depthImage, depthFormat, { depthImageAspectFlags, 0, 1, 0, 1 });
    VK_CHECK_RESULT(vkCreateImageView(d, &imageViewCreateInfo, nullptr, &_depthView));

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


}

EarthSceneRenderer::~EarthSceneRenderer()
{
    DebugLog("~EarthSceneRenderer()");

    VkDevice d = device->LogicalDevice();

    vkDeviceWaitIdle(d);

    _models.clear();

    vkDestroyImageView(d, _depthView, nullptr);
    vkDestroyImage(d, _depthImage, nullptr);
    vkFreeMemory(d, _depthImageMemory, nullptr);

    for (int i = 0; i < swapchain->ConcurrentFramesCount(); i++) {
        vkDestroyFence(d, multiFrameFences[i], nullptr);
        vkDestroySemaphore(d, imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(d, commandsCompleteSemaphores[i], nullptr);
    }

    delete command, command = nullptr;

    framebuffers.clear();

    for (auto& r : renderPasses) {
        delete r;
        r = nullptr;
    }
    renderPasses.clear();

    delete swapchain        , swapchain         = nullptr;
    delete device           , device            = nullptr;
    delete surface          , surface           = nullptr;
    delete layerAndExtension, layerAndExtension = nullptr;
    delete instance         , instance          = nullptr;
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

    viewport.width = swapchain->Extent().width;
    viewport.height = swapchain->Extent().height;
    vkCmdSetViewport(_commandBuffers.buffers[index], 0, 1, &viewport);
    scissorRect.extent.width = (uint32_t)viewport.width;
    scissorRect.extent.height = (uint32_t)viewport.height;
    vkCmdSetScissor(_commandBuffers.buffers[index], 0, 1, &scissorRect);

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

void ProcessNode(aiNode* node, const aiScene* scene)
{
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // the node object only contains indices to index the actual objects in the scene.
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        DebugLog("mesh index %d %s", node->mMeshes[i], mesh->mName.C_Str());
        ProcessMesh(mesh, scene);
    }

    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene);
    }

}

void ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    DebugLog("  vertices num %d", mesh->mNumVertices);
    int tmp = 0;
    for (int j = 0; j < mesh->mNumFaces; j++) {
        tmp += mesh->mFaces[j].mNumIndices;
    }
    DebugLog("  indices num %d", tmp);

    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

#ifndef NDEBUG
    aiReturn res;
    aiColor3D color3;
    int mode;
    float valuef;
    res = material->Get(AI_MATKEY_COLOR_DIFFUSE, color3);
    DebugLog("  %d diffuse (%f, %f, %f)", res, color3.r, color3.g, color3.b);
    res = material->Get(AI_MATKEY_COLOR_SPECULAR, color3);
    DebugLog("  %d specular (%f, %f, %f)", res, color3.r, color3.g, color3.b);
    res = material->Get(AI_MATKEY_COLOR_AMBIENT, color3);
    DebugLog("  %d ambient (%f, %f, %f)", res, color3.r, color3.g, color3.b);
    res = material->Get(AI_MATKEY_COLOR_EMISSIVE, color3);
    DebugLog("  %d emissive (%f, %f, %f)", res, color3.r, color3.g, color3.b);
    res = material->Get(AI_MATKEY_COLOR_TRANSPARENT, color3);
    DebugLog("  %d transparent (%f, %f, %f)", res, color3.r, color3.g, color3.b);
    res = material->Get(AI_MATKEY_ENABLE_WIREFRAME, mode);
    DebugLog("  %d wireframe %d", res, mode);
    res = material->Get(AI_MATKEY_TWOSIDED, mode);
    DebugLog("  %d twosided %d", res, mode);
    res = material->Get(AI_MATKEY_SHADING_MODEL, mode);
    DebugLog("  %d shading model %d", res, mode);
    res = material->Get(AI_MATKEY_BLEND_FUNC, mode);
    DebugLog("  %d blend func %d", res, mode);
    res = material->Get(AI_MATKEY_OPACITY, valuef);
    DebugLog("  %d opacity %f", res, valuef);
    res = material->Get(AI_MATKEY_SHININESS, valuef);
    DebugLog("  %d shininess %f", res, valuef);
    res = material->Get(AI_MATKEY_SHININESS_STRENGTH, valuef);
    DebugLog("  %d shininess strength %f", res, valuef);
    res = material->Get(AI_MATKEY_REFRACTI, valuef);
    DebugLog("  %d refracti %f", res, valuef);
#endif

    LoadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    LoadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    LoadMaterialTextures(material, aiTextureType_AMBIENT, "texture_ambient");
    LoadMaterialTextures(material, aiTextureType_EMISSIVE, "texture_emissive");
    LoadMaterialTextures(material, aiTextureType_HEIGHT, "texture_height");
    LoadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal");
    LoadMaterialTextures(material, aiTextureType_SHININESS, "texture_shininess");
    LoadMaterialTextures(material, aiTextureType_OPACITY, "texture_opacity");
    LoadMaterialTextures(material, aiTextureType_DISPLACEMENT, "texture_displacement");
    LoadMaterialTextures(material, aiTextureType_LIGHTMAP, "texture_lightmap");
    LoadMaterialTextures(material, aiTextureType_REFLECTION, "texture_reflection");
}

void LoadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName)
{
    int textureCount = mat->GetTextureCount(type);
    DebugLog("  %d textures of type %s", textureCount, typeName.c_str());
    for(unsigned int i = 0; i < textureCount; i++)
    {
        aiString str1;
        aiReturn res1;
        res1 = mat->GetTexture(type, i, &str1);
        DebugLog("  %d, %d, %s, PATH, %s",                  i, res1, typeName.c_str(), str1.C_Str());

#ifndef NDEBUG
        aiString str2;
        aiReturn res2, res3, res4, res5, res6, res7, res8, res9, res10;
        float f3;
        int int4, int5, int6, int7, int8, int10;
        aiVector3D v3d9;
        res2 = mat->Get(AI_MATKEY_TEXTURE(type, i), str2);
        res3 = mat->Get(AI_MATKEY_TEXBLEND(type, i), f3);
        res4 = mat->Get(AI_MATKEY_TEXOP(type, i), int4);
        res5 = mat->Get(AI_MATKEY_MAPPING(type, i), int5);
        res6 = mat->Get(AI_MATKEY_UVWSRC(type, i), int6);
        res7 = mat->Get(AI_MATKEY_MAPPINGMODE_U(type, i), int7);
        res8 = mat->Get(AI_MATKEY_MAPPINGMODE_V(type, i), int8);
        res9 = mat->Get(AI_MATKEY_TEXMAP_AXIS(type, i), v3d9);
        res10 = mat->Get(AI_MATKEY_TEXFLAGS(type, i), int10);
        // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture

        DebugLog("  %d, %d, %s, TEXTURE, %s",               i, res2, typeName.c_str(), str2.C_Str());
        DebugLog("  %d, %d, %s, TEXBLEND, %f",              i, res3, typeName.c_str(), f3);
        DebugLog("  %d, %d, %s, TEXOP, %d",                 i, res4, typeName.c_str(), int4);
        DebugLog("  %d, %d, %s, MAPPING, %d",               i, res5, typeName.c_str(), int5);
        DebugLog("  %d, %d, %s, UVWSRC, %d",                i, res6, typeName.c_str(), int6);
        DebugLog("  %d, %d, %s, MAPPINGMODE_U, %d",         i, res7, typeName.c_str(), int7);
        DebugLog("  %d, %d, %s, MAPPINGMODE_V, %d",         i, res8, typeName.c_str(), int8);
        DebugLog("  %d, %d, %s, TEXMAP_AXIS, (%f, %f, %f)", i, res9, typeName.c_str(), v3d9.x, v3d9.y, v3d9.z);
        DebugLog("  %d, %d, %s, TEXFLAGS, %d",              i, res10, typeName.c_str(), int10);
#endif
    }
}

#endif