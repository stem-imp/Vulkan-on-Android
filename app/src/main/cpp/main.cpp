#include "log/log.h"
#include "androidutility/eventloop/event_loop.h"
#include "vulkan/vulkan_utility.h"
#include "vulkan/layer_extension.h"
#include <memory>

using std::unique_ptr;
using Utility::Log;
using AndroidNative::EventLoop;

static VkInstance appInstance  = VK_NULL_HANDLE;
static VkPhysicalDevice appGpu = VK_NULL_HANDLE;
static VkDevice appDevice      = VK_NULL_HANDLE;

static bool OnActivate();
static void OnDeactivate();
static bool OnStep();

static void OnStart(void);
static void OnResume(void);
static void OnPause(void);
static void OnStop(void);
static void OnDestroy(void);

static void OnInitWindow(android_app* app);
static void OnTermWindow(void);
static void OnGainFocus(void);
static void OnLostFocus(void);

static void OnSaveInstanceState(void**, size_t*);
static void OnConfigurationChanged(void);
static void OnLowMemory(void);

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state)
{
    Log::Tag = "vulkan";
    unique_ptr<EventLoop> loop(new EventLoop(state));
    loop->onActivate = OnActivate;
    loop->onDeactivate = OnDeactivate;
    loop->onStep = OnStep;

    loop->onStart = OnStart;
    loop->onResume = OnResume;
    loop->onPause = OnPause;
    loop->onStop = OnStop;
    loop->onDestroy = OnDestroy;

    loop->onInitWindow = OnInitWindow;
    loop->onTermWindow = OnTermWindow;
    loop->onGainFocus = OnGainFocus;
    loop->onLostFocus = OnLostFocus;

    loop->onSaveInstanceState = OnSaveInstanceState;
    loop->onConfigurationChanged = OnConfigurationChanged;
    loop->onLowMemory = OnLowMemory;

    loop->Run();
}

static bool OnActivate()
{
    DebugLog("App OnActivate");
    return true;
}

static void OnDeactivate()
{
    DebugLog("App OnDeactivate");
}

bool OnStep()
{
    //DebugLog("App OnStep");
    return true;
}

void OnStart(void)
{
    DebugLog("App OnStart");
}

void OnResume(void)
{
    DebugLog("App OnResume");
}

void OnPause(void)
{
    DebugLog("App OnPause");
}

void OnStop(void)
{
    DebugLog("App OnStop");
}

void OnDestroy(void)
{
    DebugLog("App OnDestroy");
}

