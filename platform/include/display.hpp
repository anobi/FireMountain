#pragma once

#include <cstdint>
#include <SDL.h>

class Display {
public:
    Display() {};
    ~Display() {};

    int Init(const uint8_t width, const uint8_t height);
    void Destroy();

private:
    SDL_Window* window;
};