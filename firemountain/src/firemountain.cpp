#include <iostream>

#include <SDL2/SDL.h>

#include "firemountain.hpp"
#include "fm_mesh_loader.hpp"


int Firemountain::Init(const int width, const int height, SDL_Window* window) {
    this->vulkan.Init(width, height, window);
    this->create_material("mesh");

    // this->_triangle_mesh.vertices.resize(3);
    // this->_triangle_mesh.vertices[0] = {
    //     .position = {1.0f, 1.0f, 0.0f},
    //     .color = {0.0f, 1.0f, 0.0f}
    // };
    // this->_triangle_mesh.vertices[1] = {
    //     .position = {-1.0f, 1.0f, 0.0f},
    //     .color = {0.0f, 1.0f, 0.0f}
    // };
    // this->_triangle_mesh.vertices[2] = {
    //     .position = {0.0f, -1.0f, 0.0f},
    //     .color = {0.0f, 1.0f, 0.0f}
    // };

    // this->vulkan.UploadMesh(this->_triangle_mesh);
    // this->scene.AddMesh("triangle", this->_triangle_mesh);

    // this->_monke_mesh.load_from_obj("assets/monke.obj");
    // this->vulkan.UploadMesh(this->_monke_mesh);
    // this->scene.AddMesh("monke", this->_monke_mesh);

    return 0;
}

void Firemountain::Frame() {
    this->vulkan.Draw(this->_renderables.data(), this->_renderables.size());
}

void Firemountain::Destroy() {
    this->vulkan.Destroy();
}

void Firemountain::ProcessImGuiEvent(SDL_Event* e)
{
    this->vulkan.ProcessImGuiEvent(e);
}

bool Firemountain::AddMesh(const std::string& name, const char* path) {
    this->_meshes[name] = MeshLoader::LoadGltf("assets/monke.glb", &this->vulkan);

    RenderObject render_object;
    render_object.mesh = &this->_meshes[name][0]->mesh_buffers;
    render_object.material = this->get_material("mesh");
    render_object.transform = glm::mat4{ 1.0f };
    render_object.index_count = this->_meshes[name][0]->surfaces[0].count;
    render_object.first_index = this->_meshes[name][0]->surfaces[0].start_index;
    this->_renderables.push_back(render_object);
    
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

// GPUMeshBuffers* Firemountain::get_render_mesh(const std::string& name) {
//     auto i = this->_meshes.find(name);
//     if (i == this->_meshes.end()) {
//         return nullptr;
//     } else {
//         return &(*i).second;
//     }
// }