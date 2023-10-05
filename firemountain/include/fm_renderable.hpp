#pragma once

#include <vk_types.hpp>
#include <vk_mesh.hpp>


struct Material {
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
};

struct RenderObject {
    Mesh* mesh;
    Material* material;
    glm::mat4 transform;
};
