#include "vulkan_main.hpp"

#include "log.hpp"
#include "stb_image.h"
#include "tiny_obj_loader.h"
#include <gtc/matrix_transform.hpp>
#include <limits>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <chrono>

using AndroidNative::Log;

using std::numeric_limits;
using std::pair;
using std::search;
using std::map;
using std::unordered_map;
using std::chrono::duration;
using std::chrono::high_resolution_clock;
using std::chrono::seconds;
using std::invalid_argument;

using glm::rotate;
using glm::lookAt;
using glm::perspective;
using glm::radians;

using tinyobj::attrib_t;
using tinyobj::shape_t;
using tinyobj::material_t;
using tinyobj::LoadObj;

const int VK_QUEUE_PRESENT_BIT = 0x00000020;

typedef struct SwapchainSupportInfo {
    VkSurfaceCapabilitiesKHR capabilities;
    vector<VkSurfaceFormatKHR> formats;
    vector<VkPresentModeKHR> presentModes;
} SwapchainSupportInfo;

void CreateInstance(InstanceInfo& instanceInfo, const LayerAndExtension& layerAndExtension);
void CreateSurface(InstanceInfo& instanceInfo, ANativeWindow* nativeWindow);
bool SelectPhysicalDevice(InstanceInfo& instanceInfo, LayerAndExtension& layerAndExtension, const vector<int>& targetQueues, vector<const char*> targetExtensionNames);
bool IsDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const vector<int>& targetQueues, LayerAndExtension& layerAndExtension, vector<const char*> deviceExtensionNames, DeviceInfo* deviceInfo = nullptr);
map<int, uint32_t> FindQueueFamiliesIndices(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const vector<int>& targetQueues);
SwapchainSupportInfo QuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
void CreateLogicalDevice(DeviceInfo& deviceInfo, const LayerAndExtension& layerAndExtension);
void CreateSwapChain(const InstanceInfo& instanceInfo, SwapchainInfo& swapchainInfo);
void CreateImageViews(const InstanceInfo& instanceInfo, SwapchainInfo& swapchainInfo);
VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectMask, VkDevice device);
void CreateRenderPass(const InstanceInfo& instanceInfo, const SwapchainInfo& swapchainInfo, VkRenderPass& renderPass);
void CreateDescriptorSetLayout(VkDescriptorSetLayout& descriptorSetLayout, DeviceInfo& deviceInfo);
void CreateGraphicsPipeline(PipelineInfo& pipelineInfo, android_app* app, const DeviceInfo& deviceInfo, SwapchainInfo& swapchainInfo, VkRenderPass& renderPass, VkDescriptorSetLayout& descriptorSetLayout);
void CreateFrameBuffers(const InstanceInfo& instanceInfo, SwapchainInfo& swapchainInfo, VkRenderPass renderPass);
void CreateCommandPool(const DeviceInfo& deviceInfo, VkQueueFlagBits family, CommandInfo& commandInfo);

void CreateDepthBuffer(SwapchainInfo &swapchainInfo, const DeviceInfo &deviceInfo,
                       CommandInfo &commandInfo);
void CreateTextureImage(TextureOject& textureObject, android_app* app, DeviceInfo& deviceInfo, CommandInfo& shortLivedCommandInfo);
void CreateTextureImageView(TextureObject& textureObject, VkDevice device);
void CreateTextureSampler(VkSampler& sampler, VkDevice device);

void LoadModel(const char* filePath, vector<VertexV1>& vertices, vector<uint32_t>& indices, float scale = 1);
void CreateVertexBuffer(vector<VertexV1>& vertices, BufferInfo& bufferInfo, DeviceInfo& deviceInfo, CommandInfo& commandInfo);
void CreateBuffer(DeviceInfo &deviceInfo, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags preferredProperties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
bool MapMemoryTypeToIndex(const VkPhysicalDevice& physicalDevice, uint32_t memoryRequirements, VkMemoryPropertyFlags preferredProperties, uint32_t& memoryIndex);;
void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, DeviceInfo &deviceInfo, CommandInfo& commandInfo);
void CreateIndexBuffer(vector<uint32_t>& indices, BufferInfo& bufferInfo, DeviceInfo& deviceInfo, CommandInfo& commandInfo);

void CreateUniformBuffers(vector<VkBuffer>& uniformBuffers, vector<VkDeviceMemory>& uniformBuffersMemory, DeviceInfo& deviceInfo, SwapchainInfo& swapchainInfo);
void CreateDescriptorPool(VkDescriptorPool& descriptorPool, DeviceInfo& deviceInfo, SwapchainInfo& swapchainInfo);
void CreateDescriptorSets(ResourceDescriptor& descriptor, vector<VkBuffer>& uniformBuffers, TextureObject& textureObject, DeviceInfo& deviceInfo, SwapchainInfo& swapchainInfo);

void CreateSharedGraphicsAndPresentCommandBuffers(SwapchainInfo& swapchainInfo, const DeviceInfo& deviceInfo, CommandInfo& commandInfo, const VkRenderPass& renderPass, const PipelineInfo& pipelineInfo, const BufferInfo& bufferInfo, const vector<uint32_t>& indices, vector<VkDescriptorSet>& descriptorSets);
void CreateSharedGraphicsAndPresentSyncPrimitives(const DeviceInfo& deviceInfo, DrawSyncPrimitives& primitives);

void UpdateMVP(vector<VkDeviceMemory>& mvpMemory, uint32_t currentImageIndex, DeviceInfo& deviceInfo, SwapchainInfo& swapchainInfo);

void DeleteSwapchain(const DeviceInfo& deviceInfo, SwapchainInfo& swapchainInfo);
void DeleteRenderData(const InstanceInfo& instanceInfo, vector<CommandInfo>& commandInfos, PipelineInfo& pipelineInfo, VkRenderPass& renderPass, SwapchainInfo& swapchainInfo);

