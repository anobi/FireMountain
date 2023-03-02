#include <iostream>

#include <SDL2/SDL.h>

#include "firemountain.hpp"


int Firemountain::Init(const int width, const int height, SDL_Window* window) {
    this->vulkan.Init(width, height, window);

    this->vulkan._triangle_mesh._vertices.resize(3);
    this->vulkan._triangle_mesh._vertices[0] = {
        .position = {1.0f, 1.0f, 0.0f},
        .color = {0.0f, 1.0f, 0.0f}
    };
    this->vulkan._triangle_mesh._vertices[1] = {
        .position = {-1.0f, 1.0f, 0.0f},
        .color = {0.0f, 1.0f, 0.0f}
    };
    this->vulkan._triangle_mesh._vertices[2] = {
        .position = {0.0f, -1.0f, 0.0f},
        .color = {0.0f, 1.0f, 0.0f}
    };

    this->vulkan.UploadMesh(this->vulkan._triangle_mesh);

    return 0;
}

void Firemountain::Frame() {
    this->vulkan.Draw();
}

void Firemountain::Destroy() {
    this->vulkan.Destroy();
}
