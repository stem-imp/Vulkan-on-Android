#include "layer_extension.h"
#include "vulkan_utility.h"

const char* const LayerAndExtension::GOOGLE_UNIQUE_OBJECT_LAYER        = "VK_LAYER_GOOGLE_unique_objects";
const char* const LayerAndExtension::GOOGLE_THREADING_LAYER            = "VK_LAYER_GOOGLE_threading";
const char* const LayerAndExtension::LUNARG_CORE_VALIDATION_LAYER      = "VK_LAYER_LUNARG_core_validation";
const char* const LayerAndExtension::LUNARG_OBJECT_TRACKER_LAYER       = "VK_LAYER_LUNARG_object_tracker";
const char* const LayerAndExtension::LUNARG_PARAMETER_VALIDATION_LAYER = "VK_LAYER_LUNARG_parameter_validation";

static VkBool32 VKAPI_PTR VKDebugReportCallbackEXImplementation(VkDebugReportFlagsEXT      flags,
                                                                VkDebugReportObjectTypeEXT objectType,
                                                                uint64_t                   object,
                                                                size_t                     location,
                                                                int32_t                    messageCode,
                                                                const char*                layerPrefix,
                                                                const char*                message,
                                                                void*                      userData);

LayerAndExtension::LayerAndExtension(void):
#ifdef NDEBUG
        enableValidationLayers(false),
#else
        enableValidationLayers(true),
#endif
        _instance(VK_NULL_HANDLE),
        _debugReportCallbackExt(VK_NULL_HANDLE),
        _instanceLayerProperties(VK_NULL_HANDLE)
{
    uint32_t instanceLayerCount;
    VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr));
    if (instanceLayerCount) {
        _instanceLayerProperties.resize(instanceLayerCount);
        VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&instanceLayerCount, _instanceLayerProperties.data()));
    }
    for (uint32_t i = 0; i < instanceLayerCount; i++) {
        _instanceLayerNamesSupported.push_back(_instanceLayerProperties[i].layerName);
        DebugLog("instance Layer Name: %s", _instanceLayerProperties[i].layerName);
    }
    CheckLayerLoadingSequence(_instanceLayerNamesSupported);

    EnumerateInstanceAndLayerExtensions();
}

void LayerAndExtension::CheckLayerLoadingSequence(vector<string>& instanceLayerNamesSupported)
{
    // VK_LAYER_GOOGLE_unique_objects need to be after
    // VK_LAYER_LUNARG_core_validation
    // VK_GOOGLE_THREADING_LAYER better to be the very first one
    vector<string>& layers = instanceLayerNamesSupported;
    uint32_t uniqueObjIdx = -1;
    uint32_t threadingIdx = -1;
    uint32_t size = layers.size();
    for (uint32_t idx = 0; idx < size; ++idx) {
        if (layers[idx] == GOOGLE_UNIQUE_OBJECT_LAYER) {
            uniqueObjIdx = idx;
        }
        if (layers[idx] == GOOGLE_THREADING_LAYER) {
            threadingIdx = idx;
        }
    }
    if (uniqueObjIdx != -1) {
        std::swap(layers[uniqueObjIdx], layers[size - 1]);
    }

    if (threadingIdx != -1) {
        std::swap(layers[threadingIdx], layers[0]);
    }
}

bool LayerAndExtension::EnumerateInstanceAndLayerExtensions()
{
    ExtensionGroup extInfo = { 0, {} };
    VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &extInfo.count, nullptr));

    vector<ExtensionGroup> _instanceExtensions;
    DebugLog("%d instance extension properties.", extInfo.count);
    if (extInfo.count) {
        extInfo.properties.resize(extInfo.count);
        VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &extInfo.count, extInfo.properties.data()));
        for (int i = 0; i < extInfo.count; i++) {
            DebugLog("  Extension name: %s", extInfo.properties[i].extensionName);
        }
        _instanceExtensions.emplace_back(extInfo);
    }

    for (int i = 0; i < _instanceLayerNamesSupported.size(); i++) {
        extInfo.count = 0;
        VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(_instanceLayerProperties[i].layerName, &extInfo.count, nullptr));
        DebugLog("%s has %d instance extension properties.", _instanceLayerProperties[i].layerName, extInfo.count);
        if (extInfo.count == 0) {
            continue;
        }

        extInfo.properties.resize(extInfo.count);
        VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(_instanceLayerProperties[i].layerName, &extInfo.count, extInfo.properties.data()));
        for (int i = 0; i < extInfo.count; i++) {
            DebugLog("  Extension name: %s", extInfo.properties[i].extensionName);
        }
        _instanceExtensions.emplace_back(extInfo);
    }
    _instanceExtensionNamesSupported.clear();
    return PickUniqueExtensionNames(_instanceExtensions, _instanceExtensionNamesSupported);
}

bool LayerAndExtension::PickUniqueExtensionNames(const vector<ExtensionGroup>& prop, vector<string>& instanceExtensionNamesSupported)
{
    vector<string>& names = instanceExtensionNamesSupported;
    names.clear();
    for (auto& ext : prop) {
        for (uint32_t i = 0; i < ext.count; ++i) {
            bool skip = false;
            for (uint32_t j = 0; j < names.size(); ++j) {
                if (!strcmp(ext.properties[i].extensionName, names[j].c_str()) || ext.properties[i].extensionName[0] == 0) {
                    skip = true;
                    break;
                }
            }
            if (skip) {
                continue;
            }
            names.push_back(&ext.properties[i].extensionName[0]);
            DebugLog("Unique extension name: %s", ext.properties[i].extensionName);
        }
    }
    return true;
}

