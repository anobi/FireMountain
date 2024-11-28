#include <string>
#include <unordered_map>
#include <fmt/core.h>
#include <SDL2/SDL.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "firemountain.hpp"
#include "display.hpp"
#include "camera.hpp"

int WIDTH = 1920;
int HEIGHT = 1080;
float CAMERA_V_SPEED = 1;
float CAMERA_H_SPEED = 1;

Camera camera;
CameraProjectionType camera_projection = CameraProjectionType::PERSPECTIVE;


struct Transform {
    glm::vec3 position = { 0.0f, 0.0f, 0.0f};
    glm::quat rotation = { 1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale = { 1.0f, 1.0f, 1.0f};
};

struct GameSceneObject {
    std::string mesh_file;
    MeshID mesh_id;
    Transform transform;

    bool is_light = false;

    LightID light_id;
    LightType light_type;
    float light_intensity = 0.0f;
    float light_range = 0.0f;
    glm::vec3 light_direction;
    glm::vec3 light_color;

    bool dirty = true;
};


std::unordered_map<std::string, GameSceneObject> game_scene = {
    {"sponza", (GameSceneObject) {
        .mesh_file = "assets/Sponza/glTF/Sponza.gltf"
    }},
    // {"structure", {
    //     .mesh_file = "assets/structure.glb"
    // }},
    {"froge", (GameSceneObject) {
        .mesh_file = "assets/good_froge.glb",
        .transform = (Transform) {
            .position = {0.1f, 0.5f, 0.1f}
        }
    }},
    {"sun", (GameSceneObject) {
        .light_type = LightType::Area,
        .light_intensity = 1.8f,
        .light_range = 100.0f,
        .light_direction = { 0.0f, -1.0f, -0.5f },
        .light_color = { 0.8f, 0.4f, 0.2f }
    }},
    {"mid_point_light", (GameSceneObject) {
        .transform = (Transform) { 
            .position = { 0.0f, 3.0f, 0.0f }
        },
        .light_type = LightType::Point,
        .light_intensity = 8.0f,
        .light_range = 100.0f,
        .light_color = { 0.8f, 0.4f, 0.2f }
    }},
    {"corner_torch_blue", (GameSceneObject) {
        .transform = (Transform) { 
            .position = { 8.8f, 1.5f, 3.2f }
        },
        .light_type = LightType::Point,
        .light_intensity = 8.0f,
        .light_range = 100.0f,
        .light_color = { 0.2f, 0.4f, 0.8f }
    }},
    {"corner_torch_green", (GameSceneObject) {
        .transform = (Transform) {
            .position = { 9.0f, 1.5f, -3.6f }
        },
        .light_type = LightType::Point,
        .light_intensity = 8.0f,
        .light_range = 100.0f,
        .light_color = { 0.2f, 0.8f, 0.4f }
    }},
    {"corner_torch_red", (GameSceneObject) {
        .transform = (Transform) { 
            .position = { -9.5f, 1.5f, -3.65f }
        },
        .light_type = LightType::Point,
        .light_intensity = 8.0f,
        .light_range = 100.0f,
        .light_color = { 0.8f, 0.2f, 0.1f }
    }},
    {"corner_torch_purple", (GameSceneObject) {
        .transform = (Transform) { 
            .position = { -9.5f, 1.5f, 3.2f }
        },
        .light_type = LightType::Point,
        .light_intensity = 8.0f,
        .light_range = 100.0f,
        .light_color = { 0.8f, 0.2f, 0.8f }
    }}
};


// Spooky halloween scene
// std::unordered_map<std::string, GameSceneObject> game_scene = {
//     {"sponza", (GameSceneObject) {
//         .mesh_file = "assets/Sponza/glTF/Sponza.gltf"
//     }},
    // {"structure", {
    //     .mesh_file = "assets/structure.glb"
    // }},
    // {"froge", (GameSceneObject) {
    //     .mesh_file = "assets/evil_froge.glb",
    //     .transform = (Transform) {
    //         .position = {0.1f, 0.5f, 0.1f}
    //     }
    // }},
    // {"sun", (GameSceneObject) {
    //     .light_type = LightType::Area,
    //     .light_intensity = 0.8f,
    //     .light_range = 100.0f,
    //     .light_direction = { 0.0f, -1.0f, -0.5f },
    //     .light_color = { 0.8f, 0.15f, 0.1f }
    // }},
    // {"mid_point_light", (GameSceneObject) {
    //     .transform = (Transform) { 
    //         .position = { -3.0f, 3.0f, 0.0f }
    //     },
    //     .light_type = LightType::Point,
    //     .light_intensity = 0.02f,
    //     .light_range = 100.0f,
    //     .light_color = { 0.9f, 0.2f, 0.1f }
    // }},
//     {"corner_torch_red", (GameSceneObject) {
//         .transform = (Transform) { 
//             .position = { -9.5f, 1.5f, -3.65f }
//         },
//         .light_type = LightType::Point,
//         .light_intensity = 12.0f,
//         .light_range = 100.0f,
//         .light_color = { 0.8f, 0.2f, 0.1f }
//     }},
//     {"corner_torch_purple", (GameSceneObject) {
//         .transform = (Transform) { 
//             .position = { -9.5f, 1.5f, 3.2f }
//         },
//         .light_type = LightType::Point,
//         .light_intensity = 12.0f,
//         .light_range = 100.0f,
//         .light_color = { 0.8f, 0.2f, 0.8f }
//     }}
// };


int RunApp()
{
    Firemountain firemountain;
    Display display;

    SDL_Init(SDL_INIT_VIDEO);
    display.Init(WIDTH, HEIGHT);
    firemountain.Init(WIDTH, HEIGHT, display.window);

    for (auto& [key, obj] : game_scene) {
        if (obj.light_type != LightType::None) {
            obj.light_id = firemountain.AddLight(key);
        } else {
            obj.mesh_id = firemountain.AddMesh(key, obj.mesh_file.c_str());
        }
    }

    int tick = 0;
    bool running = true;
    bool resize_requested = false;
    SDL_Event event;

    camera.position = glm::vec3(3.0f, 1.0f, 0.0f);
    camera.yaw = -1.5f;

    SDL_bool capture_mouse = SDL_FALSE;
    int mouse_captured_x = 0;
    int mouse_captured_y = 0;
    SDL_SetRelativeMouseMode(capture_mouse);
    while(running) {
        tick += 1;
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
                    if (capture_mouse == SDL_TRUE) { 
                        capture_mouse = SDL_FALSE; 
                        // Return mouse to where it was before grabbing
                        SDL_WarpMouseInWindow(display.window, mouse_captured_x, mouse_captured_y);
                    }
                    else { 
                        capture_mouse = SDL_TRUE; 
                        // Save the mouse location so we can return it later
                        SDL_GetMouseState(&mouse_captured_x, &mouse_captured_y);
                    }
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
                    int new_width, new_height = 0;
                    SDL_GetWindowSize(display.window, &new_width, &new_height);
                    if (new_width != WIDTH || new_height != HEIGHT) {
                        WIDTH = new_width;
                        HEIGHT = new_height;
                        resize_requested = true;
                    }
                }
                break;
            
            default:
                break;
            }
            SDL_SetRelativeMouseMode(capture_mouse);
            firemountain.ProcessImGuiEvent(&event);
        }

        if (resize_requested) {
            firemountain.Resize(static_cast<uint32_t>(WIDTH), static_cast<uint32_t>(HEIGHT));
            resize_requested = false;
        }

        // Rotate and bob the froge
        game_scene["froge"].transform.position.y += 0.0025f * sin(0.02f * tick);
        game_scene["froge"].transform.rotation = glm::angleAxis(glm::radians(0.5f * tick), glm::vec3(0, 1, 0));
        game_scene["froge"].dirty = true;

        std::vector<RenderSceneObj> render_scene;

        // Send updated transforms to renderer
        for (auto& [key, obj] : game_scene) {
            // Push meshes

            // This should probably just be done in shaders or a comp shader while rendering.
            // Just send the transform stuff to renderer.
            if (obj.mesh_id) {
                auto m = glm::translate(glm::identity<glm::mat4>(), obj.transform.position)
                    * glm::mat4_cast(obj.transform.rotation)
                    * glm::scale(glm::identity<glm::mat4>(), obj.transform.scale);
                render_scene.push_back({ 
                    .mesh_id = obj.mesh_id, 
                    .transform = m 
                });
            }

            // Push lights
            if (obj.light_id)  {
                render_scene.push_back({
                    .light_id = obj.light_id,
                    .light_position_type = { obj.transform.position, obj.light_type },
                    .light_color_intensity = { obj.light_color, obj.light_intensity },
                    .light_direction_range = { obj.light_direction, obj.light_range}
                });
            }
            obj.dirty = false;
        }

        camera.Update();
        auto render_camera = fmCamera {
            .position = camera.position,
            .view = camera.get_view_matrix(),
            .projection = camera.GetProjectionMatrix(WIDTH, HEIGHT, camera_projection)
        };
        firemountain.Frame(render_camera, render_scene);
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