bool InitVulkan(android_app* app, InstanceInfo& instanceInfo, SwapchainInfo& swapchainInfo, VkRenderPass& renderPass, set<VkCommandPool>& commandPools, vector<CommandInfo>& commandInfos, vector<DrawSyncPrimitives>& primitives, vector<VertexV1>& vertices, vector<uint32_t>& indices, BufferInfo& bufferInfo, PipelineInfo& pipelineInfo, vector<VkBuffer>& uniformBuffers, vector<VkDeviceMemory>& uniformBuffersMemory, ResourceDescriptor& resourceDescriptor, set<VkDescriptorPool>& descriptorPools, TextureOject& textureObject)
{
    if (instanceInfo.initialized) {
        return true;
    }
    if (!InitVulkan()) {
        Log::Error("Vulkan is unavailable, install Vulkan and re-start");
        return false;
    }

    LayerAndExtension layerAndExt;
    vector<const char*> instExtNames = { "VK_KHR_surface", "VK_KHR_android_surface" };
    if (layerAndExt.enableValidationLayers) {
        instExtNames.push_back(LayerAndExtension::DEBUG_EXTENSION_NAME);

        vector<const char*> instLayerNames = {
            LayerAndExtension::GOOGLE_THREADING_LAYER, LayerAndExtension::CORE_VALIDATION_LAYER,
            LayerAndExtension::OBJECT_TRACKER_LAYER, LayerAndExtension::IMAGE_LAYER,
            LayerAndExtension::SWAPCHAIN_LAYER, LayerAndExtension::PARAMETER_VALIDATION_LAYER
        };
        layerAndExt.EnableInstanceLayers(instLayerNames);
    }
    layerAndExt.EnableInstanceExtensions(instExtNames);
    CreateInstance(instanceInfo, layerAndExt);
    if (layerAndExt.enableValidationLayers) {
        layerAndExt.HookDebugReportExtension(instanceInfo.instance);
    }

    CreateSurface(instanceInfo, app->window);

    vector<int> targetQueues = { VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_PRESENT_BIT, VK_QUEUE_TRANSFER_BIT };
    vector<const char*> targetExtNames = { "VK_KHR_swapchain" };
    if (!SelectPhysicalDevice(instanceInfo, layerAndExt, targetQueues, targetExtNames)) {
        targetQueues.erase(targetQueues.begin() + 2);
        if (!SelectPhysicalDevice(instanceInfo, layerAndExt, targetQueues, targetExtNames)) {
            return false;
        }
    }
    layerAndExt.EnableDeviceExtensions(targetExtNames);

    CreateLogicalDevice(instanceInfo.devices[0], layerAndExt);
    CreateSwapChain(instanceInfo, swapchainInfo);
    CreateImageViews(instanceInfo, swapchainInfo);
    CreateRenderPass(instanceInfo, swapchainInfo, renderPass);

    commandPools.clear();
    commandInfos.clear();
    CommandInfo graphicsCommandInfo, transferCommandInfo;
    CreateCommandPool(instanceInfo.devices[0], VK_QUEUE_GRAPHICS_BIT, graphicsCommandInfo);
    commandInfos.push_back(graphicsCommandInfo);
    commandPools.insert(commandInfos[commandInfos.size() - 1].pool);
    CreateCommandPool(instanceInfo.devices[0], VK_QUEUE_TRANSFER_BIT, transferCommandInfo);
    commandInfos.push_back(transferCommandInfo);
    commandPools.insert(commandInfos[commandInfos.size() - 1].pool);

    CreateDepthBuffer(swapchainInfo, instanceInfo.devices[0], commandInfos[0]);
    CreateFrameBuffers(instanceInfo, swapchainInfo, renderPass);

    CreateTextureImage(textureObject, app, instanceInfo.devices[0], commandInfos[1]);
    CreateTextureImageView(textureObject, instanceInfo.devices[0].logicalDevices[0]);
    CreateTextureSampler(textureObject.sampler, instanceInfo.devices[0].logicalDevices[0]);

    resourceDescriptor.layouts.clear();
    resourceDescriptor.layouts.push_back(VkDescriptorSetLayout{});
    VkDescriptorSetLayout& descriptorSetLayout = resourceDescriptor.layouts[0];
    CreateDescriptorSetLayout(descriptorSetLayout, instanceInfo.devices[0]);
    CreateGraphicsPipeline(pipelineInfo, app, instanceInfo.devices[0], swapchainInfo, renderPass, descriptorSetLayout);

    vertices.clear();
    indices.clear();
    LoadModel((string(app->activity->externalDataPath) + string("/angelyy.obj")).c_str(), vertices, indices, 0.0001220703125f);
    CreateVertexBuffer(vertices, bufferInfo, instanceInfo.devices[0], commandInfos[1]);
    CreateIndexBuffer(indices, bufferInfo, instanceInfo.devices[0], commandInfos[1]);
    CreateUniformBuffers(uniformBuffers, uniformBuffersMemory, instanceInfo.devices[0], swapchainInfo);
    CreateDescriptorPool(resourceDescriptor.pool, instanceInfo.devices[0], swapchainInfo);
    descriptorPools.clear();
    descriptorPools.insert(resourceDescriptor.pool);
    CreateDescriptorSets(resourceDescriptor, uniformBuffers, textureObject, instanceInfo.devices[0], swapchainInfo);

    if (instanceInfo.devices[0].sharedGraphicsAndPresentQueue) {
        CreateSharedGraphicsAndPresentCommandBuffers(swapchainInfo, instanceInfo.devices[0], commandInfos[0], renderPass, pipelineInfo, bufferInfo, indices, resourceDescriptor.sets);
        primitives.resize(2);
        for (auto &p : primitives) {
            p.concurrentFrameCount = 2;
        }
        CreateSharedGraphicsAndPresentSyncPrimitives(instanceInfo.devices[0], primitives[0]);
    } else {
        // TODO:  multi queue version.
    }

    instanceInfo.initialized = true;
    return true;
}

void CreateInstance(InstanceInfo& instanceInfo, const LayerAndExtension& layerAndExtension)
{
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .apiVersion = VK_MAKE_VERSION(1, 0, 0),
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .pApplicationName = "Android Vulkan",
        .pEngineName = "vulkan",
    };
    VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = layerAndExtension.InstanceExtensionEnabledCount(),
        .ppEnabledExtensionNames = layerAndExtension.InstanceExtensionNamesEnabled(),
        .enabledLayerCount = layerAndExtension.InstanceLayerEnabledCount(),
        .ppEnabledLayerNames = layerAndExtension.InstanceLayerNamesEnabled(),
    };
    VkResult result;
    result = vkCreateInstance(&instanceCreateInfo, nullptr, &instanceInfo.instance);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkCreateInstance: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
}

void CreateSurface(InstanceInfo& instanceInfo, ANativeWindow* nativeWindow)
{
    VkAndroidSurfaceCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr, .flags = 0, .window = nativeWindow
    };
    VkResult result = (vkCreateAndroidSurfaceKHR(instanceInfo.instance, &createInfo, nullptr, &instanceInfo.surface));
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkCreateAndroidSurfaceKHR: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
}

bool SelectPhysicalDevice(InstanceInfo& instanceInfo, LayerAndExtension& layerAndExtension, const vector<int>& targetQueues, vector<const char*> targetExtensionNames)
{
    // Find one GPU to use:
    // On Android, every GPU device is equal -- supporting
    // graphics/compute/present
    uint32_t gpuCount = 0;
    VkResult result = vkEnumeratePhysicalDevices(instanceInfo.instance, &gpuCount, nullptr);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkEnumeratePhysicalDevices: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
    VkPhysicalDevice tmpGpus[gpuCount];
    result = vkEnumeratePhysicalDevices(instanceInfo.instance, &gpuCount, tmpGpus);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkEnumeratePhysicalDevices: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }

    for (const auto& d : tmpGpus) {
        DeviceInfo devInfo = { .physicalDevice = d };
        if (IsDeviceSuitable(d, instanceInfo.surface, targetQueues, layerAndExtension, targetExtensionNames, &devInfo)) {
            instanceInfo.devices.push_back(devInfo);
        }
    }
    if (instanceInfo.devices.size() == 0) {
        return false;
    }
    return true;
}

