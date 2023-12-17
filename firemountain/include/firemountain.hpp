#pragma once

#include "fm_utils.hpp"
#include "fm_scene.hpp"
#include "vk_mesh.hpp"
#include "vk_types.hpp"
#include "vk_renderer.hpp"



class Firemountain {
public:
    Firemountain() {};
    ~Firemountain() {};

    int Init(const int width, const int height, SDL_Window* window);
    void Frame();
    void Destroy();

    Mesh* AddMesh(const std::string& name, const char* path);

    fmVK::Vulkan vulkan;
    Scene scene;
    

private:
    DeletionQueue _deletion_queue;

    std::vector<RenderObject> _renderables;
    std::unordered_map<std::string, Material> _materials;
    std::unordered_map<std::string, Mesh> _meshes;

    Material* create_material(const std::string& name);
    Material* get_material(const std::string& name);
    Mesh* get_mesh(const std::string& name);
};