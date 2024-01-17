#pragma once

#include "vk_types.hpp"
#include "vk_mesh.hpp"


struct Material {
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
};

struct RenderObject {
    uint32_t index_count;
    uint32_t first_index;
    VkBuffer index_buffer;

    Mesh* mesh;
    Material* material;
    glm::mat4 transform;
};
