#ifndef VULKAN_INSTANCE_H
#define VULKAN_INSTANCE_H

#include "layer_extension.h"
#ifdef __ANDROID__
#include "vulkan_wrapper.h"
#endif
#include <vector>

using std::vector;

namespace Vulkan
{
    class Instance {
    public:
        ~Instance();
        void CreateInstance(const LayerAndExtension& layerAndExtension);
        const vector<const char*> LayersEnabled() const;

        VkInstance GetInstance() const { return _instance; }
    private:
        const LayerAndExtension* _layerAndExtension;
        VkInstance               _instance;
    };
}

#endif // VULKAN_INSTANCE_H
