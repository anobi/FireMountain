#include <string>
#include <fmt/core.h>
#include <SDL2/SDL.h>

#include "firemountain.hpp"
#include "display.hpp"

int WIDTH = 1920;
int HEIGHT = 1080;


int RunApp()
{
    Firemountain firemountain;
    Display display;

    SDL_Init(SDL_INIT_VIDEO);
    display.Init(WIDTH, HEIGHT);
    firemountain.Init(WIDTH, HEIGHT, display.window);

    auto froge = firemountain.AddMesh("froge", "assets/good_froge.glb");
    // auto monke = firemountain.AddMesh("monke", "assets/monke.glb");

    bool running = true;
    SDL_Event event;

    float camera_yaw = 0.0f;
    float camera_pitch = 0.0f;
    glm::vec3 camera_velocity = glm::vec3(0);

    while(running) {
        while(SDL_PollEvent(&event)) {
            switch (event.type)
            {
            case SDL_QUIT:
                running = false;
                break;

            case SDL_KEYDOWN:
                if(event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                    break;
                }

                if (event.key.keysym.sym == SDLK_w) { camera_velocity.z = -1; }
                if (event.key.keysym.sym == SDLK_s) { camera_velocity.z =  1; }
                if (event.key.keysym.sym == SDLK_a) { camera_velocity.x = -1; }
                if (event.key.keysym.sym == SDLK_d) { camera_velocity.x =  1; }
                break;

            case SDL_KEYUP:
                if (event.key.keysym.sym == SDLK_w) { camera_velocity.z = 0; }
                if (event.key.keysym.sym == SDLK_s) { camera_velocity.z = 0; }
                if (event.key.keysym.sym == SDLK_a) { camera_velocity.x = 0; }
                if (event.key.keysym.sym == SDLK_d) { camera_velocity.x = 0; }
                break;

            case SDL_MOUSEMOTION:
                camera_yaw = (float) event.motion.xrel / 200.0f;
                camera_pitch = (float) event.motion.yrel / 200.0f;
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    int w, h;
                    SDL_GetWindowSize(display.window, &w, &h);
                    firemountain.Resize((uint32_t)w, (uint32_t)h);
                }
                break;
            
            default:
                break;
            }
            firemountain.UpdateCamera(camera_pitch, camera_yaw, camera_velocity);
            firemountain.ProcessImGuiEvent(&event);
        }
        firemountain.Frame();
    }

    firemountain.Destroy();
    display.Destroy();
    SDL_Quit();

    return 0;
}


#ifdef _WIN32

#include <Windows.h>
int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow
)
{
    RunApp();
    return 0;
}
#else

std::string GetWorkingDirectory(char* executable_path)
{
    std::string::size_type pos = std::string(executable_path).find_last_of("\\/");
    return std::string(executable_path).substr(0, pos);
}

int main(int argc, char* argv[])
{
    RunApp();
    return 0;
}
#endif