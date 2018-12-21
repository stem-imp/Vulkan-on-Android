#include "command.h"
#include "vulkan_utility.h"

using Vulkan::Command;
using Vulkan::Device;

void CreateCommandPoolImpl(VkCommandPoolCreateFlags poolFlags, VkQueueFlags queueFlags, Command::CommandPools &commandPools, const Device &device);

Command::~Command()
{
    DebugLog("~Command()");
    if (_shortLivedPool.graphics != VK_NULL_HANDLE) {
        DebugLog("~Command() vkDestroyCommandPool() _shortLivedPool.graphics");
        vkDestroyCommandPool(_device, _shortLivedPool.graphics, nullptr);
        _shortLivedPool.graphics = VK_NULL_HANDLE;
    }
    if (_shortLivedPool.compute != VK_NULL_HANDLE) {
        DebugLog("~Command() vkDestroyCommandPool() _shortLivedPool.compute");
        vkDestroyCommandPool(_device, _shortLivedPool.compute, nullptr);
        _shortLivedPool.compute = VK_NULL_HANDLE;
    }
    if (_shortLivedPool.transfer != VK_NULL_HANDLE) {
        DebugLog("~Command() vkDestroyCommandPool() _shortLivedPool.transfer");
        vkDestroyCommandPool(_device, _shortLivedPool.transfer, nullptr);
        _shortLivedPool.transfer = VK_NULL_HANDLE;
    }
    if (_shortLivedPool.sparseBinding != VK_NULL_HANDLE) {
        DebugLog("~Command() vkDestroyCommandPool() _shortLivedPool.sparseBinding");
        vkDestroyCommandPool(_device, _shortLivedPool.sparseBinding, nullptr);
        _shortLivedPool.sparseBinding = VK_NULL_HANDLE;
    }
    if (_generalPool.graphics != VK_NULL_HANDLE) {
        DebugLog("~Command() vkDestroyCommandPool() _generalPool.graphics");
        vkDestroyCommandPool(_device, _generalPool.graphics, nullptr);
        _generalPool.graphics = VK_NULL_HANDLE;
    }
    if (_generalPool.compute != VK_NULL_HANDLE) {
        DebugLog("~Command() vkDestroyCommandPool() _generalPool.compute");
        vkDestroyCommandPool(_device, _generalPool.compute, nullptr);
        _generalPool.compute = VK_NULL_HANDLE;
    }
    if (_generalPool.transfer != VK_NULL_HANDLE) {
        DebugLog("~Command() vkDestroyCommandPool() _generalPool.transfer");
        vkDestroyCommandPool(_device, _generalPool.transfer, nullptr);
        _generalPool.transfer = VK_NULL_HANDLE;
    }
    if (_generalPool.sparseBinding != VK_NULL_HANDLE) {
        DebugLog("~Command() vkDestroyCommandPool() _generalPool.sparseBinding");
        vkDestroyCommandPool(_device, _generalPool.sparseBinding, nullptr);
        _generalPool.sparseBinding = VK_NULL_HANDLE;
    }
}

void Command::BuildCommandPools(VkCommandPoolCreateFlags poolFlags, const Device& device)
{
    CreateCommandPool(poolFlags | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, device.queueFlags, device);
    CreateCommandPool(poolFlags, device.queueFlags, device);
    _device = device.LogicalDevice();
}

void Command::CreateCommandPool(VkCommandPoolCreateFlags poolFlags, VkQueueFlags queueFlags, const Device& device)
{
    const Device::QueueGroup& familyQueues = device.FamilyQueues();
    if (poolFlags & VK_COMMAND_POOL_CREATE_TRANSIENT_BIT) {
        _shortLivedPool.graphicsQueueIndex      = familyQueues.graphics.index;
        _shortLivedPool.computeQueueIndex       = familyQueues.compute.index;
        _shortLivedPool.transferQueueIndex      = familyQueues.transfer.index;
        _shortLivedPool.sparseBindingQueueIndex = familyQueues.sparseBinding.index;
        CreateCommandPoolImpl(poolFlags, queueFlags, _shortLivedPool, device);
    } else {
        _generalPool.graphicsQueueIndex      = familyQueues.graphics.index;
        _generalPool.computeQueueIndex       = familyQueues.compute.index;
        _generalPool.transferQueueIndex      = familyQueues.transfer.index;
        _generalPool.sparseBindingQueueIndex = familyQueues.sparseBinding.index;
        CreateCommandPoolImpl(poolFlags, queueFlags, _generalPool, device);
    }
}

