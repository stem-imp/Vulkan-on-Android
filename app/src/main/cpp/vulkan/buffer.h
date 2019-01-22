#ifndef VULKAN_BUFFER_H
#define VULKAN_BUFFER_H

#ifdef __ANDROID__
#include "vulkan_wrapper.h"
#endif
#include "device.h"
#include "vulkan_utility.h"

namespace Vulkan {

    class Buffer {
    public:
        Buffer(const Device& device) : _device(device) {}
        Buffer(Buffer&& other);
        ~Buffer();

        void CreateBuffer(VkDeviceSize        size,
                          VkBufferUsageFlags  usage,

                          const void*         pNext                 = nullptr,
                          VkBufferCreateFlags flags                 = 0,
                          VkSharingMode       sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
                          uint32_t            queueFamilyIndexCount = 0,
                          const uint32_t*     pQueueFamilyIndices   = nullptr);

        void AllocateBuffer(VkMemoryRequirements& memoryRequirements, VkMemoryPropertyFlags preferredProperties, const void* pNext = nullptr);

        void BindBuffer(VkDeviceSize memoryOffset = 0)
        {
            VK_CHECK_RESULT(vkBindBufferMemory(_device.LogicalDevice(), _buffer, _memory, memoryOffset));
        }

        void Free();

        typedef struct ExtendedBufferParameter
        {
            struct {
                const void*         pNext;
                VkBufferCreateFlags flags;
                VkSharingMode       sharingMode;
                uint32_t            queueFamilyIndexCount;
                const uint32_t*     pQueueFamilyIndices;
            } create;

            struct {
                const void*           pNext;
                VkDeviceSize          memoryOffset;
                VkMemoryRequirements* memoryRequirements;
            } allocateAndBind;
        } ExtendedBufferParameter;

        void BuildDefaultBuffer(VkDeviceSize          size,
                                VkBufferUsageFlags    usage,
                                VkMemoryPropertyFlags preferredProperties,
                                VkMemoryRequirements* memoryRequirements = nullptr);
        void BuildExtendedBuffer(VkDeviceSize             size,
                                 VkBufferUsageFlags       usage,
                                 VkMemoryPropertyFlags    preferredProperties,
                                 ExtendedBufferParameter& extendedBufferParameter,
                                 VkMemoryRequirements* memoryRequirements = nullptr);

        void Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
        {
            VK_CHECK_RESULT(vkMapMemory(_device.LogicalDevice(), _memory, offset, size, 0, &mapped));
        }

        void Unmap()
        {
            vkUnmapMemory(_device.LogicalDevice(), _memory);
        }

        const VkBuffer& GetBuffer() const { return _buffer; }

        void*        mapped    = nullptr;
        VkDeviceSize alignment = 0;
    private:
        VkBuffer       _buffer = VK_NULL_HANDLE;
        VkDeviceMemory _memory = VK_NULL_HANDLE;

        const Device&  _device;
    };
}

#endif // VULKAN_BUFFER_H
