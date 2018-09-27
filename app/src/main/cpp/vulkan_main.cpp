#include "vulkan_main.hpp"

#include "log.hpp"
#include <limits>
#include <algorithm>
#include <map>

using AndroidNative::Log;
using std::numeric_limits;
using std::pair;
using std::search;
using std::map;

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
void CreateRenderPass(const InstanceInfo& instanceInfo, const SwapchainInfo& swapchainInfo, VkRenderPass& renderPass);
// createGraphicsPipeline
void CreateFrameBuffers(const InstanceInfo& instanceInfo, SwapchainInfo& swapchainInfo, VkRenderPass renderPass, VkImageView depthView = VK_NULL_HANDLE);
void CreateCommandPool(const DeviceInfo& deviceInfo, VkQueueFlagBits family, CommandInfo& commandInfo);
void CreateSharedGraphicsAndPresentCommandBuffers(SwapchainInfo& swapchainInfo, const DeviceInfo& deviceInfo, CommandInfo& commandInfo, const VkRenderPass& renderPass);
void CreateSharedGraphicsAndPresentSyncPrimitives(const DeviceInfo& deviceInfo, DrawSyncPrimitives& primitives);

void DeleteSwapchain(const DeviceInfo& deviceInfo, SwapchainInfo& swapchainInfo);
void DeleteRenderData(const InstanceInfo& instanceInfo, vector<CommandInfo>& commandInfos, VkRenderPass& renderPass, SwapchainInfo& swapchainInfo);

bool InitVulkan(android_app* app, InstanceInfo& instanceInfo, SwapchainInfo& swapchainInfo, VkRenderPass& renderPass, set<VkCommandPool>& commandPools, vector<CommandInfo>& commandInfos, vector<DrawSyncPrimitives>& primitives)
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
    CreateFrameBuffers(instanceInfo, swapchainInfo, renderPass);

    commandPools.clear();
    commandInfos.clear();
    CommandInfo graphicsCommandInfo, transferCommandInfo;
    CreateCommandPool(instanceInfo.devices[0], VK_QUEUE_GRAPHICS_BIT, graphicsCommandInfo);
    commandInfos.push_back(graphicsCommandInfo);
    commandPools.insert(commandInfos[commandInfos.size() - 1].pool);
    CreateCommandPool(instanceInfo.devices[0], VK_QUEUE_TRANSFER_BIT, transferCommandInfo);
    commandInfos.push_back(transferCommandInfo);
    commandPools.insert(commandInfos[commandInfos.size() - 1].pool);

    if (instanceInfo.devices[0].sharedGraphicsAndPresentQueue) {
        CreateSharedGraphicsAndPresentCommandBuffers(swapchainInfo, instanceInfo.devices[0], commandInfos[0], renderPass);
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
    if (capabilities.currentExtent.width != numeric_limits<uint32_t>::max()) {
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

void RecreateSwapchain(const InstanceInfo& instanceInfo, vector<CommandInfo>& commandInfos, VkRenderPass& renderPass, SwapchainInfo& swapchainInfo)
{
    DeleteRenderData(instanceInfo, commandInfos, renderPass, swapchainInfo);
    CreateSwapChain(instanceInfo, swapchainInfo);
    CreateImageViews(instanceInfo, swapchainInfo);
    CreateRenderPass(instanceInfo, swapchainInfo, renderPass);
    CreateFrameBuffers(instanceInfo, swapchainInfo, renderPass);
    CreateSharedGraphicsAndPresentCommandBuffers(swapchainInfo, instanceInfo.devices[0], commandInfos[0], renderPass);
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
        VkImageViewCreateInfo viewCreateInfo = {};
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
        }
    }
}

