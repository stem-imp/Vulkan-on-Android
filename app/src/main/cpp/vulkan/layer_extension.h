#ifndef VULKAN_LAYER_EXTENSION_H
#define VULKAN_LAYER_EXTENSION_H

#ifdef __ANDROID__
#include "vulkan_wrapper.h"
#endif
#include <vector>
#include <string>

using std::vector;
using std::string;

class LayerAndExtension {
public:
    static const char* const GOOGLE_UNIQUE_OBJECT_LAYER;
    static const char* const GOOGLE_THREADING_LAYER;
    static const char* const LUNARG_CORE_VALIDATION_LAYER;
    static const char* const LUNARG_OBJECT_TRACKER_LAYER;
    static const char* const LUNARG_PARAMETER_VALIDATION_LAYER;

    const bool enableValidationLayers;

    struct ExtensionGroup {
        uint32_t count;
        vector<VkExtensionProperties> properties;
    };

    LayerAndExtension(void);
    ~LayerAndExtension();

    bool EnableInstanceLayers(const vector<const char *>& requestedLayerNames);
    bool EnableInstanceExtensions(const vector<const char*>& requestedExtensionNames);

    bool HookDebugReportExtension(VkInstance instance);

    static bool PickUniqueExtensionNames(const vector<ExtensionGroup>& prop, vector<string>& instanceExtensionNamesSupported);

    uint32_t EnabledInstanceLayerCount(void) const;
    uint32_t EnabledInstanceExtensionCount(void) const;
    const vector<const char*> EnabledInstanceLayerNames(void) const;
    const vector<string>& EnabledRawInstanceLayerNames(void) const { return _instanceLayerNamesEnabled; }
    const vector<const char*> EnabledInstanceExtensionNames(void) const;
private:
    VkInstance _instance;
    VkDebugReportCallbackEXT _debugReportCallbackExt;

    vector<VkLayerProperties> _instanceLayerProperties;
    vector<string> _instanceLayerNamesSupported;
    vector<string> _instanceLayerNamesEnabled;

    vector<string> _instanceExtensionNamesSupported;
    vector<string> _instanceExtensionNamesEnabled;

    void CheckLayerLoadingSequence(vector<string>& instanceLayerNamesSupported);
    bool EnumerateInstanceAndLayerExtensions();

    bool IsInstanceLayerSupported(const char* layerName) const;
    bool IsInstanceExtensionSupported(const char* extensionName) const;
};

#endif // VULKAN_LAYER_EXTENSION_H