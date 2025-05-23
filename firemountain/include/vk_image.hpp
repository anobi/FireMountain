#pragma once

#include <functional>
#include "vk_types.hpp"

namespace fmvk {
    namespace Image {
        struct AllocatedImage {
            VkImage image;
            VkImageView view;
            VmaAllocation allocation;
            VkExtent3D extent;
            VkFormat format;
        };

        AllocatedImage create_image(VkDevice device, VmaAllocator allocator, VkExtent3D size, VkFormat format, VkImageUsageFlags usage_flags, bool mipmapped = false);
        void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout);
        void copy_image_to_image(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D src_size, VkExtent2D dst_size);
        void generate_mipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D image_size);
        void destroy_image(AllocatedImage &image, VkDevice device, VmaAllocator allocator);
    }
}