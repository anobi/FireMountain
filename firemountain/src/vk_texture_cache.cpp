#include "vk_texture_cache.hpp"

fmvk::TextureID fmvk::TextureCache::add_texture(const VkImageView &image, VkSampler sampler) {
    for (unsigned int i = 0; i < cache.size(); i++) {
        if (cache[i].imageView == image && cache[i].sampler == sampler) {
            return TextureID {i};
        }
    }

    uint32_t index = cache.size();
    cache.push_back(VkDescriptorImageInfo {
        .sampler = sampler,
        .imageView = image,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    });
    return TextureID {index};
}
