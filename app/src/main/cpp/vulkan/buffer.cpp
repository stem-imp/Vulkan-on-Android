#include "buffer.h"

namespace Vulkan
{
    Buffer::Buffer(Buffer&& other) : _device(other._device)
    {
        DebugLog("Buffer(Buffer&&)");
        mapped    = other.mapped   , other.mapped    = nullptr;
        alignment = other.alignment, other.alignment = 0;
        _buffer   = other._buffer  , other._buffer   = VK_NULL_HANDLE;
        _memory   = other._memory  , _memory         = VK_NULL_HANDLE;
    }

    Buffer::~Buffer()
    {
        DebugLog("~Buffer()");
        VkDevice device = _device.LogicalDevice();
        if (_buffer != VK_NULL_HANDLE) {
            DebugLog("~Buffer() vkDestroyBuffer()");
            vkDestroyBuffer(device, _buffer, nullptr);
            _buffer = VK_NULL_HANDLE;
        }
        if (_memory != VK_NULL_HANDLE) {
            DebugLog("~Buffer() vkFreeMemory()");
            vkFreeMemory(device, _memory, nullptr);
            _memory = VK_NULL_HANDLE;
        }
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

    void Buffer::AllocateBuffer(VkDeviceSize          allocationSize,
                                uint32_t              memoryTypeIndex,

                                const void*           pNext,
                                VkMemoryRequirements* memoryRequirements)
    {
        VkDevice device = _device.LogicalDevice();
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, _buffer, &memRequirements);
        if (memoryRequirements) {
            *memoryRequirements = memRequirements;
        }

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext           = pNext;
        allocInfo.allocationSize  = allocationSize;
        allocInfo.memoryTypeIndex = memoryTypeIndex;
        VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &_memory));
    }

    void Buffer::BuildDefaultBuffer(VkDeviceSize          size,
                                    VkBufferUsageFlags    usage,
                                    VkMemoryPropertyFlags preferredProperties)
    {
        CreateBuffer(size, usage);
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(_device.LogicalDevice(), _buffer, &memRequirements);
        uint32_t memoryTypeIndex;
        memoryTypeIndex = MapMemoryTypeToIndex(_device.PhysicalDevice(), memRequirements.memoryTypeBits, preferredProperties);
        AllocateBuffer(memRequirements.size, memoryTypeIndex);
        BindBuffer();
    }

    void Buffer::BuildExtendedBuffer(VkDeviceSize            size,
                                     VkBufferUsageFlags      usage,
                                     VkMemoryPropertyFlags   preferredProperties,
                                     ExtendedBufferParameter extendedBufferParameter)
    {
        ExtendedBufferParameter& p = extendedBufferParameter;
        CreateBuffer(size, usage, p.create.pNext, p.create.flags, p.create.sharingMode, p.create.queueFamilyIndexCount, p.create.pQueueFamilyIndices);
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(_device.LogicalDevice(), _buffer, &memRequirements);
        uint32_t memoryTypeIndex;
        memoryTypeIndex = MapMemoryTypeToIndex(_device.PhysicalDevice(), memRequirements.memoryTypeBits, preferredProperties);
        AllocateBuffer(memRequirements.size, memoryTypeIndex, p.allocateAndBind.pNext, p.allocateAndBind.memoryRequirements);
        BindBuffer(p.allocateAndBind.memoryOffset);
    }
}
