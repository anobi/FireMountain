#pragma once

#include <vulkan/vulkan.h>

namespace VKUtil {
    void transition_image(
        VkCommandBuffer cmd, VkImage image, 
        VkImageLayout current_layout, VkImageLayout new_layout
    );

    void copy_image_to_image(
        VkCommandBuffer cmd, 
        VkImage src, 
        VkImage dst, 
        VkExtent2D src_size, 
        VkExtent2D dst_size
    );

    void generate_mipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D image_size);
}