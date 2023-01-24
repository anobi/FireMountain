#pragma once

#include <cstdint>
#include <SDL2/SDL.h>

class Display {
public:
    Display() {};
    ~Display() {};

    int Init(const int width, const int height);
    void Destroy();

    SDL_Window* window;
};