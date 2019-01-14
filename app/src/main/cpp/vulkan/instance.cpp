#include "instance.h"
#include "../log/log.h"
#include "vulkan_utility.h"

using Utility::Log;

namespace Vulkan {

    Instance::~Instance()
    {
        if (_instance) {
            DebugLog("~Instance() vkDestroyInstance()");
            vkDestroyInstance(_instance, nullptr), _instance = nullptr;
        } else {
            DebugLog("~Instance()");
        }
    }

    void Instance::CreateInstance(const LayerAndExtension& layerAndExtension)
    {
        _layerAndExtension = &layerAndExtension;
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pNext = nullptr;
        appInfo.apiVersion = GetAPIVersion();
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pApplicationName = "Vulkan on Android";
        appInfo.pEngineName = "vulkan";

        VkInstanceCreateInfo instanceInfo = {};
        instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceInfo.pNext = nullptr;
        instanceInfo.flags = 0;
        instanceInfo.pApplicationInfo = &appInfo;
        instanceInfo.enabledExtensionCount = layerAndExtension.EnabledInstanceExtensionCount();
        const vector<const char*> enabledExtensionNames = layerAndExtension.EnabledInstanceExtensionNames();
        instanceInfo.ppEnabledExtensionNames = enabledExtensionNames.data();

        instanceInfo.enabledLayerCount = layerAndExtension.EnabledInstanceLayerCount();
        const vector<const char*> layersEnabled = layerAndExtension.EnabledInstanceLayerNames();
        instanceInfo.ppEnabledLayerNames = layersEnabled.data();
        VK_CHECK_RESULT(vkCreateInstance(&instanceInfo, nullptr, &_instance));
    }
}

const vector<const char*> Instance::LayersEnabled() const
{
    const vector<string>& layers = _layerAndExtension->EnabledRawInstanceLayerNames();
    vector<const char*> ret = {};
    for (auto& name : layers) {
        ret.push_back(name.c_str());
    }
    return ret;
}