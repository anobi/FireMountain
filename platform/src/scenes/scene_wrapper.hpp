#pragma once
#include "game_scene.hpp"
#include "vk_types.hpp"

class ISceneWrapper {
public:
    virtual void Init() = 0;
    virtual void Update(int tick) = 0;
    virtual ~ISceneWrapper() {};

    GameScene scene;

    std::vector<RenderSceneObj> GetRenderScene() {
        std::vector<RenderSceneObj> render_scene;

        // Send meshes with updated transforms to renderer
        for (auto& [key, obj] : this->scene.objects) {
            // Push meshes

            // TODO:
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
        return render_scene;
    };
};
