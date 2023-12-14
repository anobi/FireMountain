#include <iostream>

#include <SDL2/SDL.h>

#include "firemountain.hpp"


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



Mesh* Firemountain::AddMesh(const std::string& name, const char* path) {
    Mesh mesh;
    mesh.load_from_obj(path);
    this->vulkan.UploadMesh(mesh);
    this->_meshes[name] = mesh;

    RenderObject render_object;
    render_object.mesh = this->get_mesh(name);
    render_object.material = this->get_material("mesh");
    render_object.transform = glm::mat4{ 1.0f };
    this->_renderables.push_back(render_object);
    
    return &this->_meshes[name];
}

Material* Firemountain::create_material(const std::string& name) {
    Material mat = {
        .pipeline = this->vulkan._mesh_pipeline,
        .pipeline_layout = this->vulkan._mesh_pipeline_layout
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

Mesh* Firemountain::get_mesh(const std::string& name) {
    auto i = this->_meshes.find(name);
    if (i == this->_meshes.end()) {
        return nullptr;
    } else {
        return &(*i).second;
    }
}