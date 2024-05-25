#include "vk_image.hpp"
#include "vk_init.hpp"
#include "vk_buffer.hpp"


fmvk::Image::AllocatedImage fmvk::Image::create_image(VkDevice device, VmaAllocator allocator, VkExtent3D size, VkFormat format, VkImageUsageFlags usage_flags, bool mipmapped) {
    fmvk::Image::AllocatedImage new_image;

    new_image.format = format;
    VkImageCreateInfo image_info = VKInit::image_create_info(new_image.format, usage_flags, size);
    if (mipmapped) {
        image_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
    }
    VmaAllocationCreateInfo allocation_info = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };
    VK_CHECK(vmaCreateImage(allocator, &image_info, &allocation_info, &new_image.image, &new_image.allocation, nullptr));

    VkImageAspectFlags aspect_flag = VK_IMAGE_ASPECT_COLOR_BIT;
    if (format == VK_FORMAT_D32_SFLOAT) {
        aspect_flag = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    VkImageViewCreateInfo view_info = VKInit::imageview_create_info(new_image.format, new_image.image, aspect_flag);
    view_info.subresourceRange.levelCount = image_info.mipLevels;
    VK_CHECK(vkCreateImageView(device, &view_info, nullptr, &new_image.view));

    return new_image;
}

void fmvk::Image::destroy_image(fmvk::Image::AllocatedImage image, VkDevice device, VmaAllocator allocator)
{
    vkDestroyImageView(device, image.view, nullptr);
    vmaDestroyImage(allocator, image.image, image.allocation);
}


void fmvk::Image::transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout) {
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

void fmvk::Image::copy_image_to_image(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D src_size, VkExtent2D dst_size) {
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

void fmvk::Image::generate_mipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D image_size)
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
                    .mipLevel = (uint32_t) mip,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                },
                .dstSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = (uint32_t) (mip + 1),
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
