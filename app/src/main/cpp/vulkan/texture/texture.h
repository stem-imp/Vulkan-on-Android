#ifndef VULKAN_TEXTURE_H
#define VULKAN_TEXTURE_H

#ifdef __ANDROID__
#include "vulkan_wrapper.h"
#endif
#include "../../log/log.h"
#include "../device.h"
#include "../command.h"
#include "../vulkan_utility.h"

#include "stb_image.h"

using Utility::Log;
using Vulkan::Device;

namespace Vulkan
{
    class Texture
    {
    public:
        typedef struct TextureAttribs {
            uint32_t            width = 0;
            uint32_t            height = 0;
            uint32_t            channelsPerPixel;
            uint32_t            mipmapLevels = 1;
            VkSamplerMipmapMode samplerMipmapMode;
            bool                sRGB;
            VkFormat            format;
        } TextureAttribs;

        uint32_t ArrayLayers()
        {
            return ArrayLayersImpl();
        }

        const VkImageView& ImageView() const { return view; }

    protected:
        Texture(const Device& device) : device(device) { DebugLog("Texture()"); }
        ~Texture()
        {
            DebugLog("~Texture()");
            VkDevice d = device.LogicalDevice();
            if (view != VK_NULL_HANDLE) {
                DebugLog("~Texture() vkDestroyImageView()");
                vkDestroyImageView (d, view, nullptr);
                view = VK_NULL_HANDLE;
            }
            if (image != VK_NULL_HANDLE) {
                DebugLog("~Texture() vkDestroyImage()");
                vkDestroyImage(d, image, nullptr);
                image = VK_NULL_HANDLE;
            }
            if (memory != VK_NULL_HANDLE) {
                DebugLog("~Texture() vkFreeMemory()");
                vkFreeMemory(d, memory, nullptr);
                memory = VK_NULL_HANDLE;
            }
        }

        void AllocateTexture(VkMemoryRequirements& memoryRequirements, VkMemoryPropertyFlags preferredProperties, const void* pNext = nullptr);
        void BindTexture(VkDeviceSize memoryOffset = 0)
        {
            VK_CHECK_RESULT(vkBindImageMemory(device.LogicalDevice(), image, memory, memoryOffset));
        }
        void CreateImageView(const TextureAttribs& textureAttribs);

        void GenerateMipmaps(TextureAttribs& textureAttribs, Command& command, VkCommandBuffer commandBuffer);

        VkImage        image        = VK_NULL_HANDLE;
        VkDeviceMemory memory       = VK_NULL_HANDLE;
        VkImageView    view         = VK_NULL_HANDLE;

        const Device& device;
    private:
        virtual uint32_t ArrayLayersImpl() = 0;
    };

    class Texture2D : public Texture
    {
    public:
        Texture2D(const Device& device) : Texture(device) { DebugLog("Texture2D()"); }
        Texture2D(Texture2D&& other) : Texture(other.device)
        {
            DebugLog("Texture2D(Texture2D&&)");
            view   = other.view  , other.view   = VK_NULL_HANDLE;
            image  = other.image , other.image  = VK_NULL_HANDLE;
            memory = other.memory, other.memory = VK_NULL_HANDLE;
        }
        ~Texture2D()
        {
            DebugLog("~Texture2D()");
            Texture::~Texture();
        }

        void BuildTexture2D(TextureAttribs& textureAttribs, uint8_t* data, VkMemoryPropertyFlags preferredProperties, Command& command);
    private:
        void CreateTexure2D(TextureAttribs& textureAttribs);
        uint32_t ArrayLayersImpl() override { return 1; }
    };
}

#endif // VULKAN_TEXTURE_H
