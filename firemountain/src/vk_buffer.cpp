#include "vk_buffer.hpp"


fmvk::Buffer::AllocatedBuffer fmvk::Buffer::create_buffer(size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage, VmaAllocator allocator) {
    VkBufferCreateInfo buffer_info = { 
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .size = alloc_size,
        .usage = usage
    };
    VmaAllocationCreateInfo vma_malloc_info = {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = memory_usage
    };

    fmvk::Buffer::AllocatedBuffer buffer;
    VK_CHECK(vmaCreateBuffer(allocator, &buffer_info, &vma_malloc_info, &buffer.buffer, &buffer.allocation, &buffer.info));
    return buffer;
}

void fmvk::Buffer::destroy_buffer(AllocatedBuffer buffer, VmaAllocator allocator) {
    vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
}