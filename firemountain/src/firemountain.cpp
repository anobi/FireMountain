#include <iostream>

#include "firemountain.hpp"


int Firemountain::Init(const uint8_t width, const uint8_t height, void* window) {
    this->vulkan.Init(width, height, window);
    return 0;
}

void Firemountain::Frame() {

}

void Firemountain::Destroy() {
    this->vulkan.Destroy();
}