LayerAndExtension::~LayerAndExtension()
{
    if (_debugReportCallbackExt) {
        DebugLog("~LayerAndExtension() vkDestroyDebugReportCallbackEXT");
        vkDestroyDebugReportCallbackEXT(_instance, _debugReportCallbackExt, nullptr);
        vkCreateDebugReportCallbackEXT = nullptr;
    } else {
        DebugLog("~LayerAndExtension()");
    }
}

bool LayerAndExtension::EnableInstanceLayers(const vector<const char *>& requestedLayerNames)
{
    if (!requestedLayerNames.size() || !enableValidationLayers) {
        return false;
    }

    _instanceLayerNamesEnabled.clear();
    int i = 0;
    for (; i < requestedLayerNames.size(); i++) {
        if (IsInstanceLayerSupported(requestedLayerNames[i])) {
            _instanceLayerNamesEnabled.push_back(requestedLayerNames[i]);
        } else {
            Log::Warn("%s is not supported.", requestedLayerNames[i]);
        }
    }
    return (i == requestedLayerNames.size());
}

bool LayerAndExtension::IsInstanceLayerSupported(const char* const layerName) const
{
    for (const auto name : _instanceLayerNamesSupported) {
        if (!strcmp(name.c_str(), layerName)) {
            return true;
        }
    }
    DebugLog("%s is not supported.", layerName);
    return false;
}

bool LayerAndExtension::EnableInstanceExtensions(const vector<const char*>& requestedExtensionNames)
{
    if (!requestedExtensionNames.size()) {
        return false;
    }

    if (!enableValidationLayers) {
        for (auto name : requestedExtensionNames) {
            if (!strcmp(name, VK_EXT_DEBUG_REPORT_EXTENSION_NAME)) {
                return false;
            }
        }
    }

    _instanceExtensionNamesEnabled.clear();
    int i = 0;
    for (; i < requestedExtensionNames.size(); i++) {
        if (IsInstanceExtensionSupported(requestedExtensionNames[i])) {
            _instanceExtensionNamesEnabled.push_back(requestedExtensionNames[i]);
        } else {
#ifdef ENABLE_NON_ENUMERATED_EXT
            _instanceExtensionNamesEnabled.push_back(static_cast<char*>(requestedExtensionNames[i]));
            break;
#else
            Log::Warn("%s is not supported.", requestedExtensionNames[i]);
#endif
        }
    }
    bool result = (i == requestedExtensionNames.size());
    if (!result) {
        _instanceExtensionNamesEnabled.clear();
    }
    return result;
}

bool LayerAndExtension::IsInstanceExtensionSupported(const char* const extensionName) const
{
    for (const auto name : _instanceExtensionNamesSupported) {
        if (!strcmp(name.c_str(), extensionName)) {
            return true;
        }
    }
    return false;
}

uint32_t LayerAndExtension::EnabledInstanceLayerCount(void) const
{
    DebugLog("%d enabled instance layers.", _instanceLayerNamesEnabled.size());
    return static_cast<uint32_t>(_instanceLayerNamesEnabled.size());
}

const vector<const char*> LayerAndExtension::EnabledInstanceLayerNames(void) const
{
    vector<const char*> ret = {};
    for (auto& name : _instanceLayerNamesEnabled) {
        DebugLog("  %s enabled", name.c_str());
        ret.push_back(name.c_str());
    }
    return ret;
}

uint32_t LayerAndExtension::EnabledInstanceExtensionCount(void) const
{
    DebugLog("%d enabled instance extensions.", _instanceExtensionNamesEnabled.size());
    return static_cast<uint32_t>(_instanceExtensionNamesEnabled.size());
}

const vector<const char*> LayerAndExtension::EnabledInstanceExtensionNames(void) const
{
    if (_instanceExtensionNamesEnabled.size()) {
        vector<const char*> ret = {};
        for (auto& name : _instanceExtensionNamesEnabled) {
            DebugLog("  %s enabled", name.c_str());
            ret.push_back(name.c_str());
        }
        return ret;
    }
    return {};
}

bool LayerAndExtension::HookDebugReportExtension(VkInstance instance)
{
    assert(instance);
    _instance = instance;
    if (!IsInstanceExtensionSupported(VK_EXT_DEBUG_REPORT_EXTENSION_NAME)) {
        return false;
    }
    if (!vkCreateDebugReportCallbackEXT) {
        vkCreateDebugReportCallbackEXT  = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
        vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
        vkDebugReportMessageEXT         = (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(instance, "vkDebugReportMessageEXT");
    }

    VkDebugReportCallbackCreateInfoEXT dbgInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT,
        .pNext = nullptr,
        .flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
                 VK_DEBUG_REPORT_WARNING_BIT_EXT |
                 VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
                 VK_DEBUG_REPORT_ERROR_BIT_EXT |
                 VK_DEBUG_REPORT_DEBUG_BIT_EXT,
        .pfnCallback = VKDebugReportCallbackEXImplementation,
        .pUserData = this,
    };
    VK_CHECK_RESULT(vkCreateDebugReportCallbackEXT(instance, &dbgInfo, nullptr, &_debugReportCallbackExt));
    return true;
}

static VkBool32 VKAPI_PTR VKDebugReportCallbackEXImplementation(VkDebugReportFlagsEXT      flags,
                                                                VkDebugReportObjectTypeEXT objectType,
                                                                uint64_t                   object,
                                                                size_t                     location,
                                                                int32_t                    messageCode,
                                                                const char*                layerPrefix,
                                                                const char*                pMessage,
                                                                void*                      userData)
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