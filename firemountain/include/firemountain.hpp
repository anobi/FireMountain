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
    void Frame(glm::vec3 camera_position, glm::mat4 view_projection_matrix, std::vector<RenderSceneObj> scene);
    void Resize(const uint32_t width, const uint32_t height);
    void Destroy();

    void ProcessImGuiEvent(SDL_Event* e);

    MeshID AddMesh(const std::string& name, const char* path);
    LightID AddLight(const std::string& name);

    fmvk::Vulkan vulkan;
    Scene scene;
    

    // TODO: There shouldn't be almost anything private here.
    //       Only stuff like renderer and so on. This is the main interface class.
private:
    DeletionQueue _deletion_queue;

    std::vector<RenderObject> _renderables;
    // std::vector<IRenderable> _renderables;
    std::unordered_map<std::string, MaterialInstance> _materials;
    //std::unordered_map<std::string, GPUMeshBuffers> _meshes;

    std::unordered_map<std::string, std::vector<std::shared_ptr<MeshAsset>>> _meshes;
    std::unordered_map<std::string, std::shared_ptr<Node>> loadedNodes;
    std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> loaded_Scenes;

    MaterialInstance* create_material(const std::string& name);
    MaterialInstance* get_material(const std::string& name);
    std::vector<std::shared_ptr<MeshAsset>> get_mesh(const std::string& name);
};