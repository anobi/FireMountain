#include <iostream>

#include <SDL2/SDL.h>

#include "firemountain.hpp"


int Firemountain::Init(const int width, const int height, SDL_Window* window) {
    this->vulkan.Init(width, height, window);
    this->scene.Init();

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
    this->vulkan.Draw();
}

void Firemountain::Destroy() {
    this->vulkan.Destroy();
}

Mesh* Firemountain::AddMesh(const std::string& name, const char* path) {
    auto mesh = this->_meshes[name];
    mesh.load_from_obj(path);

    this->vulkan.UploadMesh(mesh);
    this->scene.AddMesh(name, mesh);
    
    return &mesh;
}