void OnInitWindow(android_app* app)
{
    DebugLog("App OnInitWindow");

    if (!InitVulkan()) {
        Log::Error("Vulkan is unavailable, install vulkan and re-start.");
        return;
    }

    vector<const char*> requestedInstanceExtensionNames = { VK_KHR_SURFACE_EXTENSION_NAME };
    AppendInstanceExtension(requestedInstanceExtensionNames);
    LayerAndExtension layerAndExtension;
    if (layerAndExtension.enableValidationLayers) {
        requestedInstanceExtensionNames.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

        vector<const char*> requestedInstanceLayerNames = {
            LayerAndExtension::GOOGLE_THREADING_LAYER,// LayerAndExtension::GOOGLE_UNIQUE_OBJECT_LAYER,
            LayerAndExtension::LUNARG_CORE_VALIDATION_LAYER, LayerAndExtension::LUNARG_OBJECT_TRACKER_LAYER,
            LayerAndExtension::LUNARG_PARAMETER_VALIDATION_LAYER
        };
        layerAndExtension.EnableInstanceLayers(requestedInstanceLayerNames);
    }
    layerAndExtension.EnableInstanceExtensions(requestedInstanceExtensionNames);
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .apiVersion = GetAPIVersion(),
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .pApplicationName = "Vulkan on Android",
        .pEngineName = "vulkan",
    };
    vector<const char*> enabledInstanceLayerNames = layerAndExtension.EnabledInstanceLayerNames();
    vector<const char*> enabledInstanceExtensionNames = layerAndExtension.EnabledInstanceExtensionNames();
    VkInstanceCreateInfo instanceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = layerAndExtension.EnabledInstanceExtensionCount(),
        .ppEnabledExtensionNames = enabledInstanceExtensionNames.data(),
        .enabledLayerCount = layerAndExtension.EnabledInstanceLayerCount(),
        .ppEnabledLayerNames = enabledInstanceLayerNames.data(),
    };
    VK_CHECK_RESULT(vkCreateInstance(&instanceCreateInfo, nullptr, &appInstance));
    if (layerAndExtension.enableValidationLayers) {
        layerAndExtension.HookDebugReportExtension(appInstance);
    }


    uint32_t gpuCount = 0;
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(appInstance, &gpuCount, nullptr));
    VkPhysicalDevice tmpGpus[gpuCount];
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(appInstance, &gpuCount, tmpGpus));
    appGpu = tmpGpus[0];
    LayerAndExtension::ExtensionGroup extInfo = { 0, {} };
    VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(appGpu, nullptr, &extInfo.count, nullptr));
    DebugLog("%d device extension properties.", extInfo.count);
    vector<LayerAndExtension::ExtensionGroup> supportedDeviceExtensions;
    if (extInfo.count) {
        extInfo.properties.resize(extInfo.count);
        VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(appGpu,
                                                             nullptr,
                                                             &extInfo.count,
                                                             extInfo.properties.data()));
        supportedDeviceExtensions.emplace_back(extInfo);
    }
    size_t size = layerAndExtension.EnabledInstanceLayerNames().size();
    for (size_t i = 0; i < size; i++) {
        extInfo.count = 0;

        VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(appGpu,
                                                             layerAndExtension.EnabledInstanceLayerNames()[i],
                                                             &extInfo.count,
                                                             nullptr));
        DebugLog("%s has %d device extension properties.",
                 layerAndExtension.EnabledInstanceLayerNames()[i], extInfo.count);
        if (extInfo.count) {
            extInfo.properties.resize(extInfo.count);
            VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(appGpu,
                                                                 layerAndExtension.EnabledInstanceLayerNames()[i],
                                                                 &extInfo.count,
                                                                 nullptr));
            for (int i = 0; i < extInfo.count; i++) {
                DebugLog("  Extension name: %s", extInfo.properties[i].extensionName);
            }
            supportedDeviceExtensions.emplace_back(extInfo);
        }
    }
    vector<string> supportedDeviceExtensionNames;
    LayerAndExtension::PickUniqueExtensionNames(supportedDeviceExtensions, supportedDeviceExtensionNames);
    vector<const char*> requestedDeviceExtensionNames = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    bool enable = false;
    for (auto name : supportedDeviceExtensionNames) {
        if (!strcmp(name.c_str(), requestedDeviceExtensionNames[0])) {
            enable = true;
        }
    }
    if (!enable) {
        Log::Error("Cannot enable %s", requestedDeviceExtensionNames[0]);
        return;
    }

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(appGpu, &queueFamilyCount, nullptr);
    vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(appGpu, &queueFamilyCount, queueFamilyProperties.data());
    uint32_t queueFamilyIndex;
    for (queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++) {
        if (queueFamilyProperties[queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            break;
        }
    }
    assert(queueFamilyIndex < queueFamilyCount);


    float priorities[] = {
        1.0f,
    };
    VkDeviceQueueCreateInfo queueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueCount = 1,
        .queueFamilyIndex = queueFamilyIndex,
        // Send nullptr for queue priority so debug extension could
        // catch the bug and call back app's debug function
        //.pQueuePriorities = priorities
        .pQueuePriorities = nullptr
    };
    const uint32_t deviceExtensionCount = requestedDeviceExtensionNames.size();
    VkDeviceCreateInfo deviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledLayerCount = layerAndExtension.EnabledInstanceLayerCount(),
        .ppEnabledLayerNames = enabledInstanceLayerNames.data(),
        .enabledExtensionCount = deviceExtensionCount,
        .ppEnabledExtensionNames = requestedDeviceExtensionNames.data(),
        .pEnabledFeatures = nullptr,
    };
    vkCreateDevice(appGpu, &deviceCreateInfo, nullptr, &appDevice);
}

void OnTermWindow(void)
{
    DebugLog("App OnTermWindow");

    if (appDevice) {
        vkDestroyDevice(appDevice, nullptr), appDevice = VK_NULL_HANDLE;
    }
    vkDestroyInstance(appInstance, nullptr), appInstance = VK_NULL_HANDLE;
}

void OnGainFocus(void)
{
    DebugLog("App OnGainFocus");
}

void OnLostFocus(void)
{
    DebugLog("App OnLostFocus");
}

void OnSaveInstanceState(void**, size_t*)
{
    DebugLog("App OnSaveInstanceState");
}

void OnConfigurationChanged(void)
{
    DebugLog("App OnConfigurationChanged");
}

void OnLowMemory(void)
{
    DebugLog("App OnLowMemory");
}