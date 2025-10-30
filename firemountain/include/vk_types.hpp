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

/*struct AllocatedImage {
    VkImage image;
    VkImageView view;
    VmaAllocation allocation;
    VkExtent3D extent;
    VkFormat format;
};*/

struct ComputePushConstants {
    glm::vec4 data_1;
    glm::vec4 data_2;
    glm::vec4 data_3;
    glm::vec4 data_4;
};


struct LightID {
    operator bool() const noexcept { return id != 0; }
    uint32_t id;
};

enum LightType {
    None = 0,
    Point = 1,
    Spot = 2,
    Area = 3
};

struct GPULightData {
    // Position with light type as .w
    glm::vec4 positionType = glm::vec4 {0.0f};

    // Color with light intensity as .w
    glm::vec4 colorIntensity = glm::vec4 {0.0f};

    // Direction with range as .w
    glm::vec4 directionRange = glm::vec4 {0.0f};

    // Info (spotlights only) with .x as inner cone angle and .y as outer cone angle
    glm::vec4 info = glm::vec4 {0.0f};
};

// Structure that gets fed into the shaders as an input structure
struct GPUSceneData {
    glm::mat4 view = glm::mat4 { 0.0f };
    glm::mat4 projection = glm::mat4 { 0.0f };
    glm::vec3 camera_position = glm::vec3 { 0.0f };
    unsigned int light_count;

    GPULightData lights[32];
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

struct fmCamera {
    glm::vec3 position;
    glm::mat4 view;
    glm::mat4 projection;
    bool debug_pov_lock = false;
};

struct RenderSceneObj {
    MeshID mesh_id;
    glm::mat4 transform;

    LightID light_id;
    glm::vec4 light_position_type;
    glm::vec4 light_color_intensity;
    glm::vec4 light_direction_range;
};