map<int, uint32_t> FindUniqueQueueFamiliesIndices(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, vector<VkQueueFamilyProperties> queueFamilies, const vector<int>& targetQueues)
{
    vector<pair<int, vector<int>>> queueFamilyIndices;
    vector<pair<int, vector<int>>>::iterator it;
    for (const auto& target : targetQueues) {
        for (int i = 0; i < queueFamilies.size(); i++) {
            bool find = false;
            if (target == VK_QUEUE_PRESENT_BIT) {
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
                if (queueFamilies[i].queueCount > 0 && presentSupport) {
                    find = true;
                }
            } else if (queueFamilies[i].queueCount > 0 && (queueFamilies[i].queueFlags & target)) {
                find = true;
            }
            if (find) {
                vector<pair<int, vector<int>>> tmp = {{(int) target, {-1}}};
                it = search(queueFamilyIndices.begin(), queueFamilyIndices.end(), tmp.begin(), tmp.end(), [](pair<int, vector<int>> a, pair<int, vector<int>> b) -> bool { return a.first == b.first; });
                if (it != queueFamilyIndices.end()) {
                    it->second.push_back(i);
                } else {
                    queueFamilyIndices.push_back({(int) target, {i}});
                }
            }
        }
    }

    ////////////////////////////////
    map<int, const char*> QUEUE_NAMES = {
        { VK_QUEUE_GRAPHICS_BIT, "VK_QUEUE_GRAPHICS_BIT"},
        { VK_QUEUE_COMPUTE_BIT, "VK_QUEUE_COMPUTE_BIT"},
        { VK_QUEUE_TRANSFER_BIT, "VK_QUEUE_TRANSFER_BIT"},
        { VK_QUEUE_SPARSE_BINDING_BIT, "VK_QUEUE_SPARSE_BINDING_BIT"},
        { 16, "VK_QUEUE_PROTECTED_BIT"},
        { VK_QUEUE_PRESENT_BIT, "VK_QUEUE_PRESENT_BIT"},
    };
    for (auto it = queueFamilyIndices.begin(); it != queueFamilyIndices.end(); it++) {
        DebugLog("%s", QUEUE_NAMES[it->first]);

        for (auto it1 = it->second.begin(); it1 != it->second.end(); it1++) {
            DebugLog("  family index %d", *it1);
        }
    }
    ////////////////////////////////

    map<int, uint32_t> result;
    while (queueFamilyIndices.size() > 0) {

        int minIndicesQueueIdx = 0;
        size_t minIndicesCount = queueFamilyIndices[0].second.size();
        for (int i = 1; i < queueFamilyIndices.size(); i++) {
            if (queueFamilyIndices[i].second.size() < queueFamilyIndices[minIndicesQueueIdx].second.size()) {
                minIndicesQueueIdx = i;
                minIndicesCount = queueFamilyIndices[i].second.size();
            }
        }
        int index = queueFamilyIndices[minIndicesQueueIdx].second.front();
        DebugLog("%s at %d", QUEUE_NAMES[queueFamilyIndices[minIndicesQueueIdx].first], index);
        result.insert({queueFamilyIndices[minIndicesQueueIdx].first, index});
        queueFamilyIndices.erase(queueFamilyIndices.begin() + minIndicesQueueIdx);
        for (auto it1 = queueFamilyIndices.begin(); it1 != queueFamilyIndices.end(); it1++) {
            if (it1->second.size() <= 1) {
                continue;
            }
            for (auto it2 = it1->second.begin(); it2 != it1->second.end(); ) {
                if (*it2 == index) {
                    it2 = it1->second.erase(it2);
                } else {
                    it2++;
                }
            }
        }
    }

    return result;
}

map<int, uint32_t> FindQueueFamiliesIndices(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const vector<int>& targetQueues)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
    /*queueFamilies.clear();
    queueFamilies.push_back(VkQueueFamilyProperties{.queueCount = 1, .queueFlags = (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT)});
    queueFamilies.push_back(VkQueueFamilyProperties{.queueCount = 1, .queueFlags = (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT)});
    queueFamilies.push_back(VkQueueFamilyProperties{.queueCount = 1, .queueFlags = (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT)});*/
    map<int, uint32_t> result = FindUniqueQueueFamiliesIndices(physicalDevice, surface, queueFamilies, targetQueues);

    return result;
}

bool IsDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const vector<int>& targetQueues, LayerAndExtension& layerAndExtension, vector<const char*> deviceExtensionNames, DeviceInfo* deviceInfo)
{
    map<int, uint32_t> result = FindQueueFamiliesIndices(physicalDevice, surface, targetQueues);
    if (result.size() != targetQueues.size()) {
        return  false;
    }
    layerAndExtension.EnumerateDeviceLayersAndExtensions(physicalDevice);

    bool ok = true;
    for (const auto& name : deviceExtensionNames) {
        ok = ok && layerAndExtension.IsDeviceExtensionSupported(name);
        if (!ok) {
            return false;
        }
    }

    SwapchainSupportInfo supportInfo = QuerySwapchainSupport(physicalDevice, surface);
    if (supportInfo.formats.empty() || supportInfo.presentModes.empty()) {
        return false;
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);
    if (!supportedFeatures.samplerAnisotropy) {
        return false;
    }

    if (deviceInfo != nullptr) {
        deviceInfo->familyToIndex = result;
        int graphicsIndex = result[(int)VK_QUEUE_GRAPHICS_BIT];
        int presentIndex = result[VK_QUEUE_PRESENT_BIT];
        deviceInfo->sharedGraphicsAndPresentQueue = (graphicsIndex == presentIndex);

        for (auto it = result.cbegin(); it != result.cend(); it++) {
            if (deviceInfo->indexToFamily.count(it->second) > 0) {
                deviceInfo->indexToFamily[it->second].insert(it->first);
            } else {
                deviceInfo->indexToFamily[it->second] = { it->first };
            }
        }
    }
    return true;
}

SwapchainSupportInfo QuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    SwapchainSupportInfo info;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &info.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        info.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, info.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        info.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, info.presentModes.data());
    }

    return info;
}

void CreateLogicalDevice(DeviceInfo& deviceInfo, const LayerAndExtension& layerAndExtension)
{
    vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    //float queuePriority = 1.0f;
    map<int, VkDeviceQueueCreateInfo> createInfos;
    vector<float> queuePriorities(deviceInfo.familyToIndex.size(), 0.5f);
    float* queuePrioritiesBase = queuePriorities.data();
    for (auto it = deviceInfo.indexToFamily.cbegin(); it != deviceInfo.indexToFamily.cend(); it++) {
        //vector<float> queuePriorities(it->second.size(), 0.5f);
        VkDeviceQueueCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        createInfo.queueFamilyIndex = it->first;
        createInfo.queueCount = it->second.size();
        createInfo.pQueuePriorities = queuePrioritiesBase;//queuePriorities.data();
        createInfos[it->first] = createInfo;
        queueCreateInfos.push_back(createInfo);
        queuePrioritiesBase += createInfo.queueCount;
    }

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = layerAndExtension.DeviceExtensionEnabledCount();
    createInfo.ppEnabledExtensionNames = layerAndExtension.DeviceExtensionNamesEnabled();
    VkPhysicalDeviceFeatures eabledFeatures = { .samplerAnisotropy = VK_TRUE };
    createInfo.pEnabledFeatures = &eabledFeatures;

    VkDevice device;
    VkResult result = vkCreateDevice(deviceInfo.physicalDevice, &createInfo, nullptr, &device);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkCreateDevice: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
    deviceInfo.logicalDevices.push_back(device);
    deviceInfo.queues[device] = map<int, vector<QueueInfo>>{};
    for (auto it = deviceInfo.familyToIndex.cbegin(); it != deviceInfo.familyToIndex.cend(); it++) {
        uint32_t i = 0;
        deviceInfo.queues[device][it->first] = {};
        for (auto it1 = deviceInfo.indexToFamily.cbegin(); it1 != deviceInfo.indexToFamily.cend(); it1++, i++) {
            QueueInfo tmp = { .queue = VkQueue{}, .index = i };
            vkGetDeviceQueue(device, it->second, tmp.index, &tmp.queue);
            deviceInfo.queues[device][it->first].push_back(tmp);
        }
    }
}

VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const vector<VkSurfaceFormatKHR>& availableFormats, VkFormat targetFormat, VkColorSpaceKHR targetColorSpace)
{
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
        return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == targetFormat && availableFormat.colorSpace == targetColorSpace) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR ChooseSwapchainPresentMode(const vector<VkPresentModeKHR> availablePresentModes)
{
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }/* else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            bestMode = availablePresentMode;
        }*/
    }

    return bestMode;
}

VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, const InstanceInfo& instanceInfo)
{
    if (capabilities.currentExtent.width != numeric_limits<uint32_t>::max() && !instanceInfo.resolutionChanged) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = { static_cast<uint32_t>(instanceInfo.width), static_cast<uint32_t>(instanceInfo.height) };

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

void CreateSwapChain(const InstanceInfo& instanceInfo, SwapchainInfo& swapchainInfo)
{
    const DeviceInfo& deviceInfo = instanceInfo.devices[0];
    SwapchainSupportInfo supportInfo = QuerySwapchainSupport(deviceInfo.physicalDevice, instanceInfo.surface);
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(supportInfo.formats, VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);
    VkPresentModeKHR presentMode = ChooseSwapchainPresentMode(supportInfo.presentModes);
    VkExtent2D extent = ChooseSwapExtent(supportInfo.capabilities, instanceInfo);

    swapchainInfo.extent = extent;
    swapchainInfo.format = surfaceFormat.format;

    uint32_t imageCount = supportInfo.capabilities.minImageCount + 1;
    if (supportInfo.capabilities.maxImageCount > 0 && imageCount > supportInfo.capabilities.maxImageCount) {
        imageCount = supportInfo.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = instanceInfo.surface;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = extent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t graphicsIndex = deviceInfo.familyToIndex.at((int)VK_QUEUE_GRAPHICS_BIT);
    if (!deviceInfo.sharedGraphicsAndPresentQueue) {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        uint32_t presentIndex = deviceInfo.familyToIndex.at(VK_QUEUE_PRESENT_BIT);
        uint32_t queueFamilyIndices[] = { graphicsIndex, presentIndex };
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 1;
        swapchainCreateInfo.pQueueFamilyIndices = &graphicsIndex;
    }

    if (supportInfo.capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        swapchainCreateInfo.preTransform = supportInfo.capabilities.currentTransform;
    }

    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR; // My testing device(Google Pixel XL) only supports this flag.
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
    VkResult result = vkCreateSwapchainKHR(deviceInfo.logicalDevices[0], &swapchainCreateInfo, nullptr, &swapchainInfo.swapchain);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkCreateSwapchainKHR: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
}

void RecreateSwapchain(android_app* app, const InstanceInfo& instanceInfo, vector<CommandInfo>& commandInfos, PipelineInfo& pipelineInfo, VkRenderPass& renderPass, SwapchainInfo& swapchainInfo, const BufferInfo& bufferInfo, const vector<uint32_t>& indices, ResourceDescriptor& resourceDescriptor)
{
    DeleteRenderData(instanceInfo, commandInfos, pipelineInfo, renderPass, swapchainInfo);
    CreateSwapChain(instanceInfo, swapchainInfo);
    CreateImageViews(instanceInfo, swapchainInfo);
    CreateRenderPass(instanceInfo, swapchainInfo, renderPass);
    CreateGraphicsPipeline(pipelineInfo, app, instanceInfo.devices[0], swapchainInfo, renderPass, resourceDescriptor.layouts[0]);
    CreateDepthBuffer(swapchainInfo, instanceInfo.devices[0], commandInfos[0]);
    CreateFrameBuffers(instanceInfo, swapchainInfo, renderPass);
    CreateSharedGraphicsAndPresentCommandBuffers(swapchainInfo, instanceInfo.devices[0], commandInfos[0], renderPass, pipelineInfo, bufferInfo, indices, resourceDescriptor.sets);
}

void CreateImageViews(const InstanceInfo& instanceInfo, SwapchainInfo& swapchainInfo)
{
    uint32_t swapChainImagesCount = 0;
    VkDevice device = instanceInfo.devices[0].logicalDevices[0];
    VkResult result = vkGetSwapchainImagesKHR(device, swapchainInfo.swapchain, &swapChainImagesCount, nullptr);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkGetSwapchainImagesKHR: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
    swapchainInfo.images.resize(swapChainImagesCount);
    result = vkGetSwapchainImagesKHR(device, swapchainInfo.swapchain, &swapChainImagesCount, swapchainInfo.images.data());
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkGetSwapchainImagesKHR: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }

    swapchainInfo.views.resize(swapChainImagesCount);
    for (uint32_t i = 0; i < swapChainImagesCount; i++) {
        /*VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = swapchainInfo.images[i];
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = swapchainInfo.format;
        viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = 1;
        result = vkCreateImageView(device, &viewCreateInfo, nullptr, &swapchainInfo.views[i]);
        if (result != VK_SUCCESS) {
            string code = to_string(result);
            throw runtime_error("vkCreateImageView: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
        }*/
        swapchainInfo.views[i] = CreateImageView(swapchainInfo.images[i], swapchainInfo.format, VK_IMAGE_ASPECT_COLOR_BIT, instanceInfo.devices[0].logicalDevices[0]);
    }
}

VkFormat FindSupportedFormat(const vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice)
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if ((tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) ||
            (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)) {
            return format;
        }
    }

    string tilingCode = to_string(tiling);
    string featureMask = to_string(features);
    throw runtime_error("Failed to find supported format for tiling[" + tilingCode + "] and feature mask[" + featureMask + "].");
}

void CreateRenderPass(const InstanceInfo& instanceInfo, const SwapchainInfo& swapchainInfo, VkRenderPass& renderPass)
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapchainInfo.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    const DeviceInfo& deviceInfo = instanceInfo.devices[0];
    if (deviceInfo.sharedGraphicsAndPresentQueue) {
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    } else {
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = FindSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, deviceInfo.physicalDevice);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colourReference = {};
    colourReference.attachment = 0;
    colourReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthReference = {};
    depthReference.attachment = 1;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colourReference;
    subpassDescription.pDepthStencilAttachment = &depthReference;

    VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = 2;
    createInfo.pAttachments = attachments;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpassDescription;

    if (deviceInfo.sharedGraphicsAndPresentQueue) {
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        // .srcStageMask needs to be a part of pWaitDstStageMask in the WSI semaphore.
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dependencyFlags = 0;

        createInfo.dependencyCount = 1;
        createInfo.pDependencies = &dependency;
    }

    VkResult result = vkCreateRenderPass(deviceInfo.logicalDevices[0], &createInfo, nullptr, &renderPass);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkCreateRenderPass: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
}

VkShaderModule LoadShaderFromFile(const char* filePath, android_app* app, const DeviceInfo& deviceInfo)
{
    AAsset* file = AAssetManager_open(app->activity->assetManager, filePath, AASSET_MODE_BUFFER);
    size_t fileLength = AAsset_getLength(file);
    vector<char> fileContent(fileLength, 0);
    AAsset_read(file, fileContent.data(), fileLength);
    AAsset_close(file);

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = fileLength;
    createInfo.pCode = (const uint32_t*)fileContent.data();
    VkDevice device = deviceInfo.logicalDevices[0];
    VkShaderModule shader;
    VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &shader);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkCreateShaderModule: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
    return shader;
}

