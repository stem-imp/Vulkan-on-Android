#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#include "texture.h"
#include "../../log/log.h"

using Utility::Log;

namespace Vulkan
{
    void Texture::AllocateTexture(VkMemoryRequirements& memoryRequirements, VkMemoryPropertyFlags preferredProperties, const void* pNext)
    {
        VkDevice d = device.LogicalDevice();
        vkGetImageMemoryRequirements(d, image, &memoryRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext           = pNext;
        allocInfo.allocationSize  = memoryRequirements.size;
        uint32_t memoryTypeIndex = MapMemoryTypeToIndex(device.PhysicalDevice(), memoryRequirements.memoryTypeBits, preferredProperties);
        allocInfo.memoryTypeIndex = memoryTypeIndex;
        VK_CHECK_RESULT(vkAllocateMemory(d, &allocInfo, nullptr, &memory));
    }

    void Texture::CreateImageView(const TextureAttribs& textureAttribs)
    {
        VkImageViewCreateInfo imageViewInfo = ImageViewCreateInfo(image, textureAttribs.format, { VK_IMAGE_ASPECT_COLOR_BIT, 0, textureAttribs.mipmapLevels, 0, 1 });
        vkCreateImageView(device.LogicalDevice(), &imageViewInfo, nullptr, &view);
    }

    void Texture::GenerateMipmaps(TextureAttribs& textureAttribs, Command& command, VkCommandBuffer commandBuffer)
    {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(device.PhysicalDevice(), textureAttribs.format, &formatProperties);

        if ((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            textureAttribs.samplerMipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        } else {
            Log::Info("Texture image format does not support linear blitting.");
            textureAttribs.samplerMipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        }

        VkImageMemoryBarrier barrier = ImageMemoryBarrier(0, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED, image);

        int32_t mipWidth = (int32_t)textureAttribs.width;
        int32_t mipHeight = (int32_t)textureAttribs.height;

        for (uint32_t i = 1; i < textureAttribs.mipmapLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            VkImageBlit blit = {};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = { 0, 0, 0 };
            mipWidth = mipWidth > 1 ? mipWidth / 2 : 1;
            mipHeight = mipHeight > 1 ? mipHeight / 2 : 1;
            blit.dstOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        }

        barrier.subresourceRange.baseMipLevel = textureAttribs.mipmapLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }
}