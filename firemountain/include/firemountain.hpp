#pragma once

#include <SDL3/SDL_events.h>

#include "fm_utils.hpp"
#include "vk_mesh.hpp"
#include "vk_types.hpp"
#include "vk_renderer.hpp"

//class SDL_Event;


class Firemountain {
public:
    Firemountain() {};
    ~Firemountain() {};

    int Init(int width, int height, SDL_Window* window);
    void Frame(const fmCamera* camera, eastl::vector<RenderSceneObj> scene);
    void Resize(uint32_t width, uint32_t height);
    void Destroy();

    void ProcessImGuiEvent(SDL_Event* e);

    void CompileShaders();

    MeshID AddMesh(const eastl::string& name, const char* path);
    LightID AddLight(const eastl::string& name);

    fmvk::Vulkan vulkan;


    // TODO: There shouldn't be almost anything private here.
    //       Only stuff like renderer and so on. This is the main interface class.
private:
    DeletionQueue _deletion_queue;

    eastl::vector<RenderObject> _renderables;
    // eastl::vector<IRenderable> _renderables;
    eastl::unordered_map<eastl::string, MaterialInstance> _materials;
    //ankerl::unordered_dense::map<eastl::string, GPUMeshBuffers> _meshes;

    ankerl::unordered_dense::map<eastl::string, eastl::vector<std::shared_ptr<MeshAsset>>> _meshes;
    ankerl::unordered_dense::map<eastl::string, std::shared_ptr<Node>> loadedNodes;
    ankerl::unordered_dense::map<eastl::string, std::shared_ptr<LoadedGLTF>> loaded_Scenes;

    MaterialInstance* create_material(const eastl::string& name);
    MaterialInstance* get_material(const eastl::string& name);
    eastl::vector<std::shared_ptr<MeshAsset>> get_mesh(const eastl::string& name);
};