void CreateGraphicsPipeline(PipelineInfo& pipelineInfo, android_app* app, const DeviceInfo& deviceInfo, SwapchainInfo& swapchainInfo, VkRenderPass& renderPass, VkDescriptorSetLayout& descriptorSetLayout)
{
    // Create shader stage.
    VkShaderModule vertexShader = LoadShaderFromFile("shaders/triangle.vert.spv", app, deviceInfo);
    VkShaderModule fragmentShader = LoadShaderFromFile("shaders/triangle.frag.spv", app, deviceInfo);
    VkPipelineShaderStageCreateInfo shaderStages[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertexShader,
            .pName = "main",
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragmentShader,
            .pName = "main",
        }
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    auto bindingDescription = VertexV1::GetBindingDescription(0);
    auto attributeDescriptions = VertexV1::GetAttributeDescriptions(0);
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // pTessellationState  ignored.

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapchainInfo.extent.width;
    viewport.height = (float) swapchainInfo.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = swapchainInfo.extent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.sampleShadingEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // pDynamicState ignored.

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    VkDevice device = deviceInfo.logicalDevices[0];
    VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineInfo.layout);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkCreatePipelineLayout: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pRasterizationState = &rasterizer;
    pipelineCreateInfo.pMultisampleState = &multisampling;
    pipelineCreateInfo.pDepthStencilState = &depthStencil;
    pipelineCreateInfo.pColorBlendState = &colorBlending;
    pipelineCreateInfo.layout = pipelineInfo.layout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

    result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipelineInfo.pipeline);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkCreateGraphicsPipelines: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }

    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragmentShader, nullptr);
}

void CreateFrameBuffers(const InstanceInfo& instanceInfo, SwapchainInfo& swapchainInfo, VkRenderPass renderPass)
{
    size_t size = swapchainInfo.images.size();
    swapchainInfo.framebuffers.resize(size);
    for (size_t i = 0; i < size; i++) {
        VkImageView attachments[] = {
            swapchainInfo.views[i], swapchainInfo.depthImageInfo[i].view
        };
        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = 2;
        createInfo.pAttachments = attachments;
        createInfo.width = static_cast<uint32_t>(swapchainInfo.extent.width);
        createInfo.height = static_cast<uint32_t>(swapchainInfo.extent.height);
        createInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(instanceInfo.devices[0].logicalDevices[0], &createInfo, nullptr, &swapchainInfo.framebuffers[i]);
        if (result != VK_SUCCESS) {
            string code = to_string(result);
            throw runtime_error("vkCreateFramebuffer: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
        }
    }
}

void CreateCommandPool(const DeviceInfo& deviceInfo, VkQueueFlagBits family, CommandInfo& commandInfo)
{
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    switch (family) {
        case VK_QUEUE_GRAPHICS_BIT:
            createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            createInfo.queueFamilyIndex = deviceInfo.familyToIndex.at((int)family);
            break;
        case VK_QUEUE_TRANSFER_BIT:
            createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            if (deviceInfo.familyToIndex.find((int)family) != deviceInfo.familyToIndex.end()) {
                createInfo.queueFamilyIndex = deviceInfo.familyToIndex.at((int)family);
            } else {
                createInfo.queueFamilyIndex = deviceInfo.familyToIndex.at((int)VK_QUEUE_GRAPHICS_BIT);
            }
            break;
        default:
            break;
    }

    VkResult result = vkCreateCommandPool(deviceInfo.logicalDevices[0], &createInfo, nullptr, &commandInfo.pool);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkCreateCommandPool: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
}

VkCommandBuffer BeginOneTimeCommands(CommandInfo& commandInfo, const DeviceInfo& deviceInfo)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandInfo.pool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    VkDevice device = deviceInfo.logicalDevices[0];
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void EndOneTimeCommands(VkCommandBuffer commandBuffer, const DeviceInfo& deviceInfo, CommandInfo& commandInfo)
{
    /*vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkQueue queue;
    VkDevice device = deviceInfo.logicalDevices[0];
    if (deviceInfo.queues[device].find((int)VK_QUEUE_TRANSFER_BIT) != deviceInfo.queues[device].end()) {
        queue = deviceInfo.queues[device][(int)VK_QUEUE_TRANSFER_BIT][0].queue;
    } else {
        queue = deviceInfo.queues[device][(int)VK_QUEUE_GRAPHICS_BIT][0].queue;
    }
    VkFence fence;
    VkFenceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkResult result = vkCreateFence(device, &createInfo, nullptr, &fence);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkCreateFence: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
    vkQueueSubmit(queue, 1, &submitInfo, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, numeric_limits<uint64_t>::max());

    vkFreeCommandBuffers(device, commandInfo.pool, 1, &commandBuffer);*/
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkQueue queue;
    VkDevice device = deviceInfo.logicalDevices[0];
    if (deviceInfo.queues.at(device).find((int)VK_QUEUE_TRANSFER_BIT) != deviceInfo.queues.at(device).end()) {
        queue = deviceInfo.queues.at(device).at((int)VK_QUEUE_TRANSFER_BIT)[0].queue;
    } else {
        queue = deviceInfo.queues.at(device).at((int)VK_QUEUE_GRAPHICS_BIT)[0].queue;
    }
    VkFence fence;
    VkFenceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkResult result = vkCreateFence(device, &createInfo, nullptr, &fence);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkCreateFence: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
    vkQueueSubmit(queue, 1, &submitInfo, fence);
    result = vkWaitForFences(device, 1, &fence, VK_TRUE, numeric_limits<uint64_t>::max());
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkWaitForFences: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
    vkDestroyFence(device, fence, nullptr);

    vkFreeCommandBuffers(device, commandInfo.pool, 1, &commandBuffer);
}

void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, CommandInfo& commandInfo, const DeviceInfo& deviceInfo)
{
    VkCommandBuffer commandBuffer = BeginOneTimeCommands(commandInfo, deviceInfo);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    //barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        throw invalid_argument("Unsupported layout transition.");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    EndOneTimeCommands(commandBuffer, deviceInfo, commandInfo);
}

void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, const DeviceInfo& deviceInfo)
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkDevice device = deviceInfo.logicalDevices[0];
    VkResult result = vkCreateImage(device, &imageInfo, nullptr, &image);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkCreateImage: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    MapMemoryTypeToIndex(deviceInfo.physicalDevice, memRequirements.memoryTypeBits, properties, allocInfo.memoryTypeIndex);

    result = vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkAllocateMemory: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, CommandInfo& shortLivedCommandInfo, DeviceInfo& deviceInfo)
{
    VkCommandBuffer commandBuffer = BeginOneTimeCommands(shortLivedCommandInfo, deviceInfo);

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    EndOneTimeCommands(commandBuffer, deviceInfo, shortLivedCommandInfo);
}

