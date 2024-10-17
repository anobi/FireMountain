#include <string>
#include <fmt/core.h>
#include <SDL2/SDL.h>

#include "firemountain.hpp"
#include "display.hpp"
#include "camera.hpp"

int WIDTH = 1920;
int HEIGHT = 1080;
float CAMERA_V_SPEED = 1;
float CAMERA_H_SPEED = 1;

Camera camera;
CameraProjectionType camera_projection = CameraProjectionType::PERSPECTIVE;

int RunApp()
{
    Firemountain firemountain;
    Display display;

    SDL_Init(SDL_INIT_VIDEO);
    display.Init(WIDTH, HEIGHT);
    firemountain.Init(WIDTH, HEIGHT, display.window);

    auto sponza = firemountain.AddMesh("sponza", "assets/Sponza/glTF/Sponza.gltf");
    auto froge = firemountain.AddMesh("froge", "assets/good_froge.glb");
    // auto structure = firemountain.AddMesh("structure", "assets/structure.glb");
    // auto monke = firemountain.AddMesh("monke", "assets/monke.glb");
    // auto supersponza = firemountain.AddMesh("supersponza", "assets/sponza_next/Main.1_Sponza/NewSponza_Main_glTF_002.gltf");
    // auto sun = firemountain.AddLight("sun", LIGHT_SUN);

    bool running = true;
    bool resize_requested = false;
    SDL_Event event;

    camera.position = glm::vec3(3.0f, 1.0f, 0.0f);
    camera.yaw = -1.5f;
    // firemountain.UpdateView(camera.get_view_matrix());

    SDL_bool capture_mouse = SDL_FALSE;
    float mouse_captured_x = 0.0f;
    float mouse_captured_y = 0.0f;
    SDL_SetRelativeMouseMode(capture_mouse);
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

                if (event.key.keysym.sym == SDLK_w) { camera.velocity.z = -1; }
                if (event.key.keysym.sym == SDLK_s) { camera.velocity.z =  1; }
                if (event.key.keysym.sym == SDLK_a) { camera.velocity.x = -1; }
                if (event.key.keysym.sym == SDLK_d) { camera.velocity.x =  1; }
                if (event.key.keysym.sym == SDLK_LCTRL) { camera.velocity.y =  -1; }
                if (event.key.keysym.sym == SDLK_SPACE) { camera.velocity.y =  1; }
                break;

            case SDL_KEYUP:
                if (event.key.keysym.sym == SDLK_w) { camera.velocity.z = 0; }
                if (event.key.keysym.sym == SDLK_s) { camera.velocity.z = 0; }
                if (event.key.keysym.sym == SDLK_a) { camera.velocity.x = 0; }
                if (event.key.keysym.sym == SDLK_d) { camera.velocity.x = 0; }
                if (event.key.keysym.sym == SDLK_LCTRL) { camera.velocity.y =  0; }
                if (event.key.keysym.sym == SDLK_SPACE) { camera.velocity.y =  0; }

                if (event.key.keysym.sym == SDLK_RALT) {
                    if (capture_mouse == SDL_TRUE) { capture_mouse = SDL_FALSE; }
                    else { capture_mouse = SDL_TRUE; }
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == 3) { 
                    capture_mouse = SDL_TRUE; 
                    // Save the mouse location so we can return it later
                    mouse_captured_x = event.button.x;
                    mouse_captured_y = event.button.y;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == 3) {
                    capture_mouse = SDL_FALSE; 
                    // Return mouse to where it was before grabbing
                    SDL_WarpMouseInWindow(display.window, mouse_captured_x, mouse_captured_y);
                }
                break;

            case SDL_MOUSEMOTION:
                if (capture_mouse == SDL_TRUE) {
                    camera.yaw += (float) event.motion.xrel * CAMERA_H_SPEED / 100.0f;
                    camera.pitch -= (float) event.motion.yrel * CAMERA_V_SPEED / 100.0f;
                    camera.pitch = glm::clamp(camera.pitch, -1.5f, 1.5f);
                }
                else {

                }
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    resize_requested = true;
                    SDL_GetWindowSize(display.window, &WIDTH, &HEIGHT);
                }
                break;
            
            default:
                break;
            }
            SDL_SetRelativeMouseMode(capture_mouse);
            firemountain.ProcessImGuiEvent(&event);
        }

        if (resize_requested) {
            firemountain.Resize((uint32_t)WIDTH, (uint32_t)HEIGHT);
            resize_requested = false;
        }

        camera.Update();
        firemountain.Frame(camera.GetViewProjectionMatrix(WIDTH, HEIGHT, camera_projection));
    }

    SDL_SetRelativeMouseMode(SDL_FALSE);  // Release mouse before the exit
    
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