#ifndef VULKAN_COMMANDS_H
#define VULKAN_COMMANDS_H

#ifdef __ANDROID__
#include "vulkan_wrapper.h"
#endif
#include "device.h"

namespace Vulkan
{
    class Command {
    public:
        ~Command();

        static vector<VkCommandBuffer> CreateCommandBuffers(VkCommandPool commandPool,
                                                            VkCommandBufferLevel level,
                                                            uint32_t commandBufferCount,
                                                            VkDevice device,
                                                            const void *pNext = nullptr);

        static void BeginCommandBuffer(VkCommandBuffer           commandBuffer,
                                       VkCommandBufferUsageFlags flags,
                                       VkDevice                  device,

                                       const VkCommandBufferInheritanceInfo* pInheritanceInfo = nullptr,
                                       const void*                           pCreateNext      = nullptr,
                                       const void*                           pBeginNext       = nullptr);

        static vector<VkCommandBuffer> CreateAndBeginCommandBuffers(VkCommandPool commandPool,
                                                                    VkCommandBufferLevel level,
                                                                    uint32_t commandBufferCount,
                                                                    VkCommandBufferUsageFlags flags,
                                                                    VkDevice device,
                                                                    const VkCommandBufferInheritanceInfo *pInheritanceInfo = nullptr,
                                                                    const void *pCreateNext = nullptr,
                                                                    const void *pBeginNext = nullptr);

        static void EndAndSubmitCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool commandPool, VkDevice device);

        typedef struct CommandPools {
            int graphicsQueueIndex      = -1;
            int computeQueueIndex       = -1;
            int transferQueueIndex      = -1;
            int sparseBindingQueueIndex = -1;
            VkCommandPool graphics      = VK_NULL_HANDLE;
            VkCommandPool compute       = VK_NULL_HANDLE;
            VkCommandPool transfer      = VK_NULL_HANDLE;
            VkCommandPool sparseBinding = VK_NULL_HANDLE;
            vector<VkCommandBuffer> graphicsCmds;
            vector<VkCommandBuffer> computeCmds;
            vector<VkCommandBuffer> transferCmds;
            vector<VkCommandBuffer> sparseBindingCmds;
        } CommandPools;

        typedef struct CommandBuffers {
            vector<VkCommandBuffer> buffers;
        } CommandBuffers;

        void BuildCommandPools(VkCommandPoolCreateFlags poolFlags, const Vulkan::Device& device);

        VkCommandPool GeneralGraphcisPool() { return _generalPool.graphics; }
        VkCommandPool GeneralComputePool()
        {
            if (_generalPool.compute) {
                return _generalPool.compute;
            } else if (_generalPool.computeQueueIndex != -1) {
                return _generalPool.graphics;
            } else {
                return VK_NULL_HANDLE;
            }
        }
        VkCommandPool GeneralTransferPool()
        {
            if (_generalPool.transfer) {
                return _generalPool.transfer;
            } else if (_generalPool.transferQueueIndex != -1) {
                return _generalPool.graphics;
            } else {
                return VK_NULL_HANDLE;
            }
        }
        VkCommandPool GeneralSparseBindingPool()
        {
            if (_generalPool.sparseBinding) {
                return _generalPool.sparseBinding;
            } else if (_generalPool.sparseBindingQueueIndex != -1) {
                return _generalPool.graphics;
            } else {
                return VK_NULL_HANDLE;
            }
        }
        VkCommandPool ShortLivedGraphcisPool() { return _shortLivedPool.graphics; }
        VkCommandPool ShortLivedComputePool()
        {
            if (_shortLivedPool.compute) {
                return _shortLivedPool.compute;
            } else if (_shortLivedPool.computeQueueIndex != -1) {
                return _shortLivedPool.graphics;
            } else {
                return VK_NULL_HANDLE;
            }
        }
        VkCommandPool ShortLivedTransferPool()
        {
            if (_shortLivedPool.transfer) {
                return _shortLivedPool.transfer;
            } else if (_shortLivedPool.transferQueueIndex != -1) {
                return _shortLivedPool.graphics;
            } else {
                return VK_NULL_HANDLE;
            }
        }
        VkCommandPool ShortLivedSparseBindingPool()
        {
            if (_shortLivedPool.sparseBinding) {
                return _shortLivedPool.sparseBinding;
            } else if (_shortLivedPool.sparseBindingQueueIndex != -1) {
                return _shortLivedPool.graphics;
            } else {
                return VK_NULL_HANDLE;
            }
        }

        vector<VkCommandBuffer>& GeneralGraphcisCommands() { return _generalPool.graphicsCmds; }
        vector<VkCommandBuffer>& GeneralComputeCommands()
        {
            if (_generalPool.compute) {
                return _generalPool.computeCmds;
            } else if (_generalPool.computeQueueIndex != -1) {
                return _generalPool.graphicsCmds;
            } else {
                assert("No general compute commands." && false);
            }
        }
        vector<VkCommandBuffer>& GeneralTransferCommands()
        {
            if (_generalPool.transfer) {
                return _generalPool.transferCmds;
            } else if (_generalPool.transferQueueIndex != -1) {
                return _generalPool.graphicsCmds;
            } else {
                assert("No general compute commands." && false);
            }
        }
        vector<VkCommandBuffer>& GeneralSparseBindingCommands()
        {
            if (_generalPool.sparseBinding) {
                return _generalPool.sparseBindingCmds;
            } else if (_generalPool.sparseBindingQueueIndex != -1) {
                return _generalPool.graphicsCmds;
            } else {
                assert("No general sparse binding commands." && false);
            }
        }
        vector<VkCommandBuffer>& ShortLivedGraphcisCommands() { return _shortLivedPool.graphicsCmds; }
        vector<VkCommandBuffer>& ShortLivedComputeCommands()
        {
            if (_shortLivedPool.compute) {
                return _shortLivedPool.computeCmds;
            } else if (_shortLivedPool.computeQueueIndex != -1) {
                return _shortLivedPool.graphicsCmds;
            } else {
                assert("No short-lived compute commands." && false);
            }
        }
        vector<VkCommandBuffer>& ShortLivedTransferCommands()
        {
            if (_shortLivedPool.transfer) {
                return _shortLivedPool.transferCmds;
            } else if (_shortLivedPool.transferQueueIndex != -1) {
                return _shortLivedPool.graphicsCmds;
            } else {
                assert("No short-lived transfer commands." && false);
            }
        }
        vector<VkCommandBuffer>& ShortLivedSparseBindingCommands()
        {
            if (_shortLivedPool.sparseBinding) {
                return _shortLivedPool.sparseBindingCmds;
            } else if (_shortLivedPool.sparseBindingQueueIndex != -1) {
                return _shortLivedPool.graphicsCmds;
            } else {
                assert("No short-lived sparse binding commands." && false);
            }
        }

    private:
        void CreateCommandPool(VkCommandPoolCreateFlags poolFlags, VkQueueFlags queueFlags, const Vulkan::Device& device);

        CommandPools _generalPool;
        CommandPools _shortLivedPool;

        VkDevice _device;
    };
}

#endif // VULKAN_COMMANDS_H
