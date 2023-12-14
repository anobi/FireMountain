#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "fm_renderable.hpp"
#include "vk_types.hpp"
#include "vk_mesh.hpp"


class Scene {
public:
    void Init();
    void Destroy();
    void AddMesh(const std::string& name, Mesh mesh);

private:
    Material* create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name);
    Material* get_material(const std::string& name);

    Mesh* get_mesh(const std::string& name);

    void draw_objects(VkCommandBuffer cmd, RenderObject* first, int count);

    std::vector<RenderObject> _renderables;
    std::unordered_map<std::string, Material> _materials;
    std::unordered_map<std::string, Mesh> _meshes;
};