#include <string>
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
    auto monke = firemountain.AddMesh("monke", "assets/monke.glb");

    bool running = true;
    SDL_Event event;
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
                }

            case SDL_WINDOWEVENT_RESIZED:
                int w, h;
                SDL_GetWindowSize(display.window, &w, &h);
                firemountain.Resize(w, h);
            
            default:
                break;
            }

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