void CreateTextureImage(TextureOject& textureObject, android_app* app, DeviceInfo& deviceInfo, CommandInfo& shortLivedCommandInfo)
{
    /*VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(deviceInfo.physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &props);
    if (!((props.linearTilingFeatures | props.optimalTilingFeatures) & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
        throw runtime_error("Images created will not support format VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT");
    }*/
    VkFormat format = FindSupportedFormat({ VK_FORMAT_R8G8B8A8_UNORM }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT, deviceInfo.physicalDevice);

    // Load image and dump data to the staging buffer.
    AAsset* file = AAssetManager_open(app->activity->assetManager, "textures/angelyy01.jpg", AASSET_MODE_BUFFER);
    size_t fileLength = AAsset_getLength(file);
    vector<stbi_uc> fileContent(fileLength, 0);
    AAsset_read(file, fileContent.data(), fileLength);
    AAsset_close(file);

    uint32_t channelsInFile;
    unsigned char* imageData = stbi_load_from_memory(fileContent.data(), fileLength, reinterpret_cast<int*>(&textureObject.tex_width), reinterpret_cast<int*>(&textureObject.tex_height), reinterpret_cast<int*>(&channelsInFile), STBI_rgb_alpha);
    VkDeviceSize imageSize = textureObject.tex_width * textureObject.tex_height * 4;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkDevice device = deviceInfo.logicalDevices[0];
    CreateBuffer(deviceInfo, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, imageData, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    stbi_image_free(imageData);

    CreateImage(textureObject.tex_width, textureObject.tex_height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureObject.imageInfo.image, textureObject.imageInfo.memory, deviceInfo);
    TransitionImageLayout(textureObject.imageInfo.image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, shortLivedCommandInfo, deviceInfo);
    CopyBufferToImage(stagingBuffer, textureObject.imageInfo.image, static_cast<uint32_t>(textureObject.tex_width), static_cast<uint32_t>(textureObject.tex_height), shortLivedCommandInfo, deviceInfo);
    TransitionImageLayout(textureObject.imageInfo.image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, shortLivedCommandInfo, deviceInfo);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectMask, VkDevice device)
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    VkResult result = vkCreateImageView(device, &viewInfo, nullptr, &imageView);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkCreateImageView: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }

    return imageView;
}

void CreateTextureImageView(TextureObject& textureObject, VkDevice device)
{
    textureObject.imageInfo.view = CreateImageView(textureObject.imageInfo.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, device);
}

void CreateTextureSampler(VkSampler& sampler, VkDevice device)
{
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    VkResult result = vkCreateSampler(device, &samplerInfo, nullptr, &sampler);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkCreateSampler: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
}

void CreateVertexBuffer(vector<VertexV1>& vertices, BufferInfo& bufferInfo, DeviceInfo& deviceInfo, CommandInfo& commandInfo)
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(deviceInfo, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    VkDevice device = deviceInfo.logicalDevices[0];
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    CreateBuffer(deviceInfo, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bufferInfo.vertexBuffer, bufferInfo.vertexBufferMemory);

    CopyBuffer(stagingBuffer, bufferInfo.vertexBuffer, bufferSize, deviceInfo, commandInfo);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void CreateIndexBuffer(vector<uint32_t>& indices, BufferInfo& bufferInfo, DeviceInfo& deviceInfo, CommandInfo& commandInfo)
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(deviceInfo, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    VkDevice device = deviceInfo.logicalDevices[0];
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    CreateBuffer(deviceInfo, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bufferInfo.indexBuffer, bufferInfo.indexBufferMemory);

    CopyBuffer(stagingBuffer, bufferInfo.indexBuffer, bufferSize, deviceInfo, commandInfo);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void CreateBuffer(DeviceInfo &deviceInfo, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags preferredProperties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkDevice device = deviceInfo.logicalDevices[0];
    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkCreateBuffer: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    MapMemoryTypeToIndex(deviceInfo.physicalDevice, memRequirements.memoryTypeBits, preferredProperties, allocInfo.memoryTypeIndex);

    result = vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkAllocateMemory: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

bool MapMemoryTypeToIndex(const VkPhysicalDevice& physicalDevice, uint32_t memoryRequirements, VkMemoryPropertyFlags preferredProperties, uint32_t& memoryIndex)
{
    VkPhysicalDeviceMemoryProperties devMemProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &devMemProperties);

    for (uint32_t i = 0; i < devMemProperties.memoryTypeCount; i++) {
        if ((memoryRequirements & (1 << i)) && (devMemProperties.memoryTypes[i].propertyFlags & preferredProperties) == preferredProperties) {
            memoryIndex = i;
            return true;
        }
    }
    return false;
}

void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, DeviceInfo &deviceInfo, CommandInfo& shortLivedCommandInfo)
{
    /*VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = shortLivedCommandInfo.pool;
    allocInfo.commandBufferCount = 1;

    VkDevice device = deviceInfo.logicalDevices[0];
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);*/
    VkCommandBuffer commandBuffer = BeginOneTimeCommands(shortLivedCommandInfo, deviceInfo);

    VkBufferCopy copyRegion = {};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    /*vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkQueue queue;
    if (deviceInfo.queues[device].find((int)VK_QUEUE_TRANSFER_BIT) != deviceInfo.queues[device].end()) {
        queue = deviceInfo.queues[device][(int)VK_QUEUE_TRANSFER_BIT][0].queue;
    } else {
        queue = deviceInfo.queues[device][(int)VK_QUEUE_GRAPHICS_BIT][0].queue;
    }
    VkFence fence;
    VkFenceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkResult result = vkCreateFence(device, &createInfo, nullptr, &fence);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkCreateFence: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
    vkQueueSubmit(queue, 1, &submitInfo, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, numeric_limits<uint64_t>::max());

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);*/
    EndOneTimeCommands(commandBuffer, deviceInfo, shortLivedCommandInfo);
}

void CreateSharedGraphicsAndPresentCommandBuffers(SwapchainInfo& swapchainInfo, const DeviceInfo& deviceInfo, CommandInfo& commandInfo, const VkRenderPass& renderPass, const PipelineInfo& pipelineInfo, const BufferInfo& bufferInfo, const vector<uint32_t>& indices, vector<VkDescriptorSet>& descriptorSets)
{
    uint32_t size = swapchainInfo.images.size();
    commandInfo.buffer.resize(size);
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandInfo.pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = size;
    VkResult result = vkAllocateCommandBuffers(deviceInfo.logicalDevices[0], &allocInfo, commandInfo.buffer.data());
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkAllocateCommandBuffers: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }

    VkClearValue clearValues[] = { {}, {} };
    clearValues[0].color.float32[0] = 0.0f;
    clearValues[0].color.float32[1] = 0.34f;
    clearValues[0].color.float32[2] = 0.9f;
    clearValues[0].color.float32[3] = 1.0f;
    clearValues[1].depthStencil = { 1.0f, 0 };
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    for (uint32_t i = 0; i < size; i++) {
        result = vkBeginCommandBuffer(commandInfo.buffer[i], &beginInfo);
        if (result != VK_SUCCESS) {
            string code = to_string(result);
            throw runtime_error("vkBeginCommandBuffer: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
        }

        VkRenderPassBeginInfo renderPassBeginInfo;
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = nullptr;
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.framebuffer = swapchainInfo.framebuffers[i];
        renderPassBeginInfo.renderArea.offset = { 0, 0 };
        renderPassBeginInfo.renderArea.extent = swapchainInfo.extent;
        renderPassBeginInfo.clearValueCount = 2;
        renderPassBeginInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(commandInfo.buffer[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandInfo.buffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineInfo.pipeline);

        VkBuffer vertexBuffers[] = { bufferInfo.vertexBuffer };
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandInfo.buffer[i], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandInfo.buffer[i], bufferInfo.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandInfo.buffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineInfo.layout, 0, 1, &descriptorSets[i], 0, nullptr);

        vkCmdDrawIndexed(commandInfo.buffer[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandInfo.buffer[i]);

        result = vkEndCommandBuffer(commandInfo.buffer[i]);
        if (result != VK_SUCCESS) {
            string code = to_string(result);
            throw runtime_error("vkEndCommandBuffer: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
        }
    }
}

void CreateSharedGraphicsAndPresentSyncPrimitives(const DeviceInfo& deviceInfo, DrawSyncPrimitives& primitives)
{
    primitives.imageAvailableSemaphores.resize(primitives.concurrentFrameCount);
    primitives.renderFinishedSemaphores.resize(primitives.concurrentFrameCount);
    primitives.inFlightFences.resize(primitives.concurrentFrameCount);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkDevice device = deviceInfo.logicalDevices[0];
    for (size_t i = 0; i < primitives.concurrentFrameCount; i++) {
        VkResult result1 = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &primitives.imageAvailableSemaphores[i]);
        VkResult result2 = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &primitives.renderFinishedSemaphores[i]);
        VkResult result3 = vkCreateFence(device, &fenceInfo, nullptr, &primitives.inFlightFences[i]);
        if (result1 != VK_SUCCESS || result2 != VK_SUCCESS || result3 != VK_SUCCESS) {
            string code1 = to_string(result1);
            string code2 = to_string(result2);
            string code3 = to_string(result3);
            throw runtime_error(
                "vkCreateSemaphore: code[" + code1 + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]\n" +
                "vkCreateSemaphore: code[" + code2 + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]\n" +
                "vkCreateFence: code[" + code3 + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]"
            );
        }
    }
}

