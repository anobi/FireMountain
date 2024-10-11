#include <iostream>

#include <SDL2/SDL.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include "firemountain.hpp"
#include "fm_mesh_loader.hpp"


int Firemountain::Init(const int width, const int height, SDL_Window* window) {

    this->_main_camera.position = glm::vec3(0.0f, 0.0f, 5.0f);

    this->vulkan.Init(width, height, window);
    this->vulkan._camera = &this->_main_camera;

    return 0;
}

void Firemountain::Frame() {
    this->vulkan.Draw(this->_renderables.data(), this->_renderables.size());
    // this->vulkan.Draw(this->loaded_Scenes, this->loaded_Scenes.size());
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

bool Firemountain::AddMesh(const std::string& name, const char* path) {
    // this->_meshes[name] = MeshLoader::LoadGltf(path, &this->vulkan);
    auto mesh_file = MeshLoader::load_GLTF(&this->vulkan, path);
    assert(mesh_file.has_value());
    this->vulkan.loaded_Scenes[name] = *mesh_file;
    
    return true;
}

// TODO: Calculate position in the actual engine, and just pass those things
// here to calculate the matrices, which are stored in fireomountain camera
void Firemountain::UpdateCamera(float pitch, float yaw, glm::vec3 velocity)
{
    this->_main_camera.pitch -= pitch;
    this->_main_camera.yaw += yaw;
    this->_main_camera.velocity = velocity;
}

// MaterialInstance* Firemountain::create_material(const std::string& name) {
//     MaterialInstance mat = {
//         .pipeline = this->vulkan.GetPipeline("mesh"),
//         .pipeline_layout = this->vulkan.GetPipelineLayout("mesh")
//     };
//     this->_materials[name] = mat;
//     return &this->_materials[name];
// }

MaterialInstance* Firemountain::get_material(const std::string& name) {
    auto i = this->_materials.find(name);
    if (i == this->_materials.end()) {
        return nullptr;
    } else {
        return &(*i).second;
    }
}