#pragma once

#include "vk_types.hpp"
#include "vk_mesh.hpp"




struct RenderObject {
    uint32_t index_count;
    uint32_t first_index;
    VkBuffer index_buffer;

    MaterialInstance* material;

    glm::mat4 transform;
    VkDeviceAddress vertex_buffer_address;
};

struct DrawContext {

};

class IRenderable {
    virtual void Draw(const glm::mat4& top_matrix, DrawContext& ctx) = 0;
}