void CreateRenderPass(const InstanceInfo& instanceInfo, const SwapchainInfo& swapchainInfo, VkRenderPass& renderPass)
{
    VkAttachmentDescription attachmentDescriptions = {};
    attachmentDescriptions.format = swapchainInfo.format;
    attachmentDescriptions.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescriptions.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescriptions.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    const DeviceInfo& deviceInfo = instanceInfo.devices[0];
    if (deviceInfo.sharedGraphicsAndPresentQueue) {
        attachmentDescriptions.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDescriptions.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    } else {
        attachmentDescriptions.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachmentDescriptions.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    VkAttachmentReference colourReference = {};
    colourReference.attachment = 0;
    colourReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colourReference;

    VkRenderPassCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &attachmentDescriptions;
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

void CreateFrameBuffers(const InstanceInfo& instanceInfo, SwapchainInfo& swapchainInfo, VkRenderPass renderPass, VkImageView depthView)
{
    size_t size = swapchainInfo.images.size();
    swapchainInfo.framebuffers.resize(size);
    for (size_t i = 0; i < size; i++) {
        VkImageView attachments[2] = {
            swapchainInfo.views[i], depthView,
        };
        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = (depthView == VK_NULL_HANDLE ? 1 : 2); // 2 if using depth
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

void CreateSharedGraphicsAndPresentCommandBuffers(SwapchainInfo& swapchainInfo, const DeviceInfo& deviceInfo, CommandInfo& commandInfo, const VkRenderPass& renderPass)
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

    VkClearValue clearColor;
    clearColor.color.float32[0] = 0.0f;
    clearColor.color.float32[1] = 0.34f;
    clearColor.color.float32[2] = 0.9f;
    clearColor.color.float32[3] = 1.0f;
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
        renderPassBeginInfo.clearValueCount = 1;
//        VkClearValue clearColor;
//        clearColor.color.float32[0] =  (float)i * 0.125f;
//        clearColor.color.float32[1] = (float)i * 0.25f;
//        clearColor.color.float32[2] = (float)i * 0.15625f;
//        clearColor.color.float32[3] = 1.0f;
        renderPassBeginInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandInfo.buffer[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        // draw something
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

VkResult DrawFrame(InstanceInfo& instanceInfo, SwapchainInfo& swapchainInfo, VkRenderPass& renderPass, vector<CommandInfo>& commandInfos, vector<DrawSyncPrimitives>& primitives)
{
    DeviceInfo& deviceInfo = instanceInfo.devices[0];
    VkDevice device = deviceInfo.logicalDevices[0];
    if (deviceInfo.sharedGraphicsAndPresentQueue) {
        DrawSyncPrimitives sharedPrimitive = primitives[0];
        vkWaitForFences(device, 1, &sharedPrimitive.inFlightFences[sharedPrimitive.currentFrame], VK_TRUE, numeric_limits<uint64_t>::max());

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapchainInfo.swapchain, numeric_limits<uint64_t>::max(), sharedPrimitive.imageAvailableSemaphores[sharedPrimitive.currentFrame], VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            RecreateSwapchain(instanceInfo, commandInfos, renderPass, swapchainInfo);
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            return result;
        }

        vkResetFences(device, 1, &sharedPrimitive.inFlightFences[sharedPrimitive.currentFrame]);

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
            instanceInfo.resolutionChanged = false;
            RecreateSwapchain(instanceInfo, commandInfos, renderPass, swapchainInfo);
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

void DeleteVulkan(InstanceInfo& instanceInfo, set<VkCommandPool>& commandPools, vector<CommandInfo>& commandInfos, VkRenderPass& renderPass, SwapchainInfo& swapchainInfo, vector<DrawSyncPrimitives>& primitives)
{
    DeviceInfo deviceInfo = instanceInfo.devices[0];
    VkDevice device = deviceInfo.logicalDevices[0];

    //////////////////////////////
    DeleteRenderData(instanceInfo, commandInfos, renderPass, swapchainInfo);
    ////////////////////////////////

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

void DeleteRenderData(const InstanceInfo& instanceInfo, vector<CommandInfo>& commandInfos, VkRenderPass& renderPass, SwapchainInfo& swapchainInfo)
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
    }
    vkDestroySwapchainKHR(device, swapchainInfo.swapchain, nullptr);
}