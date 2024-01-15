#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <fmt/core.h>

#define VK_CHECK(x)                                                         \
    do {                                                                    \
		VkResult err = x;                                                   \
		if (err) {                                                          \
			fmt::print("Detected Vulkan error: {}", string_VkResult(err));  \
			abort();                                                        \
		}                                                                   \
	} while (0)

struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
};

struct AllocatedImage {
    VkImage image;
    VkImageView view;
    VmaAllocation allocation;
    VkExtent3D extent;
    VkFormat format;
};

struct MeshPushConstants {
    glm::vec4 data;
    glm::mat4 render_matrix;
};

struct ComputePushConstants {
    glm::fvec4 data_1;
    glm::fvec4 data_2;
    glm::fvec4 data_3;
    glm::fvec4 data_4;
};