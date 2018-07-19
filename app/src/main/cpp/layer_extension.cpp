#include "layer_extension.hpp"
#include <cstring>
#include <cassert>

using AndroidNative::Log;
using std::swap;

static const char* UNIQUE_OBJECT_LAYER = "VK_LAYER_GOOGLE_unique_objects";
static const char* GOOGLE_THREADING_LAYER = "VK_LAYER_GOOGLE_threading";
// Debug Extension names in use.
// assumed usage:
//   1) app calls GetDebugExtensionName()
//   2) app calls IsInstanceExtensionSupported() to make sure it is supported
//   3) app tucks dbg extension name into InstanceCreateInfo to create instance
//   4) app calls HookDebugReportExtension() to hook up debug print message
static const char* DEBUG_EXTENSION_NAME = "VK_EXT_debug_report";

static VkBool32 VKAPI_PTR VKDebugReportCallbackEXImplementation(
        VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
        uint64_t object, size_t location, int32_t messageCode,
        const char *layerPrefix, const char *pMessage, void *userData
);

LayerAndExtension::LayerAndExtension(void)
{
    _instance = VK_NULL_HANDLE;
    _debugCallbackHandle = VK_NULL_HANDLE;
    VkResult result = vkEnumerateInstanceLayerProperties(&_instanceLayerCount, nullptr);
    if (result != VK_SUCCESS) {
        if (result != VK_SUCCESS) {
            string code = to_string(result);
            throw runtime_error("vkEnumerateInstanceLayerProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
        }
    }

    if (_instanceLayerCount) {
        _instanceLayerProperties = new VkLayerProperties[_instanceLayerCount];
        assert(_instanceLayerProperties);
        result = vkEnumerateInstanceLayerProperties(&_instanceLayerCount, _instanceLayerProperties);
        if (result != VK_SUCCESS) {
            string code = to_string(result);
            throw runtime_error("vkEnumerateInstanceLayerProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
        }
    }
    for (uint32_t i = 0; i < _instanceLayerCount; i++) {
        _instanceLayers.push_back(_instanceLayerProperties[i].layerName);
        DebugLog("Instance Layer Name: %s", _instanceLayerProperties[i].layerName);
    }
    CheckLayerLoadingSequence(&_instanceLayers);

    InitializeInstanceExtensions();
}

void LayerAndExtension::CheckLayerLoadingSequence(vector<char*>* layerPtr)
{
    // VK_LAYER_GOOGLE_unique_objects need to be after
    // VK_LAYER_LUNARG_core_validation
    // VK_GOOGLE_THREADING_LAYER better to be the very first one
    std::vector<char*>& layers = *layerPtr;
    uint32_t uniqueObjIdx = -1;
    uint32_t threadingIdx = -1;
    uint32_t size = layers.size();
    for (uint32_t idx = 0; idx < size; ++idx) {
        if (!strcmp(layers[idx], UNIQUE_OBJECT_LAYER)) {
            uniqueObjIdx = idx;
        }
        if (!strcmp(layers[idx], GOOGLE_THREADING_LAYER)) {
            threadingIdx = idx;
        }
    }
    if (uniqueObjIdx != -1) {
        swap(layers[uniqueObjIdx], layers[size - 1]);
    }

    if (threadingIdx != -1) {
        swap(layers[threadingIdx], layers[0]);
    }
}

bool LayerAndExtension::InitializeInstanceExtensions()
{
    ExtensionInfo extInfo{0, nullptr};
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extInfo.count, nullptr);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkEnumerateInstanceExtensionProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
    DebugLog("%d instance extension properties.", extInfo.count);
    if (extInfo.count) {
        extInfo.prop = new VkExtensionProperties[extInfo.count];
        assert(extInfo.prop);
        result = vkEnumerateInstanceExtensionProperties(nullptr, &extInfo.count, extInfo.prop);
        if (result != VK_SUCCESS) {
            string code = to_string(result);
            throw runtime_error("vkEnumerateInstanceExtensionProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
        }
        _instanceExtensionProperties.push_back(extInfo);
    }

    for (int i = 0; i < _instanceLayerCount; i++) {
        extInfo.count = 0;
        result = vkEnumerateInstanceExtensionProperties(_instanceLayerProperties[i].layerName, &extInfo.count, nullptr);
        if (result != VK_SUCCESS) {
            string code = to_string(result);
            throw runtime_error("vkEnumerateInstanceExtensionProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
        }
        DebugLog("%s has %d instance extension properties.", _instanceLayerProperties[i].layerName, extInfo.count);
        if (extInfo.count == 0) {
            continue;
        }

        extInfo.prop = new VkExtensionProperties[extInfo.count];
        DebugLog("Filling instance extension properties for layer %s.", _instanceLayerProperties[i].layerName);
        result = vkEnumerateInstanceExtensionProperties(_instanceLayerProperties[i].layerName, &extInfo.count, extInfo.prop);
        if (result != VK_SUCCESS) {
            string code = to_string(result);
            throw runtime_error("vkEnumerateInstanceExtensionProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
        }
        _instanceExtensionProperties.push_back(extInfo);
    }
    return InitializeExtensionNames(_instanceExtensionProperties, &_instanceExtensions);
}

bool LayerAndExtension::InitializeExtensionNames(const vector<ExtensionInfo>& prop, vector<char*>* names)
{
    names->clear();
    for (auto& ext : prop) {
        for (uint32_t i = 0; i < ext.count; ++i) {
            // skip the one already inside
            bool duplicate = false;
            for (uint32_t j = 0; j < names->size(); ++j) {
                if (!strcmp(ext.prop[i].extensionName, (*names)[j])) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) {
                continue;
            }
            // save this unique one
            names->push_back(ext.prop[i].extensionName);
            DebugLog("Extension Name: %s", ext.prop[i].extensionName);
        }
    }
    return true;
}

LayerAndExtension::~LayerAndExtension()
{
    if (_instanceLayerProperties) {
        delete[] _instanceLayerProperties;
    }
    for (auto ext : _instanceExtensionProperties) {
        delete[] ext.prop;
    }
    if (_debugCallbackHandle) {
        vkDestroyDebugReportCallbackEXT(_instance, _debugCallbackHandle, nullptr);
    }
}

uint32_t LayerAndExtension::InstanceLayerCount(void)
{
    size_t size = _instanceLayers.size();
    //assert(size == _instanceLayerCount - 1); // except VK_LAYER_GOOGLE_unique_objects
    DebugLog("%d instance layers", size);
    return size;
}

char** LayerAndExtension::InstanceLayerNames()
{
    if (_instanceLayers.size()) {
        return _instanceLayers.data();
    }
    return nullptr;
}

uint32_t LayerAndExtension::InstanceExtensionCount(void)
{
    DebugLog("%d instance extensions.", static_cast<uint32_t>(_instanceExtensions.size()));
    return static_cast<uint32_t>(_instanceExtensions.size());
}

char** LayerAndExtension::InstanceExtensionNames(void)
{
    if (_instanceExtensions.size()) {
        return _instanceExtensions.data();
    }
    return nullptr;
}

bool LayerAndExtension::AddInstanceExtension(const char *extName)
{
    if (!extName) {
        return false;
    }

    // enable all available extensions, plus the one asked
    if (!IsInstanceExtensionSupported(extName)) {
#ifdef ENABLE_NON_ENUMERATED_EXT
        _instanceExtensions.push_back(static_cast<char*>(extName));
        return true;
#else
        return false;
#endif
    }
    return true;
}

bool LayerAndExtension::IsInstanceExtensionSupported(const char *extName)
{
    for (auto name : _instanceExtensions) {
        if (!strcmp(name, extName)) {
            return true;
        }
    }
    return false;
}

bool LayerAndExtension::HookDebugReportExtension(VkInstance instance)
{
    _instance = instance;
    if (!IsInstanceExtensionSupported(GetDebugExtensionName())) {
        return false;
    }
    if (!vkCreateDebugReportCallbackEXT) {
        vkCreateDebugReportCallbackEXT =
                (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
                        instance, "vkCreateDebugReportCallbackEXT");
        vkDestroyDebugReportCallbackEXT =
                (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
                        instance, "vkDestroyDebugReportCallbackEXT");
        vkDebugReportMessageEXT =
                (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(
                        instance, "vkDebugReportMessageEXT");
    }

    VkDebugReportCallbackCreateInfoEXT dbgInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT,
            .pNext = nullptr,
            .flags = VK_DEBUG_REPORT_WARNING_BIT_EXT |
                     VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
                     VK_DEBUG_REPORT_ERROR_BIT_EXT,
            .pfnCallback = VKDebugReportCallbackEXImplementation,
            .pUserData = this,  // we provide the debug object as context
    };
    VkResult result = vkCreateDebugReportCallbackEXT(instance, &dbgInfo, nullptr, &_debugCallbackHandle);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkCreateDebugReportCallbackEXT: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
    return true;
}

const char* LayerAndExtension::GetDebugExtensionName(void)
{
    return DEBUG_EXTENSION_NAME;
}

static VkBool32 VKAPI_PTR VKDebugReportCallbackEXImplementation(
        VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
        uint64_t object, size_t location, int32_t messageCode,
        const char *layerPrefix, const char *pMessage, void *userData)
{
    if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
        Log::Info("%s -- %s", layerPrefix, pMessage);
    }
    if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        Log::Warn("%s -- %s", layerPrefix, pMessage);
    }
    if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
        Log::Warn("Performance: %s -- %s", layerPrefix, pMessage);
    }
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        Log::Error("%s -- %s", layerPrefix, pMessage);
    }
    if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
        Log::Debug("%s -- %s", layerPrefix, pMessage);
    }

    return VK_FALSE;
}

