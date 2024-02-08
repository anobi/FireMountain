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

void DescriptorAllocator::init_pool(VkDevice device, uint32_t max_sets, std::span<PoolSizeRatio> pool_ratios)
{
    std::vector<VkDescriptorPoolSize> pool_sizes;
    for (PoolSizeRatio ratio : pool_ratios) {
        pool_sizes.push_back(VkDescriptorPoolSize {
            .type = ratio.type,
            .descriptorCount = static_cast<uint32_t>(ratio.ratio * max_sets)
        });
    }

    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = 0,
        .maxSets = max_sets,
        .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
        .pPoolSizes = pool_sizes.data()
    };

    vkCreateDescriptorPool(device, &pool_info, nullptr, &this->pool);
}

void DescriptorAllocator::clear_descriptors(VkDevice device)
{
    vkResetDescriptorPool(device, this->pool, 0);
}

void DescriptorAllocator::destroy_pool(VkDevice device)
{
    vkDestroyDescriptorPool(device, this->pool, nullptr);
}

VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = this->pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout
    };

    VkDescriptorSet descriptor_set;
    VK_CHECK(vkAllocateDescriptorSets(device, &alloc_info, &descriptor_set));

    return descriptor_set;
}

void DescriptorAllocatorGrowable::init(VkDevice device, uint32_t initial_sets, std::span<PoolSizeRatio> pool_ratios)
{
    this->ratios.clear();
    for (auto r : pool_ratios) {
        ratios.push_back(r);
    }

    VkDescriptorPool pool = create_pool(device, initial_sets, pool_ratios);
    this->sets_per_pool = initial_sets * 1.5;
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
    for (auto ratio : pool_ratios) {
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

    VkDescriptorPool pool;
    vkCreateDescriptorPool(device, &pool_info, nullptr, &pool);
    return pool;
}

VkDescriptorPool DescriptorAllocatorGrowable::get_pool(VkDevice device)
{
    VkDescriptorPool pool;
    if (ready_pools.size() != 0) {
        pool = ready_pools.back();
        ready_pools.pop_back();
    }
    else {
        pool = create_pool(device, this->sets_per_pool, this->ratios);
        this->sets_per_pool = this->sets_per_pool * 1.5;
        if (this->sets_per_pool > MAX_SETS_PER_POOL) {
            this->sets_per_pool = MAX_SETS_PER_POOL;
        }
    }

    return pool;
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
