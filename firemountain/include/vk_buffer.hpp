#pragma once

#include "vk_types.hpp"


namespace fmvk {
    namespace Buffer {
        struct AllocatedBuffer {
            VkBuffer buffer;
            VmaAllocation allocation;
            VmaAllocationInfo info;
        };

        AllocatedBuffer create_buffer(size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage,  VmaAllocator allocator);
        void destroy_buffer(AllocatedBuffer buffer, VmaAllocator allocator);
    };
};