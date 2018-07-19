#include "event_loop.hpp"
#include "log.hpp"
#include "layer_extension.hpp"
#include "vulkan/vulkan.h"
#include <memory>
#include <cassert>

using std::unique_ptr;
using AndroidNative::EventLoop;
using AndroidNative::Log;

static bool initialized = false;

VkInstance appInstance;
VkPhysicalDevice appGpu;
VkDevice appDevice;
//VkSurfaceKHR appSurface;

static status OnActivate();
static void OnDeactivate();
static status OnStep();

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

static status OnActivate()
{
    DebugLog("App OnActivate");
    return OK;
}

static void OnDeactivate()
{
    DebugLog("App OnDeactivate");
}

status OnStep()
{
    DebugLog("App OnStep");
    return OK;
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
        Log::Error("Vulkan is unavailable, install vulkan and re-start");
        return;
    }
    if (initialized) {
        return;
    }

    LayerAndExtension layerAndExt;
    layerAndExt.AddInstanceExtension(layerAndExt.GetDebugExtensionName());

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .apiVersion = VK_MAKE_VERSION(1, 0, 0),
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .pApplicationName = "Android Vulkan",
        .pEngineName = "vulkan",
    };
    VkInstanceCreateInfo instanceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = layerAndExt.InstanceExtensionCount(),
        .ppEnabledExtensionNames = static_cast<const char* const*>(layerAndExt.InstanceExtensionNames()),
        .enabledLayerCount = layerAndExt.InstanceLayerCount(),
        .ppEnabledLayerNames = static_cast<const char* const*>(layerAndExt.InstanceLayerNames()),
    };
    VkResult result;
    result = vkCreateInstance(&instanceCreateInfo, nullptr, &appInstance);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkCreateInstance: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }

    layerAndExt.HookDebugReportExtension(appInstance);

    // Find one GPU to use:
    // On Android, every GPU device is equal -- supporting
    // graphics/compute/present
    // for this sample, we use the very first GPU device found on the system
    uint32_t gpuCount = 0;
    result = vkEnumeratePhysicalDevices(appInstance, &gpuCount, nullptr);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkEnumeratePhysicalDevices: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
    VkPhysicalDevice tmpGpus[gpuCount];
    result = vkEnumeratePhysicalDevices(appInstance, &gpuCount, tmpGpus);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkEnumeratePhysicalDevices: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
    appGpu = tmpGpus[0];  // Pick up the first GPU Device

    // Enumerate available device validation layers & extensions
    layerAndExt.InitializeDeviceLayersAndExtensions(appGpu);

    /*// check for vulkan info on this GPU device
    VkPhysicalDeviceProperties gpuProperties;
    vkGetPhysicalDeviceProperties(appGpu, &gpuProperties);
    InfoLog("Vulkan Physical Device Name: %s", gpuProperties.deviceName);
    InfoLog("Vulkan Physical Device Info: apiVersion: %x \n\t driverVersion: %x",
         gpuProperties.apiVersion, gpuProperties.driverVersion);
    InfoLog("API Version Supported: %d.%d.%d",
         VK_VERSION_MAJOR(gpuProperties.apiVersion),
         VK_VERSION_MINOR(gpuProperties.apiVersion),
         VK_VERSION_PATCH(gpuProperties.apiVersion));

    VkAndroidSurfaceCreateInfoKHR createInfo {
        .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .window = app->window
    };
    result = vkCreateAndroidSurfaceKHR(appInstance, &createInfo, nullptr, &appSurface);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkCreateAndroidSurfaceKHR: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(appGpu, appSurface, &surfaceCapabilities);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkGetPhysicalDeviceSurfaceCapabilitiesKHR: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }

    InfoLog("Vulkan Surface Capabilities:\n");
    InfoLog("\timage count: %u - %u\n", surfaceCapabilities.minImageCount,
         surfaceCapabilities.maxImageCount);
    InfoLog("\tarray layers: %u\n", surfaceCapabilities.maxImageArrayLayers);
    InfoLog("\timage size (now): %dx%d\n", surfaceCapabilities.currentExtent.width,
         surfaceCapabilities.currentExtent.height);
    InfoLog("\timage size (extent): %dx%d - %dx%d\n",
         surfaceCapabilities.minImageExtent.width,
         surfaceCapabilities.minImageExtent.height,
         surfaceCapabilities.maxImageExtent.width,
         surfaceCapabilities.maxImageExtent.height);
    InfoLog("\tusage: %x\n", surfaceCapabilities.supportedUsageFlags);
    InfoLog("\tcurrent transform: %u\n", surfaceCapabilities.currentTransform);
    InfoLog("\tallowed transforms: %x\n", surfaceCapabilities.supportedTransforms);
    InfoLog("\tcomposite alpha flags: %u\n", surfaceCapabilities.currentTransform);*/

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(appGpu, &queueFamilyCount, nullptr);
    assert(queueFamilyCount);
    vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(appGpu, &queueFamilyCount, queueFamilyProperties.data());

    uint32_t queueFamilyIndex;
    for (queueFamilyIndex=0; queueFamilyIndex < queueFamilyCount;
         queueFamilyIndex++) {
        if (queueFamilyProperties[queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            break;
        }
    }
    assert(queueFamilyIndex < queueFamilyCount);

    // Create a logical device from GPU we picked
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
        .pQueuePriorities = priorities,
    };

    VkDeviceCreateInfo deviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledLayerCount = layerAndExt.DeviceLayerCount(),
        .ppEnabledLayerNames = static_cast<const char* const*>(layerAndExt.DeviceLayerNames()),
        .enabledExtensionCount = layerAndExt.DeviceExtensionCount(),
        .ppEnabledExtensionNames = static_cast<const char* const*>(layerAndExt.DeviceExtensionNames()),
        .pEnabledFeatures = nullptr,
    };

    result = vkCreateDevice(appGpu, &deviceCreateInfo, nullptr, &appDevice);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkCreateDevice: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
    initialized = true;
}

void OnTermWindow(void)
{
    DebugLog("App OnTermWindow");

    //vkDestroySurfaceKHR(appInstance, appSurface, nullptr);
    vkDestroyDevice(appDevice, nullptr);
    vkDestroyInstance(appInstance, nullptr);

    initialized = false;
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