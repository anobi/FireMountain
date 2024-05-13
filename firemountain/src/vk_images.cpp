#include "vk_images.hpp"
#include "vk_init.hpp"


void VKUtil::transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout) {
    VkImageAspectFlags aspect_mask = (new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
        ? VK_IMAGE_ASPECT_DEPTH_BIT
        : VK_IMAGE_ASPECT_COLOR_BIT;
    
    VkImageMemoryBarrier2 image_barrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
        .oldLayout = current_layout,
        .newLayout = new_layout,
        .image = image,
        .subresourceRange = VKInit::image_subresource_range(aspect_mask)
    };

    VkDependencyInfo dependency_info = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &image_barrier
    };

    vkCmdPipelineBarrier2(cmd, &dependency_info);
}

void VKUtil::copy_image_to_image(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D src_size, VkExtent2D dst_size) {
    VkImageBlit2 blit_region = { 
        .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
        .pNext = nullptr,
        .srcSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .dstSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    blit_region.srcOffsets[1] = {
        .x = static_cast<int32_t>(src_size.width),
        .y = static_cast<int32_t>(src_size.height),
        .z = 1
    };
    blit_region.dstOffsets[1] = {
        .x = static_cast<int32_t>(dst_size.width),
        .y = static_cast<int32_t>(dst_size.height),
        .z = 1
    };

    VkBlitImageInfo2 blit_info = {
        .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
        .pNext = nullptr,
        .srcImage = src,
        .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .dstImage = dst,
        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .regionCount = 1,
        .pRegions = &blit_region,
        .filter = VK_FILTER_LINEAR
    };

    vkCmdBlitImage2(cmd, &blit_info);
}

void VKUtil::generate_mipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D image_size)
{
    int mip_levels = int(std::floor(std::log2(std::max(image_size.width, image_size.height)))) + 1;
    for (int mip = 0; mip < mip_levels; mip++) {
        VkExtent2D half_size = VkExtent2D { 
            .width = image_size.width / 2,
            .height = image_size.height / 2
        };

        
        VkImageMemoryBarrier2 image_barrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .image = image
        };

        VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_barrier.subresourceRange = VKInit::image_subresource_range(aspect_mask);
        image_barrier.subresourceRange.levelCount = 1;
        image_barrier.subresourceRange.baseMipLevel = mip;

        VkDependencyInfo dep_info = {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &image_barrier
        };
        vkCmdPipelineBarrier2(cmd, &dep_info);

        if (mip < mip_levels - 1) {
            VkImageBlit2 blit_region {
                .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
                .pNext = nullptr,
                .srcSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = mip,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                },
                .dstSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = mip + 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                },
            };

            blit_region.srcOffsets[1].x = image_size.width;
            blit_region.srcOffsets[1].y = image_size.height;
            blit_region.srcOffsets[1].z = 1;

            blit_region.dstOffsets[1].x = half_size.width;
            blit_region.dstOffsets[1].y = half_size.height;
            blit_region.dstOffsets[1].z = 1;

            VkBlitImageInfo2 blit_info {
                .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
                .pNext = nullptr,
                .srcImage = image,
                .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .dstImage = image,
                .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .regionCount = 1,
                .pRegions = &blit_region
            };

            vkCmdBlitImage2(cmd, &blit_info);
            image_size = half_size;
        }
    }
    transition_image(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}
