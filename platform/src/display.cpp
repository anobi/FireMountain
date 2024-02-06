#include <assert.h>
#include <cstdio>
#include "display.hpp"


int Display::Init(const int width, const int height) {
    this->window = SDL_CreateWindow(
        "FireMountain",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
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