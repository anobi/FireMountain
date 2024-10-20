#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <fmt/core.h>

#define VK_CHECK(x)                                                         \
    do {                                                                    \
		VkResult err = x;                                                   \
		if (err) {                                                          \
			fmt::print("Detected Vulkan error: {}", string_VkResult(err));  \
			abort();                                                        \
		}                                                                   \
	} while (0)

struct AllocatedImage {
    VkImage image;
    VkImageView view;
    VmaAllocation allocation;
    VkExtent3D extent;
    VkFormat format;
};

struct ComputePushConstants {
    glm::vec4 data_1;
    glm::vec4 data_2;
    glm::vec4 data_3;
    glm::vec4 data_4;
};

struct GPUSceneData {
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewprojection;

    glm::vec4 ambient_color;

    glm::vec4 sunlight_direction;  // w for sun power
    glm::vec4 sunlight_color;

    // TODO: Add lights here
};

enum class MaterialPass : uint8_t {
    FM_MATERIAL_PASS_OPAQUE,
    FM_MATERIAL_PASS_TRANSPARENT,
    FM_MATERIAL_PASS_OTHER
};

struct MaterialPipeline {
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

struct MaterialInstance {
    MaterialPipeline* pipeline;
    VkDescriptorSet material_set;
    MaterialPass pass_type;
};

struct MeshID {
    operator bool() const noexcept { return id != 0; }
    uint32_t id;
};

struct ShaderID {
    operator bool() const noexcept { return id != 0; }
    uint32_t id;
};

struct RenderSceneObj {
    MeshID mesh_id;
    glm::mat4 transform;
};