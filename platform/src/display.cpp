#include <assert.h>
#include <cstdio>
#include <SDL3/SDL_video.h>
#include "display.hpp"

#include <cstdlib>


int Display::Init(const int width, const int height) {
    this->window = SDL_CreateWindow(
        "FireMountain",
        width,
        height,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
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