//#include "layer_extension.hpp"
//#include <cstring>
//#include <cassert>
//
//using AndroidNative::Log;
//using std::swap;
//
//const char* LayerAndExtension::UNIQUE_OBJECT_LAYER = "VK_LAYER_GOOGLE_unique_objects";
//const char* LayerAndExtension::GOOGLE_THREADING_LAYER = "VK_LAYER_GOOGLE_threading";
//const char* LayerAndExtension::CORE_VALIDATION_LAYER = "VK_LAYER_LUNARG_core_validation";
//const char* LayerAndExtension::OBJECT_TRACKER_LAYER = "VK_LAYER_LUNARG_object_tracker";
//const char* LayerAndExtension::SWAPCHAIN_LAYER = "VK_LAYER_LUNARG_swapchain";
//const char* LayerAndExtension::IMAGE_LAYER = "VK_LAYER_LUNARG_image";
//const char* LayerAndExtension::PARAMETER_VALIDATION_LAYER = "VK_LAYER_LUNARG_parameter_validation";
//// Debug Extension names in use.
//// assumed usage:
////   1) app calls GetDebugExtensionName()
////   2) app calls IsInstanceExtensionSupported() to make sure it is supported
////   3) app tucks dbg extension name into InstanceCreateInfo to create instance
////   4) app calls HookDebugReportExtension() to hook up debug print message
//const char* LayerAndExtension::DEBUG_EXTENSION_NAME = "VK_EXT_debug_report";
//
//static VkBool32 VKAPI_PTR VKDebugReportCallbackEXImplementation(
//        VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
//        uint64_t object, size_t location, int32_t messageCode,
//        const char *layerPrefix, const char *pMessage, void *userData
//);
//
//LayerAndExtension::LayerAndExtension(void) :
//#ifdef NDEBUG
//    enableValidationLayers(false),
//#else
//    enableValidationLayers(true),
//#endif
//    _instance(VK_NULL_HANDLE), _debugCallbackHandle(VK_NULL_HANDLE)
//{
//    uint32_t instanceLayerCount;
//    VkResult result = vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
//    if (result != VK_SUCCESS) {
//        string code = to_string(result);
//        throw runtime_error("vkEnumerateInstanceLayerProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
//    }
//
//    if (instanceLayerCount) {
//        _instanceLayerProperties = new VkLayerProperties[instanceLayerCount];
//        result = vkEnumerateInstanceLayerProperties(&instanceLayerCount, _instanceLayerProperties);
//        if (result != VK_SUCCESS) {
//            string code = to_string(result);
//            throw runtime_error("vkEnumerateInstanceLayerProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
//        }
//    }
//    for (uint32_t i = 0; i < instanceLayerCount; i++) {
//        _instanceLayerNames.push_back(_instanceLayerProperties[i].layerName);
//        DebugLog("Instance Layer Name: %s", _instanceLayerProperties[i].layerName);
//    }
//    CheckLayerLoadingSequence(&_instanceLayerNames);
//
//    EnumerateInstanceAndLayerExtensions();
//}
//
//void LayerAndExtension::CheckLayerLoadingSequence(vector<char*>* layerPtr)
//{
//    // VK_LAYER_GOOGLE_unique_objects need to be after
//    // VK_LAYER_LUNARG_core_validation
//    // VK_GOOGLE_THREADING_LAYER better to be the very first one
//    std::vector<char*>& layers = *layerPtr;
//    uint32_t uniqueObjIdx = -1;
//    uint32_t threadingIdx = -1;
//    uint32_t size = layers.size();
//    for (uint32_t idx = 0; idx < size; ++idx) {
//        if (!strcmp(layers[idx], UNIQUE_OBJECT_LAYER)) {
//            uniqueObjIdx = idx;
//        }
//        if (!strcmp(layers[idx], GOOGLE_THREADING_LAYER)) {
//            threadingIdx = idx;
//        }
//    }
//    if (uniqueObjIdx != -1) {
//        swap(layers[uniqueObjIdx], layers[size - 1]);
//    }
//
//    if (threadingIdx != -1) {
//        swap(layers[threadingIdx], layers[0]);
//    }
//}
//
//bool LayerAndExtension::EnumerateInstanceAndLayerExtensions()
//{
//    ExtensionInfo extInfo = { 0, nullptr };
//    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extInfo.count, nullptr);
//    if (result != VK_SUCCESS) {
//        string code = to_string(result);
//        throw runtime_error("vkEnumerateInstanceExtensionProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
//    }
//
//    vector<ExtensionInfo> _instanceExtensions;//.clear();
//    DebugLog("%d instance extension properties.", extInfo.count);
//    if (extInfo.count) {
//        extInfo.properties = new VkExtensionProperties[extInfo.count];
//        result = vkEnumerateInstanceExtensionProperties(nullptr, &extInfo.count, extInfo.properties);
//        if (result != VK_SUCCESS) {
//            string code = to_string(result);
//            throw runtime_error("vkEnumerateInstanceExtensionProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
//        }
//        for (int i = 0; i < extInfo.count; i++) {
//            DebugLog("  Extension name: %s", extInfo.properties[i].extensionName);
//        }
//        _instanceExtensions.push_back(extInfo);
//    }
//
//    for (int i = 0; i < _instanceLayerNames.size(); i++) {
//        extInfo.count = 0;
//        result = vkEnumerateInstanceExtensionProperties(_instanceLayerProperties[i].layerName, &extInfo.count, nullptr);
//        if (result != VK_SUCCESS) {
//            string code = to_string(result);
//            throw runtime_error("vkEnumerateInstanceExtensionProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
//        }
//        DebugLog("%s has %d instance extension properties.", _instanceLayerProperties[i].layerName, extInfo.count);
//        if (extInfo.count == 0) {
//            continue;
//        }
//
//        extInfo.properties = new VkExtensionProperties[extInfo.count];
//        result = vkEnumerateInstanceExtensionProperties(_instanceLayerProperties[i].layerName, &extInfo.count, extInfo.properties);
//        if (result != VK_SUCCESS) {
//            string code = to_string(result);
//            throw runtime_error("vkEnumerateInstanceExtensionProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
//        }
//        for (int i = 0; i < extInfo.count; i++) {
//            DebugLog("  Extension name: %s", extInfo.properties[i].extensionName);
//        }
//        _instanceExtensions.push_back(extInfo);
//    }
//    _instanceExtensionNames.clear();
//    return PickUniqueExtensionNames(_instanceExtensions, &_instanceExtensionNames);
//}
//
//bool LayerAndExtension::PickUniqueExtensionNames(const vector<ExtensionInfo> &prop, vector<char *> *names)
//{
//    names->clear();
//    for (auto& ext : prop) {
//        for (uint32_t i = 0; i < ext.count; ++i) {
//            // skip the one already inside
//            bool duplicate = false;
//            for (uint32_t j = 0; j < names->size(); ++j) {
//                if (!strcmp(ext.properties[i].extensionName, (*names)[j])) {
//                    duplicate = true;
//                    break;
//                }
//            }
//            if (duplicate) {
//                continue;
//            }
//            // save this unique one
//            names->push_back(ext.properties[i].extensionName);
//            DebugLog("Unique extension name: %s", ext.properties[i].extensionName);
//        }
//    }
//    return true;
//}
//
//LayerAndExtension::~LayerAndExtension()
//{
//    if (_instanceLayerProperties) {
//        delete[] _instanceLayerProperties;
//    }
//    if (_debugCallbackHandle) {
//        vkDestroyDebugReportCallbackEXT(_instance, _debugCallbackHandle, nullptr);
//        vkCreateDebugReportCallbackEXT = nullptr;
//    }
//}
//
//bool LayerAndExtension::EnableInstanceLayers(const vector<const char*>& layerNames)
//{
//    if (!layerNames.size() || !enableValidationLayers) {
//        return false;
//    }
//
//    _instanceLayerNamesEnabled.clear();
//    int i = 0;
//    for (; i < layerNames.size(); i++) {
//        if (IsInstanceLayerSupported(layerNames[i])) {
//            _instanceLayerNamesEnabled.push_back(static_cast<const char*>(layerNames[i]));
//        } else {
//            _instanceLayerNamesEnabled.clear();
//            break;
//        }
//    }
//    return (i == layerNames.size());
//}
//
//bool LayerAndExtension::IsInstanceLayerSupported(const char* layerName)
//{
//    for (auto name : _instanceLayerNames) {
//        if (!strcmp(name, layerName)) {
//            return true;
//        }
//    }
//    return false;
//}
//
//bool LayerAndExtension::EnableInstanceExtensions(const vector<const char*>& extensionNames)
//{
//    if (!extensionNames.size()) {
//        return false;
//    }
//
//    if (!enableValidationLayers) {
//        for (auto name : extensionNames) {
//            if (!strcmp(name, DEBUG_EXTENSION_NAME)) {
//                return false;
//            }
//        }
//    }
//
//    _instanceExtensionNamesEnabled.clear();
//    int i = 0;
//    for (; i < extensionNames.size(); i++) {
//        if (IsInstanceExtensionSupported(extensionNames[i])) {
//            _instanceExtensionNamesEnabled.push_back(static_cast<const char*>(extensionNames[i]));
//        } else {
//#ifdef ENABLE_NON_ENUMERATED_EXT
//            _instanceExtensionNamesEnabled.push_back(static_cast<char*>(extensionNames[i]));
//            break;
//#else
//            break;
//#endif
//        }
//    }
//    return (i == extensionNames.size());
//}
//
//bool LayerAndExtension::IsInstanceExtensionSupported(const char* extensionName)
//{
//    for (auto name : _instanceExtensionNames) {
//        if (!strcmp(name, extensionName)) {
//            return true;
//        }
//    }
//    return false;
//}
//
//uint32_t LayerAndExtension::InstanceLayerEnabledCount(void) const
//{
//    DebugLog("%d enabled instance layers.", _instanceLayerNamesEnabled.size());
//    return static_cast<uint32_t>(_instanceLayerNamesEnabled.size());
//}
//
//const char* const* LayerAndExtension::InstanceLayerNamesEnabled(void) const
//{
//    if (_instanceLayerNamesEnabled.size()) {
//        for (auto name : _instanceLayerNamesEnabled) {
//            DebugLog("  %s enabled", name);
//        }
//        return _instanceLayerNamesEnabled.data();
//    }
//    return nullptr;
//}
//
//uint32_t LayerAndExtension::InstanceExtensionEnabledCount(void) const
//{
//    DebugLog("%d enabled instance extensions.", _instanceExtensionNamesEnabled.size());
//    return static_cast<uint32_t>(_instanceExtensionNamesEnabled.size());
//}
//
//const char* const* LayerAndExtension::InstanceExtensionNamesEnabled(void) const
//{
//    if (_instanceExtensionNamesEnabled.size()) {
//        for (auto name : _instanceExtensionNamesEnabled) {
//            DebugLog("  %s enabled", name);
//        }
//        return _instanceExtensionNamesEnabled.data();
//    }
//    return nullptr;
//}
//
//bool LayerAndExtension::HookDebugReportExtension(VkInstance instance)
//{
//    _instance = instance;
//    if (!IsInstanceExtensionSupported(GetDebugExtensionName())) {
//        return false;
//    }
//    if (!vkCreateDebugReportCallbackEXT) {
//        vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
//        vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
//        vkDebugReportMessageEXT = (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(instance, "vkDebugReportMessageEXT");
//    }
//
//    VkDebugReportCallbackCreateInfoEXT dbgInfo = {
//        .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT,
//        .pNext = nullptr,
//        .flags = //VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
//                VK_DEBUG_REPORT_WARNING_BIT_EXT |
//                VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
//                VK_DEBUG_REPORT_ERROR_BIT_EXT |
//                VK_DEBUG_REPORT_DEBUG_BIT_EXT,
//        .pfnCallback = VKDebugReportCallbackEXImplementation,
//        .pUserData = this,  // we provide the debug object as context
//    };
//    VkResult result = vkCreateDebugReportCallbackEXT(instance, &dbgInfo, nullptr, &_debugCallbackHandle);
//    if (result != VK_SUCCESS) {
//        string code = to_string(result);
//        throw runtime_error("vkCreateDebugReportCallbackEXT: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
//    }
//    return true;
//}
//
//const char* LayerAndExtension::GetDebugExtensionName(void)
//{
//    return DEBUG_EXTENSION_NAME;
//}
//
//static VkBool32 VKAPI_PTR VKDebugReportCallbackEXImplementation(
//        VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
//        uint64_t object, size_t location, int32_t messageCode,
//        const char *layerPrefix, const char *pMessage, void *userData)
//{
//    if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
//        Log::Info("%s -- %s", layerPrefix, pMessage);
//    }
//    if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
//        Log::Warn("%s -- %s", layerPrefix, pMessage);
//    }
//    if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
//        Log::Warn("Performance: %s -- %s", layerPrefix, pMessage);
//    }
//    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
//        Log::Error("%s -- %s", layerPrefix, pMessage);
//    }
//    if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
//        Log::Debug("%s -- %s", layerPrefix, pMessage);
//    }
//
//    return VK_FALSE;
//}
//
//void LayerAndExtension::EnumerateDeviceLayersAndExtensions(VkPhysicalDevice physicalDevice)
//{
//    _physicalDev = physicalDevice;
//    if (physicalDevice == VK_NULL_HANDLE) {
//        return;
//    }
//
//    uint32_t deviceLayerCount;
//    VkResult result = vkEnumerateDeviceLayerProperties(physicalDevice, &deviceLayerCount, nullptr);
//    if (result != VK_SUCCESS) {
//        string code = to_string(result);
//        throw runtime_error("vkEnumerateDeviceLayerProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
//    }
//    DebugLog("%d device layer properties.", deviceLayerCount);
//    VkLayerProperties* deviceLayerProperties;
//    if (deviceLayerCount) {
//        deviceLayerProperties = new VkLayerProperties[deviceLayerCount];
//        result = vkEnumerateDeviceLayerProperties(physicalDevice, &deviceLayerCount, deviceLayerProperties);
//        if (result != VK_SUCCESS) {
//            string code = to_string(result);
//            throw runtime_error("vkEnumerateDeviceLayerProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
//        }
//    }
//
//#ifdef LOADER_DEVICE_LAYER_REPORT_BUG_WA
//    // validation layer for device only report out for one layer,
//    // but it seems we could ask for all layers that asked for instance
//    // so we just add them all in brutally
//    // assume all instance layers are also implemented for device layers
//    if (deviceLayerCount == 1) {
//        DebugLog("Only Reported One layer for device");
//        if (deviceLayerProperties) {
//            delete[] deviceLayerProperties;
//        }
//        size_t size = _instanceLayerNames.size();
//        deviceLayerProperties = (size ? (new VkLayerProperties[size]) : nullptr);
//        memcpy(deviceLayerProperties, _instanceLayerProperties, size * sizeof(VkLayerProperties));
//        deviceLayerCount = size;
//    }
//#endif
//
//    vector<char*> deviceLayerNames;
//    for (int i = 0; i < deviceLayerCount; i++) {
//        DebugLog("Device layer name: %s", deviceLayerProperties[i].layerName);
//        deviceLayerNames.push_back(deviceLayerProperties[i].layerName);
//    }
//    CheckLayerLoadingSequence(&deviceLayerNames);
//
//    ExtensionInfo extInfo = { 0, nullptr };
//    result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extInfo.count, nullptr);
//    if (result != VK_SUCCESS) {
//        string code = to_string(result);
//        throw runtime_error("vkEnumerateDeviceExtensionProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
//    }
//    DebugLog("%d device extension properties.", extInfo.count);
//    vector<ExtensionInfo> _deviceExtensions;
//    if (extInfo.count) {
//        extInfo.properties = new VkExtensionProperties[extInfo.count];
//        result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extInfo.count, extInfo.properties);
//        if (result != VK_SUCCESS) {
//            string code = to_string(result);
//            throw runtime_error("vkEnumerateDeviceExtensionProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
//        }
//        _deviceExtensions.push_back(extInfo);
//    }
//    for (int i = 0; i < deviceLayerCount; i++) {
//        extInfo.count = 0;
//        result = vkEnumerateDeviceExtensionProperties(physicalDevice, deviceLayerProperties[i].layerName, &extInfo.count, nullptr);
//        if (result != VK_SUCCESS) {
//            string code = to_string(result);
//            throw runtime_error("vkEnumerateDeviceExtensionProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
//        }
//        DebugLog("%s has %d device extension properties.", deviceLayerProperties[i].layerName, extInfo.count);
//        if (extInfo.count) {
//            extInfo.properties = new VkExtensionProperties[extInfo.count];
//            result = vkEnumerateDeviceExtensionProperties(physicalDevice, deviceLayerProperties[i].layerName, &extInfo.count, nullptr);
//            if (result != VK_SUCCESS) {
//                string code = to_string(result);
//                throw runtime_error("vkEnumerateDeviceExtensionProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
//            }
//            for (int i = 0; i < extInfo.count; i++) {
//                DebugLog("  Extension name: %s", extInfo.properties[i].extensionName);
//            }
//            _deviceExtensions.push_back(extInfo);
//        }
//    }
//    _deviceExtensionNames.clear();
//    PickUniqueExtensionNames(_deviceExtensions, &_deviceExtensionNames);
//}
//
//bool LayerAndExtension::EnableDeviceExtensions(const vector<const char*>& extensionNames)
//{
//    assert(_physicalDev != VK_NULL_HANDLE);
//    if (!extensionNames.size()) {
//        return false;
//    }
//
//    _deviceExtensionNamesEnabled.clear();
//    int i = 0;
//    for (; i < extensionNames.size(); i++) {
//        if (IsDeviceExtensionSupported(extensionNames[i])) {
//            _deviceExtensionNamesEnabled.push_back(static_cast<const char*>(extensionNames[i]));
//        } else {
//            break;
//        }
//    }
//    return (i == extensionNames.size());
//}
//
//bool LayerAndExtension::IsDeviceExtensionSupported(const char* extensionName)
//{
//    for (auto name : _deviceExtensionNames) {
//        if (!strcmp(name, extensionName)) {
//            return true;
//        }
//    }
//    return false;
//}
//
///*uint32_t LayerAndExtension::DeviceLayerEnabledCount()
//{
//    assert(_physicalDev != VK_NULL_HANDLE);
//    return _deviceLayerNamesEnabled.size();
//}
//
//const char** LayerAndExtension::DeviceLayerNamesEnabled()
//{
//    assert(_physicalDev != VK_NULL_HANDLE);
//    if (_deviceLayerNamesEnabled.size()) {
//        return _deviceLayerNamesEnabled.data();
//    }
//    return nullptr;
//}*/
//
//uint32_t LayerAndExtension::DeviceExtensionEnabledCount(void) const
//{
//    assert(_physicalDev != VK_NULL_HANDLE);
//    DebugLog("%d enabled device extensions.", _deviceExtensionNamesEnabled.size());
//    return _deviceExtensionNamesEnabled.size();
//}
//
//const char* const* LayerAndExtension::DeviceExtensionNamesEnabled(void) const
//{
//    assert(_physicalDev != VK_NULL_HANDLE);
//    if (_deviceExtensionNamesEnabled.size()) {
//        for (auto name : _deviceExtensionNamesEnabled) {
//            DebugLog("  %s enabled", name);
//        }
//        return _deviceExtensionNamesEnabled.data();
//    }
//    return nullptr;
//}
