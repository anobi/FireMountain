#pragma once

#include "vk_types.hpp"

namespace VKInit {
    VkCommandPoolCreateInfo command_pool_create_info(
        uint32_t queue_family_index,
        VkCommandPoolCreateFlags flags = 0
    );

    VkCommandBufferAllocateInfo command_buffer_allocate_info(
        VkCommandPool pool,
        uint32_t count = 1,
        VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
    );
}