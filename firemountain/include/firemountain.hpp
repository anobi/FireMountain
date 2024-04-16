#pragma once

#include <SDL2/SDL.h>
#include <filesystem>

#include "fm_utils.hpp"
#include "fm_scene.hpp"
#include "vk_mesh.hpp"
#include "vk_types.hpp"
#include "vk_renderer.hpp"

//class SDL_Event;

class Firemountain {
public:
    Firemountain() {};
    ~Firemountain() {};

    int Init(const int width, const int height, SDL_Window* window);
    void Frame();
    void Resize(const uint32_t width, const uint32_t height);
    void Destroy();

    void ProcessImGuiEvent(SDL_Event* e);

    bool AddMesh(const std::string& name, const char* path);

    fmVK::Vulkan vulkan;
    Scene scene;
    

private:
    DeletionQueue _deletion_queue;

    std::vector<RenderObject> _renderables;
    std::unordered_map<std::string, MaterialInstance> _materials;
    //std::unordered_map<std::string, GPUMeshBuffers> _meshes;

    std::unordered_map<std::string, std::vector<std::shared_ptr<MeshAsset>>> _meshes;
    std::unordered_map<std::string, std::shared_ptr<Node>> loadedNodes;

    MaterialInstance* create_material(const std::string& name);
    MaterialInstance* get_material(const std::string& name);
    std::vector<std::shared_ptr<MeshAsset>> get_mesh(const std::string& name);
};