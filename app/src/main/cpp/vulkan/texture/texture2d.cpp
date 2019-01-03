#include "texture.h"
#include "../vulkan_utility.h"
#include "../buffer.h"
#include <algorithm>

namespace Vulkan
{
    void Texture2D::BuildTexture2D(TextureAttribs& textureAttribs, uint8_t* data, VkMemoryPropertyFlags preferredProperties, Command& command)
    {
        Buffer stagingBuffer(device);
        size_t size = textureAttribs.width * textureAttribs.height * textureAttribs.channelsPerPixel;
        stagingBuffer.BuildDefaultBuffer(size,
                                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        stagingBuffer.Map();
        std::copy(data, data + size, (uint8_t*)stagingBuffer.mapped);
        stagingBuffer.Unmap();
        stbi_image_free(data);

        CreateTexure2D(textureAttribs);
        VkMemoryRequirements memRequirements;
        AllocateTexture(memRequirements, preferredProperties);
        BindTexture();
        CreateImageView(textureAttribs);

        vector<VkCommandBuffer> cmds = Command::CreateAndBeginCommandBuffers(command.ShortLivedTransferPool(),
                                                                             VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                                             1,
                                                                             VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                                                                             device.LogicalDevice());
        PipelineBarrierParameters pipelineBarrierParameters = {};
        pipelineBarrierParameters.commandBuffer           = cmds[0];
        pipelineBarrierParameters.imageMemoryBarrierCount = 1;
        pipelineBarrierParameters.srcStageMask            = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        pipelineBarrierParameters.dstStageMask            = VK_PIPELINE_STAGE_TRANSFER_BIT;
        VkImageMemoryBarrier barrier = ImageMemoryBarrier(0,
                                                          VK_ACCESS_TRANSFER_WRITE_BIT,
                                                          VK_IMAGE_LAYOUT_UNDEFINED,
                                                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                          image,
                                                          { VK_IMAGE_ASPECT_COLOR_BIT, 0, textureAttribs.mipmapLevels, 0, 1 });
        pipelineBarrierParameters.pImageMemoryBarriers = &barrier;
        TransitionImageLayout(&pipelineBarrierParameters);
        VkBufferImageCopy region = BufferImageCopy({ textureAttribs.width, textureAttribs.height, 1 },
                                                   { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 });
        vkCmdCopyBufferToImage(cmds[0], stagingBuffer.GetBuffer(), image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        Command::EndAndSubmitCommandBuffer(cmds[0], device.FamilyQueues().transfer.queue, command.ShortLivedTransferPool(), device.LogicalDevice(), false);

        stagingBuffer.Free();


        Command::BeginCommandBuffer(cmds[0], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        GenerateMipmaps(textureAttribs, command, cmds[0]);
        Command::EndAndSubmitCommandBuffer(cmds[0] , device.FamilyQueues().transfer.queue, command.ShortLivedTransferPool(), device.LogicalDevice());
    }

    void Texture2D::CreateTexure2D(TextureAttribs& textureAttribs)
    {
        vector<VkFormat> formats;
        if (textureAttribs.channelsPerPixel == 3) {
            formats.push_back(VK_FORMAT_R8G8B8_UNORM);
        } else if (textureAttribs.channelsPerPixel == 4) {
            formats.push_back(VK_FORMAT_R8G8B8A8_UNORM);
        }
        textureAttribs.format = FindDeviceSupportedFormat(formats,
                                                          VK_IMAGE_TILING_OPTIMAL,
                                                          VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT,
                                                          device.PhysicalDevice());
        VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if (textureAttribs.mipmapLevels > 1) {
            usage = usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        VkImageCreateInfo imageInfo = ImageCreateInfo(textureAttribs.format,
                                                      { textureAttribs.width, textureAttribs.height, 1 },
                                                      textureAttribs.mipmapLevels,
                                                      usage);
        VK_CHECK_RESULT(vkCreateImage(device.LogicalDevice(), &imageInfo, nullptr, &image));
    }
}