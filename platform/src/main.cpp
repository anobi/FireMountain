#include <string>
#include <unordered_map>
#include <fmt/core.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_mouse.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "python_embed.h"
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
    const char* mesh_file;
    MeshID mesh_id;
    Transform transform;

    bool is_light = false;

    LightID light_id {0};
    LightType light_type = LightType::None;
    float light_intensity = 0.0f;
    float light_range = 0.0f;
    glm::vec3 light_direction {};
    glm::vec3 light_color {};

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
            .position = {0.1f, 0.5f, -0.2f}
        }
    }},
    {"cube", (GameSceneObject) {
        .mesh_file = "assets/cube_1m.glb",
        .transform = (Transform) {
            .position = {0.0f, 5.0f, -0.2f}
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
/*std::unordered_map<std::string, GameSceneObject> game_scene = {
    {"sponza", (GameSceneObject) {
        .mesh_file = "assets/Sponza/glTF/Sponza.gltf"
    }},
    {"froge", (GameSceneObject) {
        .mesh_file = "assets/evil_froge.glb",
        .transform = (Transform) {
            .position = {0.1f, 0.5f, 0.1f}
        }
    }},
    {"sun", (GameSceneObject) {
        .light_type = LightType::Area,
        .light_intensity = 0.8f,
        .light_range = 100.0f,
        .light_direction = { 0.0f, -1.0f, -0.5f },
        .light_color = { 0.8f, 0.15f, 0.1f }
    }},
    {"mid_point_light", (GameSceneObject) {
        .transform = (Transform) {
            .position = { -3.0f, 3.0f, 0.0f }
        },
        .light_type = LightType::Point,
        .light_intensity = 0.02f,
        .light_range = 100.0f,
        .light_color = { 0.9f, 0.2f, 0.1f }
    }},
    {"corner_torch_red", (GameSceneObject) {
        .transform = (Transform) {
            .position = { -9.5f, 1.5f, -3.65f }
        },
        .light_type = LightType::Point,
        .light_intensity = 12.0f,
        .light_range = 100.0f,
        .light_color = { 0.8f, 0.2f, 0.1f }
    }},
    {"corner_torch_purple", (GameSceneObject) {
        .transform = (Transform) {
            .position = { -9.5f, 1.5f, 3.2f }
        },
        .light_type = LightType::Point,
        .light_intensity = 12.0f,
        .light_range = 100.0f,
        .light_color = { 0.8f, 0.2f, 0.8f }
    }}
};*/


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
            obj.mesh_id = firemountain.AddMesh(key, obj.mesh_file);
        }
    }

    float tick = 0;
    bool running = true;
    bool resize_requested = false;
    bool shader_reload_requested = false;
    SDL_Event event = {};

    camera.position = glm::vec3(3.0f, 1.0f, 0.0f);
    camera.yaw = -1.5f;

    auto camera_pov_lock = false;
    auto capture_mouse = false;
    auto mouse_captured_x = 0.0f;
    auto mouse_captured_y = 0.0f;
    SDL_SetWindowMouseGrab(display.window, capture_mouse);

    while(running) {
        tick += 1.0;

        // Mouse motion accumulators
        float mouse_pitch_acc = 0.0f;
        float mouse_yaw_acc = 0.0f;

        while(SDL_PollEvent(&event)) {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                running = false;
                break;

            case SDL_EVENT_KEY_DOWN:
                if(event.key.key == SDLK_ESCAPE) {
                    running = false;
                    break;
                }
                if (event.key.key == SDLK_F5) { shader_reload_requested = true; }

                if (event.key.key == SDLK_W) { camera.velocity.z = -1; }
                if (event.key.key == SDLK_S) { camera.velocity.z =  1; }
                if (event.key.key == SDLK_A) { camera.velocity.x = -1; }
                if (event.key.key == SDLK_D) { camera.velocity.x =  1; }
                if (event.key.key == SDLK_LCTRL) { camera.velocity.y =  -1; }
                if (event.key.key == SDLK_SPACE) { camera.velocity.y =  1; }
                break;

            case SDL_EVENT_KEY_UP:
                if (event.key.key == SDLK_W) { camera.velocity.z = 0; }
                if (event.key.key == SDLK_S) { camera.velocity.z = 0; }
                if (event.key.key == SDLK_A) { camera.velocity.x = 0; }
                if (event.key.key == SDLK_D) { camera.velocity.x = 0; }
                if (event.key.key == SDLK_LCTRL) { camera.velocity.y =  0; }
                if (event.key.key == SDLK_SPACE) { camera.velocity.y =  0; }

                if (event.key.key == SDLK_RALT) {
                    if (capture_mouse == true) {
                        capture_mouse = false;
                        // Return mouse to where it was before grabbing
                        SDL_WarpMouseInWindow(display.window, mouse_captured_x, mouse_captured_y);
                    }
                    else { 
                        capture_mouse = true;
                        // Save the mouse location so we can return it later
                        SDL_GetMouseState(&mouse_captured_x, &mouse_captured_y);
                    }
                }
                if (event.key.key == SDLK_L) { camera_pov_lock = !camera_pov_lock; }
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (event.button.button == 3) { 
                    capture_mouse = true; 
                    // Save the mouse location so we can return it later
                    mouse_captured_x = event.button.x;
                    mouse_captured_y = event.button.y;
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (event.button.button == 3) {
                    capture_mouse = false; 
                    // Return mouse to where it was before grabbing
                    SDL_WarpMouseInWindow(display.window, mouse_captured_x, mouse_captured_y);
                }
                break;

            case SDL_EVENT_MOUSE_MOTION:
                if (capture_mouse == true) {
                    mouse_pitch_acc = event.motion.yrel;
                    mouse_yaw_acc = event.motion.xrel;
                }
                break;

            case SDL_EVENT_WINDOW_RESIZED:
                int new_width, new_height = 0;
                SDL_GetWindowSize(display.window, &new_width, &new_height);
                if (new_width > 0 && new_height > 0) {
                    if (new_width != WIDTH || new_height != HEIGHT) {
                        WIDTH = new_width;
                        HEIGHT = new_height;
                        resize_requested = true;
                    }
                }
                break;
            }
            SDL_SetWindowRelativeMouseMode(display.window, capture_mouse);
            firemountain.ProcessImGuiEvent(&event);
        }

        if (resize_requested) {
            assert(WIDTH > 0 && HEIGHT > 0);
            firemountain.Resize(static_cast<uint32_t>(WIDTH), static_cast<uint32_t>(HEIGHT));
            resize_requested = false;
        }

        if (shader_reload_requested) {
            py_build_shaders("shaders", "shaders/bin");
            firemountain.CompileShaders();
            shader_reload_requested = false;
        }

        // Rotate and bob the froge
        game_scene["froge"].transform.position.y += 0.0025f * sin(0.02f * tick);
        game_scene["froge"].transform.rotation = glm::angleAxis(
            glm::radians(0.5f * tick),              // Rotation speed
            glm::normalize(glm::vec3(0, 1, 0))     // Rotate along Y-axis
        );
        game_scene["froge"].dirty = true;

        // Rotate cube
        game_scene["cube"].transform.rotation = glm::angleAxis(
            glm::radians(0.4f * tick),
            glm::normalize(glm::vec3(-0.8, 0.1, -0.4)) 
        );
        game_scene["cube"].dirty = true;

        std::vector<RenderSceneObj> render_scene;

        // Send updated transforms to renderer
        for (auto& [key, obj] : game_scene) {
            // Push meshes

            // This should probably just be done in shaders or a comp shader while rendering.
            // Just send the transform stuff to renderer.
            if (obj.mesh_id) {
                const auto m = glm::translate(glm::identity<glm::mat4>(), obj.transform.position)
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

        camera.yaw += static_cast<float>(mouse_yaw_acc) * CAMERA_H_SPEED / 100.0f;
        camera.pitch -= static_cast<float>(mouse_pitch_acc) * CAMERA_V_SPEED / 100.0f;
        camera.pitch = glm::clamp(camera.pitch, -1.5f, 1.5f);
        camera.Update();
        const auto render_camera = fmCamera {
            .position = camera.position,
            .view = camera.get_view_matrix(),
            .projection = camera.GetProjectionMatrix(
                static_cast<float>(WIDTH),
                static_cast<float>(HEIGHT),
                camera_projection
            ),
            .debug_pov_lock = camera_pov_lock
        };
        firemountain.Frame(&render_camera, render_scene);
    }

    SDL_SetWindowMouseGrab(display.window, false);  // Release mouse before the exit
    
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