VkResult DrawFrame(android_app* app, InstanceInfo& instanceInfo, SwapchainInfo& swapchainInfo, VkRenderPass& renderPass, vector<CommandInfo>& commandInfos, PipelineInfo& pipelineInfo, vector<DrawSyncPrimitives>& primitives, const BufferInfo& bufferInfo, const vector<uint32_t>& indices, vector<VkDeviceMemory>& mvpMemory, ResourceDescriptor& resourceDescriptor)
{
    DeviceInfo& deviceInfo = instanceInfo.devices[0];
    VkDevice device = deviceInfo.logicalDevices[0];
    if (deviceInfo.sharedGraphicsAndPresentQueue) {
        DrawSyncPrimitives sharedPrimitive = primitives[0];
        vkWaitForFences(device, 1, &sharedPrimitive.inFlightFences[sharedPrimitive.currentFrame], VK_TRUE, numeric_limits<uint64_t>::max());

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapchainInfo.swapchain, numeric_limits<uint64_t>::max(), sharedPrimitive.imageAvailableSemaphores[sharedPrimitive.currentFrame], VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            RecreateSwapchain(app, instanceInfo, commandInfos, pipelineInfo, renderPass, swapchainInfo, bufferInfo, indices, resourceDescriptor);
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            return result;
        }

        vkResetFences(device, 1, &sharedPrimitive.inFlightFences[sharedPrimitive.currentFrame]);

        UpdateMVP(mvpMemory, imageIndex, instanceInfo.devices[0], swapchainInfo);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSemaphores[] = { sharedPrimitive.imageAvailableSemaphores[sharedPrimitive.currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        CommandInfo& commandInfo = commandInfos[0];
        submitInfo.pCommandBuffers = &commandInfo.buffer[imageIndex];
        VkSemaphore signalSemaphores[] = { sharedPrimitive.renderFinishedSemaphores[sharedPrimitive.currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        VkQueue graphicsQueue = deviceInfo.queues[device][(int)VK_QUEUE_GRAPHICS_BIT][0].queue;
        result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, sharedPrimitive.inFlightFences[sharedPrimitive.currentFrame]);
        if (result != VK_SUCCESS) {
            string code = to_string(result);
            throw runtime_error("vkQueueSubmit: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
        }

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapChains[] = { swapchainInfo.swapchain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        VkQueue presentQueue = deviceInfo.queues[device][VK_QUEUE_PRESENT_BIT][0].queue;
        result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || instanceInfo.resolutionChanged) {
            string code = to_string(result);
            DebugLog("vkQueuePresentKHR: code[%d], file[%s], line[%d]", result, __FILE__, __LINE__);
            RecreateSwapchain(app, instanceInfo, commandInfos, pipelineInfo, renderPass, swapchainInfo, bufferInfo, indices, resourceDescriptor);
            instanceInfo.resolutionChanged = false;
        } else if (result != VK_SUCCESS) {
            string code = to_string(result);
            throw runtime_error("vkQueuePresentKHR: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
        }

        sharedPrimitive.currentFrame = (sharedPrimitive.currentFrame + 1) % sharedPrimitive.concurrentFrameCount;
    } else {
        // TODO: multi queue version.
    }
    return VK_SUCCESS;
}

void DeleteVulkan(InstanceInfo& instanceInfo, set<VkCommandPool>& commandPools, vector<CommandInfo>& commandInfos, PipelineInfo& pipelineInfo, VkRenderPass& renderPass, SwapchainInfo& swapchainInfo, vector<DrawSyncPrimitives>& primitives, BufferInfo& bufferInfo, vector<VkBuffer>& uniformBuffers, vector<VkDeviceMemory>& uniformBuffersMemory, ResourceDescriptor& resourceDescriptor, set<VkDescriptorPool>& descriptorPools, TextureOject& textureOject)
{
    DeviceInfo deviceInfo = instanceInfo.devices[0];
    VkDevice device = deviceInfo.logicalDevices[0];

    //////////////////////////////
    DeleteRenderData(instanceInfo, commandInfos, pipelineInfo, renderPass, swapchainInfo);
    ////////////////////////////////

    vkDestroySampler(device, textureOject.sampler, nullptr);
    vkDestroyImageView(device, textureOject.imageInfo.view, nullptr);

    vkDestroyImage(device, textureOject.imageInfo.image, nullptr);
    vkFreeMemory(device, textureOject.imageInfo.memory, nullptr);

    for (auto& l : resourceDescriptor.layouts) {
        vkDestroyDescriptorSetLayout(device, l, nullptr);
    }
    for (auto& p : descriptorPools) {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    for (size_t i = 0; i < swapchainInfo.images.size(); i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    vkDestroyBuffer(device, bufferInfo.indexBuffer, nullptr);
    vkFreeMemory(device, bufferInfo.indexBufferMemory, nullptr);

    vkDestroyBuffer(device, bufferInfo.vertexBuffer, nullptr);
    vkFreeMemory(device, bufferInfo.vertexBufferMemory, nullptr);

    for (size_t i = 0; i < primitives.size(); i++) {
        for (size_t j = 0; j < primitives[i].concurrentFrameCount; j++) {
            if (primitives[i].renderFinishedSemaphores.size() == 0) {
                break;
            }
            vkDestroySemaphore(device, primitives[i].renderFinishedSemaphores[j], nullptr);
            vkDestroySemaphore(device, primitives[i].imageAvailableSemaphores[j], nullptr);
            vkDestroyFence(device, primitives[i].inFlightFences[j], nullptr);
        }
    }

    for (auto& pool : commandPools) {
        vkDestroyCommandPool(device, pool, nullptr);
    }

    for (auto& devInfo : instanceInfo.devices) {
        for (auto& d : devInfo.logicalDevices) {
            vkDestroyDevice(d, nullptr);
        }
    }

    vkDestroySurfaceKHR(instanceInfo.instance, instanceInfo.surface, nullptr);
    vkDestroyInstance(instanceInfo.instance, nullptr);

    primitives.clear();
    commandInfos.clear();
    commandPools.clear();
    instanceInfo.devices.clear();
    instanceInfo.initialized = false;
}

void DeleteRenderData(const InstanceInfo& instanceInfo, vector<CommandInfo>& commandInfos, PipelineInfo& pipelineInfo, VkRenderPass& renderPass, SwapchainInfo& swapchainInfo)
{
    DeviceInfo deviceInfo = instanceInfo.devices[0];
    VkDevice device = deviceInfo.logicalDevices[0];

    vkDeviceWaitIdle(device);

    for (auto& cmd : commandInfos) {
        if (cmd.buffer.size() == 0) {
            continue;
        }
        vkFreeCommandBuffers(device, cmd.pool, (uint32_t)cmd.buffer.size(), cmd.buffer.data());
    }

    vkDestroyPipeline(device, pipelineInfo.pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineInfo.layout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    DeleteSwapchain(deviceInfo, swapchainInfo);
}

void DeleteSwapchain(const DeviceInfo& deviceInfo, SwapchainInfo& swapchainInfo)
{
    size_t size = swapchainInfo.framebuffers.size();
    VkDevice device = deviceInfo.logicalDevices[0];
    for (size_t i = 0; i < size; i++) {
        vkDestroyFramebuffer(device, swapchainInfo.framebuffers[i], nullptr);
        vkDestroyImageView(device, swapchainInfo.views[i], nullptr);
        // The application must not destroy a swapchain until after completion of all outstanding operations on images that
        // were acquired from the swapchain. swapchain and all associated VkImage handles are destroyed, and must not be
        // acquired or used any more by the application.
        //vkDestroyImage(device, swapchainInfo.images[i], nullptr);

        vkDestroyImageView(device, swapchainInfo.depthImageInfo[i].view, nullptr);
        vkDestroyImage(device, swapchainInfo.depthImageInfo[i].image, nullptr);
        vkFreeMemory(device, swapchainInfo.depthImageInfo[i].memory, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchainInfo.swapchain, nullptr);
}

void CreateDescriptorSetLayout(VkDescriptorSetLayout& descriptorSetLayout, DeviceInfo& deviceInfo)
{
    VkDescriptorSetLayoutBinding uboBinding = {};
    uboBinding.binding = 0;
    uboBinding.descriptorCount = 1;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.pImmutableSamplers = nullptr;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerBinding = {};
    samplerBinding.binding = 1;
    samplerBinding.descriptorCount = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.pImmutableSamplers = nullptr;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding binding[] = { uboBinding, samplerBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = binding;

    VkDevice device = deviceInfo.logicalDevices[0];
    VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkCreateDescriptorSetLayout: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
}

void CreateUniformBuffers(vector<VkBuffer>& uniformBuffers, vector<VkDeviceMemory>& uniformBuffersMemory, DeviceInfo& deviceInfo, SwapchainInfo& swapchainInfo)
{
    VkDeviceSize bufferSize = sizeof(MVP);

    uniformBuffers.resize(swapchainInfo.images.size());
    uniformBuffersMemory.resize(swapchainInfo.images.size());

    for (size_t i = 0; i < swapchainInfo.images.size(); i++) {
        CreateBuffer(deviceInfo, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
    }
}

void UpdateMVP(vector<VkDeviceMemory>& mvpMemory, uint32_t currentImageIndex, DeviceInfo& deviceInfo, SwapchainInfo& swapchainInfo)
{
    static auto startTime = high_resolution_clock::now();

    auto currentTime = high_resolution_clock::now();
    float time = duration<float, seconds::period>(currentTime - startTime).count();

    MVP mvp = {};
    mvp.model = rotate(mat4(1.0f), time * radians(90.0f) * 0.25f, vec3(0.0f, 0.0f, 1.0f));
    mvp.view = lookAt(vec3(0.0f, -6.0f, 3.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    mvp.projection = perspective(radians(45.0f), swapchainInfo.extent.width / (float) swapchainInfo.extent.height, 0.1f, 16.0f);
    mvp.projection[1][1] *= -1;

    VkDevice device = deviceInfo.logicalDevices[0];
    void* data;
    vkMapMemory(device, mvpMemory[currentImageIndex], 0, sizeof(mvp), 0, &data);
    memcpy(data, &mvp, sizeof(mvp));
    vkUnmapMemory(device, mvpMemory[currentImageIndex]);
}

void CreateDescriptorPool(VkDescriptorPool& descriptorPool, DeviceInfo& deviceInfo, SwapchainInfo& swapchainInfo)
{
    VkDescriptorPoolSize poolSizes[] = { {}, {} };
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(swapchainInfo.images.size());
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(swapchainInfo.images.size());

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = static_cast<uint32_t>(swapchainInfo.images.size());

    VkDevice device = deviceInfo.logicalDevices[0];
    VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkCreateDescriptorPool: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
}

void CreateDescriptorSets(ResourceDescriptor& descriptor, vector<VkBuffer>& uniformBuffers, TextureObject& textureObject, DeviceInfo& deviceInfo, SwapchainInfo& swapchainInfo)
{
    descriptor.layouts.resize(swapchainInfo.images.size(), descriptor.layouts[0]);
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptor.pool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(swapchainInfo.images.size());
    allocInfo.pSetLayouts = descriptor.layouts.data();

    descriptor.sets.resize(swapchainInfo.images.size());
    VkDevice device = deviceInfo.logicalDevices[0];
    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, descriptor.sets.data());
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkAllocateDescriptorSets: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
    // Avoid destroying the same descriptor layout.
    descriptor.layouts.resize(1);

    for (size_t i = 0; i < swapchainInfo.images.size(); i++) {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = VK_WHOLE_SIZE;

        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureObject.imageInfo.view;
        imageInfo.sampler = textureObject.sampler;

        VkWriteDescriptorSet descriptorWrites[] = { {}, {} };
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptor.sets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptor.sets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, nullptr);
    }
}

void CreateDepthBuffer(SwapchainInfo &swapchainInfo, const DeviceInfo &deviceInfo, CommandInfo &commandInfo)
{
    VkFormat depthFormat = FindSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, deviceInfo.physicalDevice
    );

    swapchainInfo.depthImageInfo.resize(swapchainInfo.images.size());
    for (size_t i = 0; i < swapchainInfo.images.size(); i++) {
        CreateImage(swapchainInfo.extent.width, swapchainInfo.extent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, swapchainInfo.depthImageInfo[i].image, swapchainInfo.depthImageInfo[i].memory, deviceInfo);
        swapchainInfo.depthImageInfo[i].view = CreateImageView(swapchainInfo.depthImageInfo[i].image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, deviceInfo.logicalDevices[0]);
        TransitionImageLayout(swapchainInfo.depthImageInfo[i].image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, commandInfo, deviceInfo);
    }
}

void LoadModel(const char* filePath, vector<VertexV1>& vertices, vector<uint32_t>& indices, float scale)
{
    attrib_t attrib;
    vector<shape_t> shapes;
    vector<material_t> materials;
    string warn, err;

    if (!LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath)) {
        throw runtime_error(err);
    }

    unordered_map<VertexV1, uint32_t> uniqueVertices = {};
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            VertexV1 vertex = {};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0] * scale,
                attrib.vertices[3 * index.vertex_index + 1] * scale,
                attrib.vertices[3 * index.vertex_index + 2] * scale
            };
            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };
            vertex.color = { 1.0f, 1.0f, 1.0f };

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }

    /*for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            VertexV1 vertex = {};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1 - attrib.texcoords[2 * index.texcoord_index + 1]
            };
            vertex.color = {1.0f, 1.0f, 1.0f};

            vertices.push_back(vertex);
            indices.push_back(indices.size());
        }
    }*/
}