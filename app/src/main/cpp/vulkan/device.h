#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H

#ifdef __ANDROID__
#include "vulkan_wrapper.h"
#endif
#include <string>
#include <vector>

using std::string;
using std::vector;

namespace Vulkan
{
    class Device {
    public:
        typedef struct QueuePair {
            int index = -1;
            //vector<VkQueue> queues;
            VkQueue queue = VK_NULL_HANDLE;
        } QueuePair;

        typedef struct QueueGroup {
            QueuePair graphics;
            QueuePair compute;
            QueuePair transfer;
            QueuePair sparseBinding;
            QueuePair present;
        } QueueGroup;

        Device(VkPhysicalDevice physicalDevice);
        ~Device();

        int GetQueueFamilyIndex(VkQueueFlagBits queueFlagBit, VkSurfaceKHR surface = VK_NULL_HANDLE);

        void EnumerateExtensions(const vector<const char*>& instanceLayerNames);
        bool EnableDeviceExtensions(const vector<const char*>& extensionNames);
        bool IsDeviceExtensionSupported(const char* extensionName) const;

        const VkPhysicalDeviceFeatures& FeaturesSupported() const { return _featuresSupported; }
        const VkPhysicalDeviceFeatures& FeaturesEnabled() const { return _featuresEnabled; }
        void RequestSampleCount(VkSampleCountFlagBits& sampleCount)
        {
            VkSampleCountFlagBits maxSampleCount = GetMaxUsableSampleCount();
            if (sampleCount > maxSampleCount) {
                sampleCount = maxSampleCount;
            }
        }

        void RequsetSamplerAnisotropy(float& samplerAnisotropy)
        {
            if (samplerAnisotropy > _properties.limits.maxSamplerAnisotropy) {
                samplerAnisotropy = _properties.limits.maxSamplerAnisotropy;
            }
        }

        void BuildDevice(const VkPhysicalDeviceFeatures& requestedFeatures, const vector<const char*>& requestedExtensions);

        VkPhysicalDevice PhysicalDevice() const { return _physicalDevice; }
        VkDevice LogicalDevice() const { return _device; }

        const QueueGroup& FamilyQueues() const { return _familyQueues; }
        bool DedicatedComputeQueueFamily() const { return _dedicatedComputeQueueFamily; }
        bool DedicatedTransferQueueFamily() const { return _dedicatedTransferQueueFamily; }
        bool DedicatedSparseBindingQueueFamily() const { return _dedicatedSparseBindingQueueFamily; }
        bool SharedGraphicsAndPresentQueueFamily() const { return _sharedGraphicsAndPresentQueueFamily; }

        const VkPhysicalDeviceProperties& PhysicalDeviceProperties() const { return  _properties; }

        VkQueueFlags queueFlags;
    private:
        void CreateDevice(const VkPhysicalDeviceFeatures& requestedFeatures, const vector<const char*>& requestedExtensions);
        void GetFamilyQueues();
        VkSampleCountFlagBits GetMaxUsableSampleCount() const;
        const vector<const char*> EnabledDeviceExtensionNames() const;

        // Base Data
        VkPhysicalDevice _physicalDevice;


        // Derived Data
        VkPhysicalDeviceProperties       _properties;
        VkPhysicalDeviceFeatures         _featuresSupported;
        VkPhysicalDeviceFeatures         _featuresEnabled;
        VkPhysicalDeviceMemoryProperties _memoryProperties;
        VkSampleCountFlagBits            _sampleCount;
        float                            _maxSamplerAnisotropy;

        vector<VkQueueFamilyProperties> _queueFamilyProperties;
        //std::unordered_map<VkDevice, QueueGroup> _familyQueuesGroup;
        QueueGroup _familyQueues;
        bool _dedicatedComputeQueueFamily         = false;
        bool _dedicatedTransferQueueFamily        = false;
        bool _dedicatedSparseBindingQueueFamily   = false;
        bool _sharedGraphicsAndPresentQueueFamily = false;

        VkDevice _device;
        //vector<VkDevice> _logicalDevices;
        vector<string> _supportedDeviceExtensionNames;
        vector<string> _enabledDeviceExtensionNames;
    };
}

#endif // VULKAN_DEVICE_H
