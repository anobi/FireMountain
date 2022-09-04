#include <assert.h>
#include <cstdio>
#include "display.hpp"


int Display::Init(const uint8_t width, const uint8_t height) {
    this->window = SDL_CreateWindow(
        "FireMountain",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN
    );

    if (this->window == NULL) {
        printf("Could not create window: %s\n", SDL_GetError());
        exit(1);
    }

    return 0;
}

void Display::Destroy() {
    SDL_DestroyWindow(window);
}