#pragma once

#include "vk_types.hpp"
#include "vk_renderer.hpp"

class Firemountain {
public:
    Firemountain() {};
    ~Firemountain() {};

    int Init(const uint8_t width, const uint8_t height, void* window);
    void Frame();
    void Destroy();  

    fmVK::Vulkan vulkan;
};