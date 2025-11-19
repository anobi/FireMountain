#include "game_scene.hpp"
#include "scene_wrapper.hpp"


class BasicSponzaScene : public ISceneWrapper {
public:
    void Init() {
        this->scene = GameScene("sponza");

        auto sponz = GameSceneObject("sponza");
        sponz.mesh_file = "assets/Sponza/glTF/Sponza.gltf";
        this->scene.add_object(sponz, "sponza");

        auto froge = GameSceneObject("froge");
        froge.mesh_file = "assets/good_froge.glb";
        froge.name = "Good Froge";
        froge.transform = (Transform) {
            .position = {0.1f, 0.5f, -0.2f}
        };
        this->scene.add_object(froge, "froge");

        auto cube = GameSceneObject("cube");
        cube.mesh_file = "assets/cube_1m.glb";
        cube.name = "Cube";
        cube.transform = (Transform) {
            .position = {0.0f, 5.0f, -0.2f}
        };
        this->scene.add_object(cube, "cube");

        auto sun_light = GameSceneObject("Sun");
        sun_light.name = "Sun";
        sun_light.light_type = LightType::Area;
        sun_light.light_intensity = 1.8f;
        sun_light.light_range = 100.0f;
        sun_light.light_direction = { 0.0f, -1.0f, -0.5f };
        sun_light.light_color = { 0.8f, 0.4f, 0.2f };
        this->scene.add_object(sun_light, "Sun");

        auto mid_light = GameSceneObject("LightMid");
        mid_light.name = "Light Mid";
        mid_light.transform = (Transform) {
            .position = { 0.0f, 3.0f, 0.0f }
        };
        mid_light.light_type = LightType::Point;
        mid_light.light_intensity = 8.0f;
        mid_light.light_range = 100.0f;
        mid_light.light_color = { 0.8f, 0.4f, 0.2f };
        this->scene.add_object(mid_light, "LightMid");

        auto blue_light = GameSceneObject("LightTorchBlue");
        blue_light.name = "Torch (blue)";
        blue_light.transform = (Transform) {
            .position = { 8.8f, 1.5f, 3.2f }
        };
        blue_light.light_type = LightType::Point;
        blue_light.light_intensity = 8.0f;
        blue_light.light_range = 100.0f;
        blue_light.light_color = { 0.2f, 0.4f, 0.8f };
        this->scene.add_object(blue_light, "LightTorchBlue");

        auto green_light = GameSceneObject("LightTorchGreen");
        green_light.name = "Torch (green)";
        green_light.transform = (Transform) {
            .position = { 9.0f, 1.5f, -3.6f }
        };
        green_light.light_type = LightType::Point;
        green_light.light_intensity = 8.0f;
        green_light.light_range = 100.0f;
        green_light.light_color = { 0.2f, 0.8f, 0.4f };
        this->scene.add_object(green_light, "LightTorchGreen");

        auto red_light = GameSceneObject("LightTorchRed");
        red_light.name = "Torch (red)";
        red_light.transform = (Transform) {
            .position = { -9.5f, 1.5f, -3.65f }
        };
        red_light.light_type = LightType::Point;
        red_light.light_intensity = 8.0f;
        red_light.light_range = 100.0f;
        red_light.light_color = { 0.8f, 0.2f, 0.1f };
        this->scene.add_object(red_light, "LightTorchRed");

        auto purp_light = GameSceneObject("LightTorchPurple");
        purp_light.name = "Torch (purple)";
        purp_light.transform = (Transform) {
            .position = { -9.5f, 1.5f, 3.2f }
        };
        purp_light.light_type = LightType::Point;
        purp_light.light_intensity = 8.0f;
        purp_light.light_range = 100.0f;
        purp_light.light_color = { 0.8f, 0.2f, 0.8f };
        this->scene.add_object(purp_light, "LightTorchPurple");
    }

    void Update(int tick) {
        // Rotate and bob the froge
        this->scene.objects["froge"].transform.position.y += 0.0025f * sin(0.02f * tick);
        this->scene.objects["froge"].transform.rotation = glm::angleAxis(
            glm::radians(0.5f * tick),              // Rotation speed
            glm::normalize(glm::vec3(0, 1, 0))     // Rotate along Y-axis
        );
        this->scene.objects["froge"].dirty = true;

        // Rotate cube
        this->scene.objects["cube"].transform.rotation = glm::angleAxis(
            glm::radians(0.4f * tick),
            glm::normalize(glm::vec3(-0.8, 0.1, -0.4))
        );
        this->scene.objects["cube"].dirty = true;
    }
};
