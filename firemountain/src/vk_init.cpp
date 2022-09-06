#include <vk_init.hpp>

VkCommandPoolCreateInfo VKInit::command_pool_create_info(
    uint32_t queue_family_index,
    VkCommandPoolCreateFlags flags
) {
    VkCommandPoolCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .queueFamilyIndex = queue_family_index
    };
    return info;
}

VkCommandBufferAllocateInfo VKInit::command_buffer_allocate_info(
    VkCommandPool pool,
    uint32_t count,
    VkCommandBufferLevel level
) {
    VkCommandBufferAllocateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = pool,
        .level = level,
        .commandBufferCount = count
    };
    return info;
}