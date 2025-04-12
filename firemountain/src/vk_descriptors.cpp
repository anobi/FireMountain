#include "vk_descriptors.hpp"


constexpr int MAX_SETS_PER_POOL = 4092;

void DescriptorLayoutBuilder::add_binding(uint32_t binding, VkDescriptorType type)
{
    VkDescriptorSetLayoutBinding new_binding = {
        .binding = binding,
        .descriptorType = type,
        .descriptorCount = 1
    };
    this->bindings.push_back(new_binding);
}

void DescriptorLayoutBuilder::clear()
{
    this->bindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::build(VkDevice device, VkShaderStageFlags shader_stages)
{
    for (auto &b : this->bindings) {
        b.stageFlags |= shader_stages;
    }

    VkDescriptorSetLayoutCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = static_cast<uint32_t>(this->bindings.size()),
        .pBindings = this->bindings.data()
    };

    VkDescriptorSetLayout set;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));
    return set;
}

void DescriptorAllocatorGrowable::init(VkDevice device, uint32_t max_sets, std::span<PoolSizeRatio> pool_ratios)
{
    this->ratios.clear();
    for (auto r : pool_ratios) {
        ratios.push_back(r);
    }

    VkDescriptorPool pool = create_pool(device, max_sets, pool_ratios);
    this->sets_per_pool = max_sets * 1.5;
    this->ready_pools.push_back(pool);
}

void DescriptorAllocatorGrowable::clear_pools(VkDevice device)
{
    for (auto p : this->ready_pools) {
        vkResetDescriptorPool(device, p, 0);
    }
    for (auto p : this->full_pools)  {
        vkResetDescriptorPool(device, p, 0);
        this->ready_pools.push_back(p);
    }
    this->full_pools.clear();
}

VkDescriptorSet DescriptorAllocatorGrowable::allocate(VkDevice device, VkDescriptorSetLayout layout)
{
    VkDescriptorPool pool = get_pool(device);
    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout
    };

    VkDescriptorSet ds;
    VkResult result = vkAllocateDescriptorSets(device, &alloc_info, &ds);

    // Try again if allocation failed, and explode if it still fails
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
        this->full_pools.push_back(pool);

        pool = get_pool(device);
        alloc_info.descriptorPool = pool;
        VK_CHECK(vkAllocateDescriptorSets(device, &alloc_info, &ds));
    }

    this->ready_pools.push_back(pool);
    return ds;
}

VkDescriptorPool DescriptorAllocatorGrowable::create_pool(VkDevice device, uint32_t set_count, std::span<PoolSizeRatio> pool_ratios)
{
    std::vector<VkDescriptorPoolSize> pool_sizes;
    for (PoolSizeRatio ratio : pool_ratios) {
        pool_sizes.push_back(VkDescriptorPoolSize {
            .type = ratio.type,
            .descriptorCount = uint32_t(ratio.ratio * set_count)
        });
    }

    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = 0,
        .maxSets = set_count,
        .poolSizeCount = (uint32_t) pool_sizes.size(),
        .pPoolSizes = pool_sizes.data()
    };

    VkDescriptorPool new_pool;
    vkCreateDescriptorPool(device, &pool_info, nullptr, &new_pool);
    return new_pool;
}

VkDescriptorPool DescriptorAllocatorGrowable::get_pool(VkDevice device)
{
    VkDescriptorPool new_pool;
    if (ready_pools.size() != 0) {
        new_pool = ready_pools.back();
        ready_pools.pop_back();
    }
    else {
        new_pool = create_pool(device, this->sets_per_pool, this->ratios);
        this->sets_per_pool = this->sets_per_pool * 1.5;
        if (this->sets_per_pool > MAX_SETS_PER_POOL) {
            this->sets_per_pool = MAX_SETS_PER_POOL;
        }
    }

    return new_pool;
}

void DescriptorAllocatorGrowable::destroy_pools(VkDevice device)
{
    for (auto p : this->ready_pools) {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    this->ready_pools.clear();

    for (auto p : this->full_pools) {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    this->full_pools.clear();
}

void DescriptorWriter::write_image(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type)
{
    VkDescriptorImageInfo &info = image_infos.emplace_back(VkDescriptorImageInfo {
        .sampler = sampler,
        .imageView = image,
        .imageLayout = layout
    });

    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = VK_NULL_HANDLE,
        .dstBinding = (uint32_t) binding,
        .descriptorCount = 1,
        .descriptorType = type,
        .pImageInfo = &info
    };

    this->writes.push_back(write);
}

void DescriptorWriter::write_buffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
{
    VkDescriptorBufferInfo &info = buffer_infos.emplace_back(VkDescriptorBufferInfo {
        .buffer = buffer,
        .offset = offset,
        .range = size
    });

    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = VK_NULL_HANDLE,
        .dstBinding = (uint32_t) binding,
        .descriptorCount = 1,
        .descriptorType = type,
        .pBufferInfo = &info
    };

    this->writes.push_back(write);
}

void DescriptorWriter::clear()
{
    this->image_infos.clear();
    this->writes.clear();
    this->buffer_infos.clear();
}

void DescriptorWriter::update_set(VkDevice device, VkDescriptorSet set)
{
    for (VkWriteDescriptorSet& write : this->writes) {
        write.dstSet = set;
    }
    vkUpdateDescriptorSets(device, (uint32_t) this->writes.size(), this->writes.data(), 0, nullptr);
}
