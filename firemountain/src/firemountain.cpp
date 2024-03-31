#include <iostream>

#include <SDL2/SDL.h>
#include <glm/gtx/transform.hpp>

#include "firemountain.hpp"
#include "fm_mesh_loader.hpp"


int Firemountain::Init(const int width, const int height, SDL_Window* window) {
    this->vulkan.Init(width, height, window);
    this->create_material("mesh");

    return 0;
}

void Firemountain::Frame() {
    this->vulkan.Draw(this->_renderables.data(), this->_renderables.size());
}

void Firemountain::Resize(const uint32_t width, const uint32_t height)
{
    this->vulkan.Resize(width, height);
}

void Firemountain::Destroy() {
    this->vulkan.Destroy();
}

void Firemountain::ProcessImGuiEvent(SDL_Event* e)
{
    this->vulkan.ProcessImGuiEvent(e);
}

int mesh_index = 0;

bool Firemountain::AddMesh(const std::string& name, const char* path) {
    this->_meshes[name] = MeshLoader::LoadGltf(path, &this->vulkan);

    RenderObject render_object;
    render_object.mesh = &this->_meshes[name][0]->mesh_buffers;
    render_object.index_count = this->_meshes[name][0]->surfaces[0].count;
    render_object.first_index = this->_meshes[name][0]->surfaces[0].start_index;
    render_object.material = this->get_material("mesh");
    render_object.transform = glm::translate(glm::vec3{
        0.0f + mesh_index * 2.0f,
        0.0f,
        0.0f
    });

    this->_renderables.push_back(render_object);

    mesh_index += 1;
    
    return true;
}

Material* Firemountain::create_material(const std::string& name) {
    Material mat = {
        .pipeline = this->vulkan.GetPipeline("mesh"),
        .pipeline_layout = this->vulkan.GetPipelineLayout("mesh")
    };
    this->_materials[name] = mat;
    return &this->_materials[name];
}

Material* Firemountain::get_material(const std::string& name) {
    auto i = this->_materials.find(name);
    if (i == this->_materials.end()) {
        return nullptr;
    } else {
        return &(*i).second;
    }
}