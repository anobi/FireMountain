#pragma once

#include "vk_types.hpp"

class Firemountain {
public:
    Firemountain() {};
    ~Firemountain() {};

    int Init();
    void Frame();
    void Destroy();
    
private:
    VkExtent2D _window_extent;
};
