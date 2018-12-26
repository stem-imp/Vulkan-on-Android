//#ifndef LAYER_AND_EXTENSION_HPP
//#define LAYER_AND_EXTENSION_HPP
//
//#include "log.hpp"
//#include "vulkan_wrapper.h"
//#include <vector>
//#include <cstdint>
//
//using std::vector;
//
//// Some loader only report one layer for device layers, enable this to
//// workaround it: it will copy all instance layers into device layers
//// and NOT enumerating device layers
////#define LOADER_DEVICE_LAYER_REPORT_BUG_WA
//
//// If extension is not enumerated, it should not be enabled in general;
//// If you know an extension is on the device, but loader does not report it,
//// it could be forced in by enabling
//// the following compile flag and call EnableInstanceExtensions().
//// #define ENABLE_NON_ENUMERATED_EXT 1
//
//// A Helper class to manage validation layers and extensions
//// Supposed usage:
////   1) validation layers: app should enable them with
////        InstanceLayerEnabledCount()
////        InstanceLayerNamesEnabled()
////   2) Extension layers:  app should check for supportability
////        and then enable in app code ( not in this class )
////   3) DbgExtension: once instance is created, just call
////        HookDbgReportExt
//class LayerAndExtension
//{
//public:
//    LayerAndExtension(void);
//    ~LayerAndExtension();
//
//    bool EnableInstanceLayers(const vector<const char*>& layerNames);
//    bool EnableInstanceExtensions(const vector<const char*>& extensionNames);
//
//    uint32_t InstanceLayerEnabledCount(void) const;
//    const char* const* InstanceLayerNamesEnabled(void) const;
//    uint32_t InstanceExtensionEnabledCount(void) const;
//    const char* const* InstanceExtensionNamesEnabled(void) const;
//
//    bool HookDebugReportExtension(VkInstance instance);
//
//    void EnumerateDeviceLayersAndExtensions(VkPhysicalDevice physicalDevice);
//    bool IsDeviceExtensionSupported(const char* extensionName);
//
//    bool EnableDeviceExtensions(const vector<const char*>& extensionNames);
//
//    /*uint32_t DeviceLayerEnabledCount(void);
//    const char** DeviceLayerNamesEnabled(void);*/
//    uint32_t DeviceExtensionEnabledCount(void) const;
//    const char* const* DeviceExtensionNamesEnabled(void) const;
//
//    const char* GetDebugExtensionName(void);
//
//    static const char* UNIQUE_OBJECT_LAYER;
//    static const char* GOOGLE_THREADING_LAYER;
//    static const char* CORE_VALIDATION_LAYER;
//    static const char* OBJECT_TRACKER_LAYER;
//    static const char* SWAPCHAIN_LAYER;
//    static const char* IMAGE_LAYER;
//    static const char* PARAMETER_VALIDATION_LAYER;
//
//    static const char* DEBUG_EXTENSION_NAME;
//
//    const bool enableValidationLayers;
//private:
//    struct ExtensionInfo {
//        uint32_t count;
//        VkExtensionProperties* properties;
//    };
//
//    VkInstance _instance;
//    VkDebugReportCallbackEXT _debugCallbackHandle;
//    VkPhysicalDevice _physicalDev;
//
//    VkLayerProperties* _instanceLayerProperties = nullptr;
//    vector<char*> _instanceLayerNames;
//    //vector<ExtensionInfo> _instanceExtensions;
//    vector<char*> _instanceExtensionNames;
//
//    vector<const char*> _instanceLayerNamesEnabled;
//    vector<const char*> _instanceExtensionNamesEnabled;
//
//    vector<char*> _deviceExtensionNames;
//
//    vector<const char*> _deviceExtensionNamesEnabled;
//
//    void CheckLayerLoadingSequence(vector<char*>* layers);
//    bool EnumerateInstanceAndLayerExtensions();
//    bool PickUniqueExtensionNames(const vector<ExtensionInfo> &props, vector<char *> *names);
//
//    bool IsInstanceLayerSupported(const char* layerName);
//    bool IsInstanceExtensionSupported(const char* extensionName);
//};
//
//#endif // LAYER_AND_EXTENSION_HPP
