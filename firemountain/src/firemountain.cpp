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

void Firemountain::Frame(glm::mat4 view_projection_matrix) {
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

MaterialInstance* Firemountain::get_material(const std::string& name) {
    auto i = this->_materials.find(name);
    if (i == this->_materials.end()) {
        return nullptr;
    } else {
        return &(*i).second;
    }
}