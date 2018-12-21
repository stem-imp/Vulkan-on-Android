#include "device.h"
#include "layer_extension.h"
#include "vulkan_utility.h"

using ExtensionGroup = LayerAndExtension::ExtensionGroup;

namespace Vulkan {

    Device::Device(VkPhysicalDevice physicalDevice)
    {
        assert(physicalDevice);
        _physicalDevice = physicalDevice;

        vkGetPhysicalDeviceProperties(_physicalDevice, &_properties);
        DebugLog("maxImageArrayLayers: %d", _properties.limits.maxImageArrayLayers);
        DebugLog("maxPushConstantsSize: %d", _properties.limits.maxPushConstantsSize);
        DebugLog("maxBoundDescriptorSets: %d", _properties.limits.maxBoundDescriptorSets);
        DebugLog("maxDescriptorSetSamplers: %d", _properties.limits.maxDescriptorSetSamplers);
        DebugLog("maxDescriptorSetUniformBuffers: %d", _properties.limits.maxDescriptorSetUniformBuffers);
        DebugLog("maxDescriptorSetUniformBuffersDynamic: %d", _properties.limits.maxDescriptorSetUniformBuffersDynamic);
        DebugLog("maxDescriptorSetSampledImages: %d", _properties.limits.maxDescriptorSetSampledImages);
        DebugLog("maxDescriptorSetInputAttachments: %d", _properties.limits.maxDescriptorSetInputAttachments);
        DebugLog("maxVertexInputAttributes: %d", _properties.limits.maxVertexInputAttributes);
        DebugLog("maxVertexInputBindings: %d", _properties.limits.maxVertexInputBindings);

        vkGetPhysicalDeviceFeatures(physicalDevice, &_featuresSupported);
        DebugLog("robustBufferAccess: %d", _featuresSupported.robustBufferAccess);
        DebugLog("fullDrawIndexUint32: %d", _featuresSupported.fullDrawIndexUint32);
        DebugLog("imageCubeArray: %d", _featuresSupported.imageCubeArray);
        DebugLog("independentBlend: %d", _featuresSupported.independentBlend);
        DebugLog("geometryShader: %d", _featuresSupported.geometryShader);
        DebugLog("tessellationShader: %d", _featuresSupported.tessellationShader);
        DebugLog("sampleRateShading: %d", _featuresSupported.sampleRateShading);
        DebugLog("dualSrcBlend: %d", _featuresSupported.dualSrcBlend);
        DebugLog("logicOp: %d", _featuresSupported.logicOp);
        DebugLog("multiDrawIndirect: %d", _featuresSupported.multiDrawIndirect);
        DebugLog("drawIndirectFirstInstance: %d", _featuresSupported.drawIndirectFirstInstance);
        DebugLog("depthClamp: %d", _featuresSupported.depthClamp);
        DebugLog("depthBiasClamp: %d", _featuresSupported.depthBiasClamp);
        DebugLog("fillModeNonSolid: %d", _featuresSupported.fillModeNonSolid);
        DebugLog("wideLines: %d", _featuresSupported.wideLines);
        DebugLog("largePoints: %d", _featuresSupported.largePoints);
        DebugLog("alphaToOne: %d", _featuresSupported.alphaToOne);
        DebugLog("multiViewport: %d", _featuresSupported.multiViewport);
        DebugLog("samplerAnisotropy: %d", _featuresSupported.samplerAnisotropy);
        DebugLog("textureCompressionETC2: %d", _featuresSupported.textureCompressionETC2);
        DebugLog("textureCompressionASTC_LDR: %d", _featuresSupported.textureCompressionASTC_LDR);
        DebugLog("textureCompressionBC: %d", _featuresSupported.textureCompressionBC);
        DebugLog("occlusionQueryPrecise: %d", _featuresSupported.occlusionQueryPrecise);
        DebugLog("pipelineStatisticsQuery: %d", _featuresSupported.pipelineStatisticsQuery);
        DebugLog("vertexPipelineStoresAndAtomics: %d", _featuresSupported.vertexPipelineStoresAndAtomics);
        DebugLog("fragmentStoresAndAtomics: %d", _featuresSupported.fragmentStoresAndAtomics);
        DebugLog("shaderTessellationAndGeometryPointSize: %d", _featuresSupported.shaderTessellationAndGeometryPointSize);
        DebugLog("shaderImageGatherExtended: %d", _featuresSupported.shaderImageGatherExtended);
        DebugLog("shaderStorageImageExtendedFormats: %d", _featuresSupported.shaderStorageImageExtendedFormats);
        DebugLog("shaderStorageImageMultisample: %d", _featuresSupported.shaderStorageImageMultisample);
        DebugLog("shaderStorageImageReadWithoutFormat: %d", _featuresSupported.shaderStorageImageReadWithoutFormat);
        DebugLog("shaderStorageImageWriteWithoutFormat: %d", _featuresSupported.shaderStorageImageWriteWithoutFormat);
        DebugLog("shaderUniformBufferArrayDynamicIndexing: %d", _featuresSupported.shaderUniformBufferArrayDynamicIndexing);
        DebugLog("shaderSampledImageArrayDynamicIndexing: %d", _featuresSupported.shaderSampledImageArrayDynamicIndexing);
        DebugLog("shaderStorageBufferArrayDynamicIndexing: %d", _featuresSupported.shaderStorageBufferArrayDynamicIndexing);
        DebugLog("shaderStorageImageArrayDynamicIndexing: %d", _featuresSupported.shaderStorageImageArrayDynamicIndexing);
        DebugLog("shaderClipDistance: %d", _featuresSupported.shaderClipDistance);
        DebugLog("shaderCullDistance: %d", _featuresSupported.shaderCullDistance);
        DebugLog("shaderFloat64: %d", _featuresSupported.shaderFloat64);
        DebugLog("shaderInt64: %d", _featuresSupported.shaderInt64);
        DebugLog("shaderInt16: %d", _featuresSupported.shaderInt16);
        DebugLog("shaderResourceResidency: %d", _featuresSupported.shaderResourceResidency);
        DebugLog("shaderResourceMinLod: %d", _featuresSupported.shaderResourceMinLod);
        DebugLog("sparseBinding: %d", _featuresSupported.sparseBinding);
        DebugLog("sparseResidencyBuffer: %d", _featuresSupported.sparseResidencyBuffer);
        DebugLog("sparseResidencyImage2D: %d", _featuresSupported.sparseResidencyImage2D);
        DebugLog("sparseResidencyImage3D: %d", _featuresSupported.sparseResidencyImage3D);
        DebugLog("sparseResidency2Samples: %d", _featuresSupported.sparseResidency2Samples);
        DebugLog("sparseResidency4Samples: %d", _featuresSupported.sparseResidency4Samples);
        DebugLog("sparseResidency8Samples: %d", _featuresSupported.sparseResidency8Samples);
        DebugLog("sparseResidency16Samples: %d", _featuresSupported.sparseResidency16Samples);
        DebugLog("sparseResidencyAliased: %d", _featuresSupported.sparseResidencyAliased);
        DebugLog("variableMultisampleRate: %d", _featuresSupported.variableMultisampleRate);
        DebugLog("inheritedQueries: %d", _featuresSupported.inheritedQueries);
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &_memoryProperties);

        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        if (queueFamilyCount <= 0) {
            throw runtime_error("No queue family properties.");
        }
        _queueFamilyProperties.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, _queueFamilyProperties.data());
        DebugLog("Constructor Device(VkPhysicalDevice)");
    }

    Device::~Device()
    {
        DebugLog("~Device()");
        vkDestroyDevice(_device, nullptr);
    }

    void Device::EnumerateExtensions(const vector<const char*>& instanceLayerNames)
    {
        assert(_physicalDevice);
        ExtensionGroup extInfo = { 0, {} };
        VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extInfo.count, nullptr));
        DebugLog("%d device extension properties.", extInfo.count);
        vector<ExtensionGroup> deviceExtensions;
        if (extInfo.count) {
            extInfo.properties.resize(extInfo.count);
            VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extInfo.count,
                                                                 extInfo.properties.data()));
            deviceExtensions.emplace_back(extInfo);
        }
        size_t size = instanceLayerNames.size();
        for (size_t i = 0; i < size; i++) {
            extInfo.count = 0;

            VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(_physicalDevice, instanceLayerNames[i], &extInfo.count, nullptr));
            DebugLog("%s has %d device extension properties.", instanceLayerNames[i], extInfo.count);
            if (extInfo.count) {
                extInfo.properties.resize(extInfo.count);
                VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(_physicalDevice, instanceLayerNames[i], &extInfo.count, nullptr));
                for (int i = 0; i < extInfo.count; i++) {
                    DebugLog("  Extension name: %s", extInfo.properties[i].extensionName);
                }
                deviceExtensions.emplace_back(extInfo);
            }
        }
        _supportedDeviceExtensionNames.clear();
        LayerAndExtension::PickUniqueExtensionNames(deviceExtensions, _supportedDeviceExtensionNames);
    }

    bool Device::EnableDeviceExtensions(const vector<const char*>& extensionNames)
    {
        assert(_physicalDevice);
        if (!extensionNames.size()) {
            return false;
        }

        _enabledDeviceExtensionNames.clear();
        int i = 0;
        for (; i < extensionNames.size(); i++) {
            if (IsDeviceExtensionSupported(extensionNames[i])) {
                _enabledDeviceExtensionNames.push_back(static_cast<const char*>(extensionNames[i]));
            } else {
                break;
            }
        }
        bool res = (i == extensionNames.size());
        if (!res) {
            Log::Warn("Request %d extensions but only %s are supported.", extensionNames.size(), i);
            _enabledDeviceExtensionNames.clear();
        }
        return res;
    }

    bool Device::IsDeviceExtensionSupported(const char* extensionName) const
    {
        for (auto& name : _supportedDeviceExtensionNames) {
            if (!strcmp(name.c_str(), extensionName)) {
                return true;
            }
        }
        return false;
    }

    int Device::GetQueueFamilyIndex(VkQueueFlagBits queueFlagBit, VkSurfaceKHR surface)
    {
        // Dedicated transfer queue family.
        if (queueFlagBit == VK_QUEUE_TRANSFER_BIT) {
            for (uint32_t i = 0; i < static_cast<uint32_t>(_queueFamilyProperties.size()); i++) {
                if ((_queueFamilyProperties[i].queueFlags & queueFlagBit) &&
                    ((_queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)) {
                    _dedicatedTransferQueueFamily = true;
                    _familyQueues.transfer.index = i;
                    return i;
                }
            }
        }
            // Dedicated compute queue family.
        else if (queueFlagBit == VK_QUEUE_COMPUTE_BIT) {
            for (uint32_t i = 0; i < static_cast<uint32_t>(_queueFamilyProperties.size()); i++) {
                if ((_queueFamilyProperties[i].queueFlags & queueFlagBit) &&
                    ((_queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)) {
                    _dedicatedComputeQueueFamily = true;
                    _familyQueues.compute.index = i;
                    return i;
                }
            }
        }

        uint32_t size = _queueFamilyProperties.size();
        // Save info about whether a queue family of a physical device supports presentation to a given surface to presentables.
        vector<int> presentables;
        if (queueFlagBit == VK_QUEUE_GRAPHICS_BIT) {
            assert (surface);
            presentables.resize(size);
            for (uint32_t i = 0; i < size; i++) {
                VkBool32 support = true;
                vkGetPhysicalDeviceSurfaceSupportKHR(_physicalDevice, i, surface, &support);
                presentables[i] = support ? i : -1;
            }
        }

        // Get index for queue family;
        for (uint32_t i = 0; i < size; i++) {
            if ((_queueFamilyProperties[i].queueFlags & queueFlagBit)) {
                if (queueFlagBit == VK_QUEUE_GRAPHICS_BIT) {
                    _familyQueues.graphics.index = i;

                    for (uint32_t j = 0; j < size; j++) {
                        if (presentables[j] != -1) {
                            _familyQueues.present.index = j;
                            // I prefer shared graphics and present queue family.
                            if (i == j) {
                                _sharedGraphicsAndPresentQueueFamily = true;
                                break;
                            }
                        }
                    }
                } else if (queueFlagBit == VK_QUEUE_COMPUTE_BIT) {
                    _familyQueues.compute.index = i;
                    // Compute family might support presentation as graphics family, but I don't care.
                } else if (queueFlagBit == VK_QUEUE_TRANSFER_BIT) {
                    _familyQueues.transfer.index = i;
                } else if (queueFlagBit == VK_QUEUE_SPARSE_BINDING_BIT) {
                    // As far as I know there is no device supporting dedicated sparse binding queue family right now.
                    _familyQueues.sparseBinding.index = i;
                }
                return i;
            }
        }

        // All commands that are allowed on a queue that supports transfer operations are also allowed on a queue that
        // supports either graphics or compute operations. Thus, if the capabilities of a queue family include
        // VK_QUEUE_GRAPHICS_BIT or VK_QUEUE_COMPUTE_BIT, then reporting the VK_QUEUE_TRANSFER_BIT capability separately
        // for that queue family is optional.
        if (queueFlagBit == VK_QUEUE_TRANSFER_BIT) {
            if (_familyQueues.compute.index != -1) {
                _familyQueues.transfer.index = _familyQueues.compute.index;
            }
            if (_familyQueues.graphics.index != -1) {
                _familyQueues.transfer.index = _familyQueues.graphics.index;
            }
            if (_familyQueues.transfer.index == -1) {
                Log::Warn("Could not find a matching queue family index for queue flags %d.", queueFlagBit);
                return -1;
            } else {
                return _familyQueues.transfer.index;
            }
        }

        Log::Warn("Could not find a matching queue family index for queue flags %d.", queueFlagBit);
        return -1;
    }

    VkSampleCountFlagBits Device::GetMaxUsableSampleCount() const
    {
        VkSampleCountFlags counts = std::min(_properties.limits.framebufferColorSampleCounts,
                                             _properties.limits.framebufferDepthSampleCounts);
        if (counts & VK_SAMPLE_COUNT_64_BIT) {
            return VK_SAMPLE_COUNT_64_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_32_BIT) {
            return VK_SAMPLE_COUNT_32_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_16_BIT) {
            return VK_SAMPLE_COUNT_16_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_8_BIT) {
            return VK_SAMPLE_COUNT_8_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_4_BIT) {
            return VK_SAMPLE_COUNT_4_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_2_BIT) {
            return VK_SAMPLE_COUNT_2_BIT;
        }

        return VK_SAMPLE_COUNT_1_BIT;
    }

    void Device::RequestSampleCount(VkSampleCountFlagBits& sampleCount)
    {
        VkSampleCountFlagBits maxSampleCount = GetMaxUsableSampleCount();
        if (sampleCount > maxSampleCount) {
            sampleCount = maxSampleCount;
        }
    }

    const vector<const char*> Device::EnabledDeviceExtensionNames() const
    {
        vector<const char*> ret = {};
        for (auto& e : _enabledDeviceExtensionNames) {
            ret.push_back(e.c_str());
        }
        return ret;
    }

    void Device::CreateDevice(const VkPhysicalDeviceFeatures& requestedFeatures,
                              const vector<const char*> &requestedExtensions)
    {
        vector<VkDeviceQueueCreateInfo> queueInfos = {};
        const float qeuePriority = 0.0f;

        VkDeviceQueueCreateInfo queueInfo = {};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.pNext = nullptr;
        queueInfo.flags = 0;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &qeuePriority;
        if (_familyQueues.graphics.index != -1) {
            queueInfo.queueFamilyIndex = _familyQueues.graphics.index;
            queueInfos.emplace_back(queueInfo);
        }
        if (_dedicatedComputeQueueFamily) {
            queueInfo.queueFamilyIndex = _familyQueues.compute.index;
            queueInfos.emplace_back(queueInfo);
        }
        if (_dedicatedTransferQueueFamily) {
            queueInfo.queueFamilyIndex = _familyQueues.transfer.index;
            queueInfos.emplace_back(queueInfo);
        }
        if (_dedicatedSparseBindingQueueFamily) {
            queueInfo.queueFamilyIndex = _familyQueues.sparseBinding.index;
            queueInfos.emplace_back(queueInfo);
        }
        if (_familyQueues.present.index != _familyQueues.graphics.index &&
            _familyQueues.present.index != _familyQueues.compute.index &&
            _familyQueues.present.index != -1)
        {
            queueInfo.queueFamilyIndex = _familyQueues.present.index;
            queueInfos.emplace_back(queueInfo);
        }

        VkDeviceCreateInfo deviceInfo = {};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.pNext = nullptr;
        deviceInfo.flags = 0;
        deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
        deviceInfo.pQueueCreateInfos = queueInfos.data();
        //deviceInfo.enabledLayerCount
        //deviceInfo.ppEnabledLayerNames
        deviceInfo.enabledExtensionCount = static_cast<uint32_t>(_enabledDeviceExtensionNames.size());
        vector<const char*> enabledDeviceExtensionNames = EnabledDeviceExtensionNames();
        deviceInfo.ppEnabledExtensionNames = enabledDeviceExtensionNames.data();
        deviceInfo.pEnabledFeatures = &requestedFeatures;
        _featuresEnabled = requestedFeatures;
        VK_CHECK_RESULT(vkCreateDevice(_physicalDevice, &deviceInfo, nullptr, &_device));
    }

    void Device::GetFamilyQueues()
    {
        assert(_device);
        if (_familyQueues.graphics.index != -1) {
            vkGetDeviceQueue(_device, _familyQueues.graphics.index, 0, &_familyQueues.graphics.queue);
            if (_familyQueues.sparseBinding.index != -1) {
                _familyQueues.sparseBinding.queue = _familyQueues.graphics.queue;
            }
        }
        if (_dedicatedComputeQueueFamily) {
            vkGetDeviceQueue(_device, _familyQueues.compute.index, 0, &_familyQueues.compute.queue);
        } else if (_familyQueues.compute.index != -1) {
            assert(_familyQueues.graphics.queue);
            _familyQueues.compute.queue = _familyQueues.graphics.queue;
        }
        if (_dedicatedTransferQueueFamily) {
            vkGetDeviceQueue(_device, _familyQueues.transfer.index, 0, &_familyQueues.transfer.queue);
        } else if (_familyQueues.transfer.index != -1) {
            assert(_familyQueues.graphics.queue);
            _familyQueues.transfer.queue = _familyQueues.graphics.queue;
        }
        if (_familyQueues.present.index == _familyQueues.graphics.index) {
            _familyQueues.present.queue = _familyQueues.graphics.queue;
        } else {
            vkGetDeviceQueue(_device, _familyQueues.present.index, 0, &_familyQueues.present.queue);
        }
    }
}