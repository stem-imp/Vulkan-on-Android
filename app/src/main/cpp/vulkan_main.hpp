#ifndef VULKAN_MAIN_HPP
#define VULKAN_MAIN_HPP

#include "layer_extension.hpp"
#include "glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "gtx/hash.hpp"
#include <android_native_app_glue.h>
#include <map>
#include <set>
#include <array>
#include <functional>

using glm::vec2;
using glm::vec3;
using std::map;
using std::set;
using std::array;
using glm::mat4;

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
    VkSampleCountFlagBits sampleCount;
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

typedef struct ImageInfo {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
} ImageInfo;

typedef struct SwapchainInfo {
    VkSwapchainKHR swapchain;
    VkExtent2D extent;
    VkFormat format;

    vector<VkImage> images;
    vector<VkImageView> views;
    vector<VkFramebuffer> framebuffers;

    vector<ImageInfo> depthImageInfo;

    ImageInfo msaa;
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

typedef struct VertexV1 {
    vec3 pos;
    vec3 color;
    vec2 texCoord;

    bool operator ==(const VertexV1& rhs) const
    {
        return pos == rhs.pos && color == rhs.color && texCoord == rhs.texCoord;
    }

    static VkVertexInputBindingDescription GetBindingDescription(uint32_t binding)
    {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = binding;
        bindingDescription.stride = sizeof(VertexV1);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions(uint32_t binding)
    {
        array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

        attributeDescriptions[0].binding = binding;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(VertexV1, pos);

        attributeDescriptions[1].binding = binding;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(VertexV1, color);

        attributeDescriptions[2].binding = binding;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(VertexV1, texCoord);

        return attributeDescriptions;
    }
} VertexV1;

namespace std
{
    template<>
    struct hash<VertexV1> {
        size_t operator ()(VertexV1 const& vertex) const
        {
            return ((std::hash<vec3>{}(vertex.pos) ^ (std::hash<vec3>{}(vertex.color) << 1)) >> 1) ^ (std::hash<vec2>{}(vertex.texCoord) << 1);
        }
    };
}

typedef struct MVP {
    mat4 model;
    mat4 view;
    mat4 projection;
} MVP;

typedef struct ResourceDescriptor {
    VkDescriptorPool pool;
    vector<VkDescriptorSet> sets;
    vector<VkDescriptorSetLayout> layouts;
} ResourceDescriptor;

typedef struct TextureObject {
    VkSampler sampler;
    ImageInfo imageInfo;
    uint32_t tex_width;
    uint32_t tex_height;
    uint32_t mipmapLevel;
    VkSamplerMipmapMode mipmapMode;
} TextureOject;

// Initialize vulkan device context
// after return, vulkan is ready to draw
bool InitVulkan(android_app* app, InstanceInfo& instanceInfo, SwapchainInfo& swapchainInfo, VkRenderPass& renderPass, set<VkCommandPool>& commandPools, vector<CommandInfo>& commandInfos, vector<DrawSyncPrimitives>& primitives, vector<VertexV1>& vertices, vector<uint32_t>& indices, BufferInfo& bufferInfo, PipelineInfo& pipelineInfo, vector<VkBuffer>& uniformBuffers, vector<VkDeviceMemory>& uniformBuffersMemory, ResourceDescriptor& transformDescriptor, set<VkDescriptorPool>& descriptorPools, TextureOject& textureObject);

VkResult DrawFrame(android_app* app, InstanceInfo& instanceInfo, SwapchainInfo& swapchainInfo, VkRenderPass& renderPass, vector<CommandInfo>& commandInfos, PipelineInfo& pipelineInfo, vector<DrawSyncPrimitives>& primitives, const BufferInfo& bufferInfo, const vector<uint32_t>& indices, vector<VkDeviceMemory>& mvpMemory, ResourceDescriptor& resourceDescriptor);
void DeleteVulkan(InstanceInfo& instanceInfo, set<VkCommandPool>& commandPools, vector<CommandInfo>& commandInfos, PipelineInfo& pipelineInfo, VkRenderPass& renderPass, SwapchainInfo& swapchainInfo, vector<DrawSyncPrimitives>& primitives, BufferInfo& bufferInfo, vector<VkBuffer>& uniformBuffers, vector<VkDeviceMemory>& uniformBuffersMemory, ResourceDescriptor& resourceDescriptor, set<VkDescriptorPool>& descriptorPools, TextureOject& textureObject);

#endif // VULKAN_MAIN_HPP