void CreateCommandPoolImpl(VkCommandPoolCreateFlags poolFlags, VkQueueFlags queueFlags, Command::CommandPools &commandPools, const Device &device)
{
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = poolFlags;
    const Device::QueueGroup& familyQueues = device.FamilyQueues();
    if ((queueFlags & VK_QUEUE_GRAPHICS_BIT) && familyQueues.graphics.index != -1) {
        createInfo.queueFamilyIndex = familyQueues.graphics.index;
        VK_CHECK_RESULT(vkCreateCommandPool(device.LogicalDevice(), &createInfo, nullptr, &commandPools.graphics));
    }
    if ((queueFlags & VK_QUEUE_COMPUTE_BIT) && familyQueues.compute.index != -1 && device.DedicatedComputeQueueFamily()) {
        createInfo.queueFamilyIndex = familyQueues.compute.index;
        VK_CHECK_RESULT(vkCreateCommandPool(device.LogicalDevice(), &createInfo, nullptr, &commandPools.compute));
    }
    if ((queueFlags & VK_QUEUE_TRANSFER_BIT) && familyQueues.transfer.index != -1 && device.DedicatedTransferQueueFamily()) {
        createInfo.queueFamilyIndex = familyQueues.transfer.index;
        VK_CHECK_RESULT(vkCreateCommandPool(device.LogicalDevice(), &createInfo, nullptr, &commandPools.transfer));
    }
    if ((queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) && familyQueues.sparseBinding.index != -1 && device.DedicatedSparseBindingQueueFamily()) {
        createInfo.queueFamilyIndex = familyQueues.sparseBinding.index;
        VK_CHECK_RESULT(vkCreateCommandPool(device.LogicalDevice(), &createInfo, nullptr, &commandPools.sparseBinding));
    }
}

vector<VkCommandBuffer> Command::CreateCommandBuffers(VkCommandPool commandPool,
                                                      VkCommandBufferLevel level,
                                                      uint32_t commandBufferCount,
                                                      VkDevice device,
                                                      const void *pNext)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.pNext = pNext;
    allocInfo.commandPool = commandPool;
    allocInfo.level = level;
    allocInfo.commandBufferCount = commandBufferCount;

    vector<VkCommandBuffer> commandBuffers(commandBufferCount);
    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()));

    return commandBuffers;
}

void Command::BeginCommandBuffer(VkCommandBuffer           commandBuffer,
                                 VkCommandBufferUsageFlags flags,
                                 VkDevice                  device,

                                 const VkCommandBufferInheritanceInfo* pInheritanceInfo,
                                 const void*                           pCreateNext,
                                 const void*                           pBeginNext)
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext            = pBeginNext;
    beginInfo.flags            = flags;
    beginInfo.pInheritanceInfo = pInheritanceInfo;
    VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));
}

vector<VkCommandBuffer> Command::CreateAndBeginCommandBuffers(VkCommandPool commandPool,
                                                              VkCommandBufferLevel level,
                                                              uint32_t commandBufferCount,
                                                              VkCommandBufferUsageFlags flags,
                                                              VkDevice device,

                                                              const VkCommandBufferInheritanceInfo *pInheritanceInfo,
                                                              const void *pCreateNext,
                                                              const void *pBeginNext)
{
    vector<VkCommandBuffer> commandBuffer = CreateCommandBuffers(commandPool, level,
                                                                 commandBufferCount, device,
                                                                 pCreateNext);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext            = pBeginNext;
    beginInfo.flags            = flags;
    beginInfo.pInheritanceInfo = pInheritanceInfo;
    for (auto& c : commandBuffer) {
        VK_CHECK_RESULT(vkBeginCommandBuffer(c, &beginInfo));
    }

    return commandBuffer;
}

void Command::EndAndSubmitCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool commandPool, VkDevice device)
{
    VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence;
    VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &fence));

    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
    VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));

    vkDestroyFence(device, fence, nullptr);
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}