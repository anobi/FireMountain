#include <iostream>

#include <SDL2/SDL.h>

#include "firemountain.hpp"


int Firemountain::Init(const int width, const int height, SDL_Window* window) {
    this->vulkan.Init(width, height, window);
    return 0;
}

void Firemountain::Frame() {
    this->vulkan.Draw();
}

void Firemountain::Destroy() {
    this->vulkan.Destroy();
}
