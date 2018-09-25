#ifndef VULKAN_MAIN_HPP
#define VULKAN_MAIN_HPP

#include <android_native_app_glue.h>
#include "layer_extension.hpp"
#include <map>
#include <set>

using std::map;
using std::set;

extern const int VK_QUEUE_PRESENT_BIT;

typedef struct QueueFamilyIndex {
    VkQueueFlagBits flag;
    int index;
} QueueFamilyIndex;

typedef struct QueueInfo {
    VkQueue queue;
    uint32_t index; // index in the queue family.
} QueueInfo;

typedef struct DeviceInfo {
    VkPhysicalDevice physicalDevice;
    vector<VkDevice> logicalDevices;
    map<int, uint32_t> familyToIndex;
    map<uint32_t, set<int>> indexToFamily;
    map<VkDevice, map<int, vector<QueueInfo>>> queues;
    bool sharedGraphicsAndPresentQueue;
} DeviceInfo;

typedef struct InstanceInfo {
    int32_t width;
    int32_t height;
    VkInstance instance;
    vector<DeviceInfo> devices;
    VkSurfaceKHR surface;
    bool initialized;
    bool resolutionChanged;
} InstanceInfo;

typedef struct SwapchainInfo {
    VkSwapchainKHR swapchain;
    VkExtent2D extent;
    VkFormat format;

    vector<VkImage> images;
    vector<VkImageView> views;
    vector<VkFramebuffer> framebuffers;
} SwapchainInfo;

typedef struct CommandInfo {
    VkCommandPool pool;
    vector<VkCommandBuffer> buffer;
} CommandInfo;

typedef struct PipelineInfo {
    VkPipelineLayout layout;
    VkPipeline pipeline;
    VkPipelineCache cache;
} PipelineInfo;

typedef struct BufferInfo {
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
} BufferInfo;

typedef struct DrawSyncPrimitives {
    size_t concurrentFrameCount;
    vector<VkSemaphore> imageAvailableSemaphores;
    vector<VkSemaphore> renderFinishedSemaphores;
    vector<VkFence> inFlightFences;
    size_t currentFrame = 0;
} DrawSyncPrimitives;

// Initialize vulkan device context
// after return, vulkan is ready to draw
bool InitVulkan(android_app* app, InstanceInfo& instanceInfo, SwapchainInfo& swapchainInfo, VkRenderPass& renderPass, set<VkCommandPool>& commandPools, vector<CommandInfo>& commandInfos, vector<DrawSyncPrimitives>& primitives);

VkResult DrawFrame(InstanceInfo& instanceInfo, SwapchainInfo& swapchainInfo, VkRenderPass& renderPass, vector<CommandInfo>& commandInfos, vector<DrawSyncPrimitives>& primitives);
void DeleteVulkan(InstanceInfo& instanceInfo, set<VkCommandPool>& commandPools, vector<CommandInfo>& commandInfos, VkRenderPass& renderPass, SwapchainInfo& swapchainInfo, vector<DrawSyncPrimitives>& primitives);

#endif // VULKAN_MAIN_HPP
