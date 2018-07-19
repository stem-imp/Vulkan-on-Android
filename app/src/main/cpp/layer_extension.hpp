#ifndef LAYERANDEXTENSION_HPP
#define LAYERANDEXTENSION_HPP

#include "log.hpp"
#include "vulkan_wrapper.h"
#include <vector>
#include <cstdint>

using std::vector;

// Some loader only report one layer for device layers, enable this to
// workaround it: it will copy all instance layers into device layers
// and NOT enumerating device layers
//#define LOADER_DEVICE_LAYER_REPORT_BUG_WA

// If extension is not enumerated, it should not be enabled in general;
// If you know an extension is on the device, but loader does not report it,
// it could be forced in by enabling
// the following compile flag and call AddInstanceExt().
// #define ENABLE_NON_ENUMERATED_EXT 1

// A Helper class to manage validation layers and extensions
// Supposed usage:
//   1) validation layers: app should enable them with
//        InstanceLayerCount()
//        InstanceLayerNames()
//   2) Extension layers:  app should check for supportability
//        and then enable in app code ( not in this class )
//   3) DbgExtension: once instance is created, just call
//        HookDbgReportExt
class LayerAndExtension
{
public:
    LayerAndExtension(void);
    ~LayerAndExtension();

    uint32_t InstanceLayerCount(void);
    char** InstanceLayerNames(void);
    uint32_t InstanceExtensionCount(void);
    char** InstanceExtensionNames(void);

    uint32_t DeviceLayerCount(void);
    char** DeviceLayerNames(void);
    uint32_t DeviceExtensionCount(void);
    char** DeviceExtensionNames(void);

    const char* GetDebugExtensionName(void);
    bool AddInstanceExtension(const char* extName);

    bool HookDebugReportExtension(VkInstance instance);

    void InitializeDeviceLayersAndExtensions(VkPhysicalDevice physicalDevice);
private:
    struct ExtensionInfo {
        uint32_t count;
        VkExtensionProperties* prop;
    };

    VkInstance _instance;
    VkDebugReportCallbackEXT _debugCallbackHandle;
    VkPhysicalDevice _physicalDev;

    vector<char*> _instanceLayers;
    vector<char*> _instanceExtensions;
    vector<char*> _deviceLayers;
    vector<char*> _deviceExtensions;

    VkLayerProperties* _instanceLayerProperties = nullptr;
    uint32_t _instanceLayerCount = 0;
    vector<ExtensionInfo> _instanceExtensionProperties;

    VkLayerProperties* _deviceLayerProperties = nullptr;
    uint32_t _deviceLayerCount = 0;
    vector<ExtensionInfo> _deviceExtensionProperties;

    bool InitializeInstanceExtensions(void);
    bool InitializeExtensionNames(const vector<ExtensionInfo>& props, vector<char*>* names);
    void CheckLayerLoadingSequence(vector<char*>* layers);

    bool IsInstanceExtensionSupported(const char* extName);
};

#endif // LAYERANDEXTENSION_HPP
