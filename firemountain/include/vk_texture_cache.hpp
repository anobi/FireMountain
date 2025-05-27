#pragma once
#include <unordered_map>
#include <vector>
#include "vk_types.hpp"

namespace fmvk {
    struct TextureID {
        uint32_t index;
    };

    struct TextureCache {
        std::vector<VkDescriptorImageInfo> cache;
        std::unordered_map<std::string, TextureID> cache_map;
        TextureID add_texture(const VkImageView& image, VkSampler sampler);
    };
}