void LayerAndExtension::InitializeDeviceLayersAndExtensions(VkPhysicalDevice physicalDevice)
{
    _physicalDev = physicalDevice;
    if (_physicalDev == VK_NULL_HANDLE) {
        return;
    }

    // get all supported layers props
    _deviceLayerCount = 0;
    VkResult result = vkEnumerateDeviceLayerProperties(_physicalDev, &_deviceLayerCount, nullptr);
    if (result == VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkEnumerateDeviceLayerProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
    DebugLog("%d device layer properties.", _deviceLayerCount);
    if (_deviceLayerCount) {
        _deviceLayerProperties = new VkLayerProperties[_deviceLayerCount];
        assert(_deviceLayerProperties);
        result = vkEnumerateDeviceLayerProperties(_physicalDev, &_deviceLayerCount, _deviceLayerProperties);
        if (result != VK_SUCCESS) {
            string code = to_string(result);
            throw runtime_error("vkEnumerateDeviceLayerProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
        }
    }

#ifdef LOADER_DEVICE_LAYER_REPORT_BUG_WA
    // validation layer for device only report out for one layer,
    // but it seems we could ask for all layers that asked for instance
    // so we just add them all in brutally
    // assume all device layers are also implemented for device layers
    if (_deviceLayerCount == 1) {
        DebugLog("Only Reported One layer for device");
        if (_deviceLayerProperties) {
            delete[] _deviceLayerProperties;
        }
        _deviceLayerProperties =(_instanceLayerCount ? (new VkLayerProperties[_instanceLayerCount]) : nullptr);
        memcpy(_deviceLayerProperties, _instanceLayerProperties, _instanceLayerCount * sizeof(VkLayerProperties));
        _deviceLayerCount = _instanceLayerCount;
    }
#endif

    for (int i = 0; i < _deviceLayerCount; i++) {
        if (!strcmp(_deviceLayerProperties[i].layerName, UNIQUE_OBJECT_LAYER)) {
            continue;
        }
        DebugLog("deviceLayerName: %s", _deviceLayerProperties[i].layerName);
        _deviceLayers.push_back(_deviceLayerProperties[i].layerName);
    }
    CheckLayerLoadingSequence(&_deviceLayers);

    ExtensionInfo ext{0, nullptr};
    result = vkEnumerateDeviceExtensionProperties(_physicalDev, nullptr, &ext.count, nullptr);
    if (result != VK_SUCCESS) {
        string code = to_string(result);
        throw runtime_error("vkEnumerateDeviceExtensionProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
    }
    DebugLog("%d device extension properties.", ext.count);
    if (ext.count) {
        ext.prop = new VkExtensionProperties[ext.count];
        assert(ext.prop);
        result = vkEnumerateDeviceExtensionProperties(_physicalDev, nullptr, &ext.count, ext.prop);
        if (result != VK_SUCCESS) {
            string code = to_string(result);
            throw runtime_error("vkEnumerateDeviceExtensionProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
        }
        _deviceExtensionProperties.push_back(ext);
    }
    for (int i = 0; i < _deviceLayerCount; i++) {
        ext.count = 0;
        result = vkEnumerateDeviceExtensionProperties(_physicalDev, _deviceLayerProperties[i].layerName, &ext.count, nullptr);
        if (result != VK_SUCCESS) {
            string code = to_string(result);
            throw runtime_error("vkEnumerateDeviceExtensionProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
        }
        DebugLog("%s has %d device extension properties.", _deviceLayerProperties[i].layerName, ext.count);
        if (ext.count) {
            ext.prop = new VkExtensionProperties[ext.count];
            assert(ext.prop);
            DebugLog("Filling device extension properties for layer %s.", _instanceLayerProperties[i].layerName);
            result = vkEnumerateDeviceExtensionProperties(_physicalDev, _deviceLayerProperties[i].layerName, &ext.count, nullptr);
            if (result != VK_SUCCESS) {
                string code = to_string(result);
                throw runtime_error("vkEnumerateDeviceExtensionProperties: code[" + code + "], file[" + __FILE__ + "], line[" + to_string(__LINE__) + "]");
            }
            _deviceExtensionProperties.push_back(ext);
        }
    }

    InitializeExtensionNames(_deviceExtensionProperties, &_deviceExtensions);
}

uint32_t LayerAndExtension::DeviceLayerCount()
{
    assert(_physicalDev != VK_NULL_HANDLE);
    return _deviceLayers.size();
}

char** LayerAndExtension::DeviceLayerNames()
{
    assert(_physicalDev != VK_NULL_HANDLE);
    if (_deviceLayers.size()) {
        return _deviceLayers.data();
    }
    return nullptr;
}

uint32_t LayerAndExtension::DeviceExtensionCount(void)
{
    assert(_physicalDev != VK_NULL_HANDLE);
    return _deviceExtensions.size();
}

char** LayerAndExtension::DeviceExtensionNames()
{
    assert(_physicalDev != VK_NULL_HANDLE);
    if (_deviceExtensions.size()) {
        return _deviceExtensions.data();
    }
    return nullptr;
}