#pragma once

#include "vk_types.hpp"
#include "vk_mesh.hpp"



struct RenderObject {
    uint32_t index_count;
    uint32_t first_index;
    VkBuffer index_buffer;

    MaterialInstance* material;
    Bounds bounds;
    glm::mat4 transform;
    VkDeviceAddress vertex_buffer_address;
};

struct DrawContext {
    std::vector<RenderObject> opaque_surfaces;
    std::vector<RenderObject> transparent_surfaces;
};

class IRenderable {
public:
    virtual ~IRenderable() = default;

private:
    virtual void Draw(const glm::mat4& top_matrix, DrawContext& ctx) = 0;
};


// Render graph stuff
struct Node : public IRenderable {
    std::weak_ptr<Node> parent;
    std::vector<std::shared_ptr<Node>> children;

    glm::mat4 local_transform;
    glm::mat4 world_transform;

    void refresh_transform(const glm::mat4& parent_matrix) {
        this->world_transform = parent_matrix * this->local_transform;
        for (auto c : this->children) {
            c->refresh_transform(this->world_transform);
        }
    }

    virtual void Draw(const glm::mat4& top_matrix, DrawContext& ctx) {
        for (auto& c : this->children) {
            c->Draw(top_matrix, ctx);
        }
    }
};