#pragma once

#include <vector>
#include <span>
#include <glm/vec3.hpp>
#include "vk_types.hpp"
#include "vk_buffer.hpp"


struct VertexInputDescription {
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct GLTFMaterial {
    MaterialInstance data;
};

struct Bounds {
    glm::vec3 origin;
    float sphere_radius;
    glm::vec3 extents;
};

struct GeoSurface {
    uint32_t start_index;
    uint32_t count;
    Bounds bounds;
    std::shared_ptr<GLTFMaterial> material;
};

struct Vertex {
    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec4 color;
};

struct GPUMeshBuffers {
    fmvk::Buffer::AllocatedBuffer index_buffer;
    fmvk::Buffer::AllocatedBuffer vertex_buffer;
    VkDeviceAddress vertex_buffer_address;
};

struct MeshAsset {
    std::string name;
    std::vector<GeoSurface> surfaces;
    GPUMeshBuffers mesh_buffers;
};

struct GPUDrawPushConstants {
    glm::mat4 world_matrix;
    VkDeviceAddress vertex_buffer;
};

