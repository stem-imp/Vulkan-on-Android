#include "buffer.h"

namespace Vulkan
{
    Buffer::Buffer(Buffer&& other) : _device(other._device)
    {
        DebugLog("Buffer(Buffer&&)");
        mapped    = other.mapped   , other.mapped    = nullptr;
        alignment = other.alignment, other.alignment = 0;
        _buffer   = other._buffer  , other._buffer   = VK_NULL_HANDLE;
        _memory   = other._memory  , other._memory   = VK_NULL_HANDLE;
    }

    Buffer::~Buffer()
    {
        DebugLog("~Buffer()");
        Free();
    }

    void Buffer::CreateBuffer(VkDeviceSize        size,
                              VkBufferUsageFlags  usage,

                              const void*         pNext,
                              VkBufferCreateFlags flags,
                              VkSharingMode       sharingMode,
                              uint32_t            queueFamilyIndexCount,
                              const uint32_t*     pQueueFamilyIndices)
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.pNext                 = pNext;
        bufferInfo.flags                 = flags;
        bufferInfo.size                  = size;
        bufferInfo.usage                 = usage;
        bufferInfo.sharingMode           = sharingMode;
        bufferInfo.queueFamilyIndexCount = queueFamilyIndexCount;
        bufferInfo.pQueueFamilyIndices   = pQueueFamilyIndices;
        VK_CHECK_RESULT(vkCreateBuffer(_device.LogicalDevice(), &bufferInfo, nullptr, &_buffer));
    }

    void Buffer::AllocateBuffer(VkMemoryRequirements& memoryRequirements, VkMemoryPropertyFlags preferredProperties, const void* pNext)
    {
        VkDevice device = _device.LogicalDevice();

        vkGetBufferMemoryRequirements(device, _buffer, &memoryRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext           = pNext;
        allocInfo.allocationSize  = memoryRequirements.size;
        uint32_t memoryTypeIndex = MapMemoryTypeToIndex(_device.PhysicalDevice(), memoryRequirements.memoryTypeBits, preferredProperties);
        allocInfo.memoryTypeIndex = memoryTypeIndex;
        VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &_memory));
    }

    void Buffer::BuildDefaultBuffer(VkDeviceSize          size,
                                    VkBufferUsageFlags    usage,
                                    VkMemoryPropertyFlags preferredProperties,
                                    VkMemoryRequirements* memoryRequirements)
    {
        CreateBuffer(size, usage);
        if (!memoryRequirements) {
            VkMemoryRequirements memRequirements;
            AllocateBuffer(memRequirements, preferredProperties);
        } else {
            AllocateBuffer(*memoryRequirements, preferredProperties);
        }
        BindBuffer();
    }

    void Buffer::BuildExtendedBuffer(VkDeviceSize             size,
                                     VkBufferUsageFlags       usage,
                                     VkMemoryPropertyFlags    preferredProperties,
                                     ExtendedBufferParameter& extendedBufferParameter,
                                     VkMemoryRequirements* memoryRequirements)
    {
        ExtendedBufferParameter& p = extendedBufferParameter;
        CreateBuffer(size, usage, p.create.pNext, p.create.flags, p.create.sharingMode, p.create.queueFamilyIndexCount, p.create.pQueueFamilyIndices);
        if (!memoryRequirements) {
            VkMemoryRequirements memRequirements;
            AllocateBuffer(memRequirements, preferredProperties);
        } else {
            AllocateBuffer(*memoryRequirements, preferredProperties);
        }
        BindBuffer(p.allocateAndBind.memoryOffset);
    }

    void Buffer::Free()
    {
        DebugLog("Buffer::Free()");
        VkDevice device = _device.LogicalDevice();
        if (_buffer != VK_NULL_HANDLE) {
            DebugLog("Buffer::Free() vkDestroyBuffer()");
            vkDestroyBuffer(device, _buffer, nullptr);
            _buffer = VK_NULL_HANDLE;
        }
        if (_memory != VK_NULL_HANDLE) {
            DebugLog("Buffer::Free() vkFreeMemory()");
            vkFreeMemory(device, _memory, nullptr);
            _memory = VK_NULL_HANDLE;
        }
    }
}
