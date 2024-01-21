#pragma once

#include <vector>
#include <span>
#include <glm/vec3.hpp>
#include "vk_types.hpp"


struct VertexInputDescription {
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    VkPipelineVertexInputStateCreateFlags flags = 0;
};


struct Vertex {
    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec3 color; 
    
    static VertexInputDescription get_vertex_description();
};

struct LoaderMesh {
    std::span<Vertex> vertices;
    std::span<uint32_t> indices;
    bool load_from_obj(const char* path);
};

struct GPUMeshBuffers {
    AllocatedBuffer index_buffer;
    AllocatedBuffer vertex_buffer;
    VkDeviceAddress vertex_buffer_address;
};

struct GPUDrawPushConstants {
    glm::mat4 world_matrix;
    VkDeviceAddress vertex_buffer;
};