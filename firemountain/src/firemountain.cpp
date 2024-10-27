#include <iostream>

#include <SDL2/SDL.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include "firemountain.hpp"
#include "fm_mesh_loader.hpp"


int Firemountain::Init(const int width, const int height, SDL_Window* window) {
    this->vulkan.Init(width, height, window);
    return 0;
}

void Firemountain::Frame(glm::vec3 camera_position, glm::mat4 view_projection_matrix, std::vector<RenderSceneObj> scene) 
{
    this->vulkan.update_scene(camera_position, view_projection_matrix, scene);
    this->vulkan.Draw(this->_renderables.data(), this->_renderables.size(), view_projection_matrix);
}

void Firemountain::Resize(const uint32_t width, const uint32_t height)
{
    this->vulkan.Resize(width, height);
}

void Firemountain::Destroy() {
    this->vulkan.Destroy();
    this->loaded_Scenes.clear();
}

void Firemountain::ProcessImGuiEvent(SDL_Event* e)
{
    this->vulkan.ProcessImGuiEvent(e);
}

int mesh_index = 0;

MeshID Firemountain::AddMesh(const std::string& name, const char* path) {
    // Should I move this to renderer?
    auto mesh_file = MeshLoader::load_GLTF(&this->vulkan, path);
    assert(mesh_file.has_value());
    auto id = this->vulkan.AddMesh(name, *mesh_file);
    return id;
}

LightID Firemountain::AddLight(const std::string &name)
{
    auto id = this->vulkan.AddLight(name);
    return id;
}

// void Firemountain::DrawMesh(const MeshID id, const glm::mat4& transform_matrix) {
//     auto mesh = this->vulkan.loaded_meshes.at(id.id);
//     mesh->Draw(transform_matrix, this->vulkan._main_draw_context);
// }

// void Firemountain::UpdateTransform(const MeshID id, const glm::mat4& transform_matrix) {
//     auto mesh = this->vulkan.loaded_meshes.at(id.id);
//     for (auto node : mesh->top_nodes) {
//         node->refresh_transform(transform_matrix);
//     }
// }

MaterialInstance* Firemountain::get_material(const std::string& name) {
    auto i = this->_materials.find(name);
    if (i == this->_materials.end()) {
        return nullptr;
    } else {
        return &(*i).second;
    }
}