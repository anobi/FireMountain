#pragma once

#include "fm_utils.hpp"
#include "vk_mesh.hpp"
#include "vk_types.hpp"
#include "vk_renderer.hpp"


class Firemountain {
public:
    Firemountain() {};
    ~Firemountain() {};

    int Init(const int width, const int height, SDL_Window* window);
    void Frame();
    void Destroy();

    fmVK::Vulkan vulkan;

private:
    DeletionQueue _deletion_queue;
};