#include <chrono>

#include "fm_scene.hpp"


void Scene::Init() {
}

void Scene::Destroy() {
    
}

void Scene::Update(const fmCamera* camera) {
    auto start = std::chrono::system_clock::now();

    this->_main_draw_context.opaque_surfaces.clear();

    if (!this->ghost_mode && camera->debug_pov_lock) {
        this->ghost_mode = true;
        this->ghost_view = this->scene_data.view;
        this->ghost_projection = this->scene_data.projection;
        this->ghost_camera_position = camera->position;
    }
    else if (this->ghost_mode && !camera->debug_pov_lock) {
        this->ghost_mode = false;
    }

    this->scene_data.camera_position = camera->position;
    this->scene_data.view = camera->view;
    this->scene_data.projection = camera->projection;

    // TODO: Light culling
    size_t scene_light_idx = 0;
    for (auto o : scene) {
        if (o.light_id) {
            GPULightData light = {
                .positionType = o.light_position_type,
                .colorIntensity = o.light_color_intensity,
                .directionRange = o.light_direction_range
            };
            this->scene_data.lights[scene_light_idx] = light;
            scene_light_idx += 1;
        }
        if (o.mesh_id) {
            auto mesh = this->loaded_meshes.at(o.mesh_id.id);
            mesh->Draw(o.transform, this->_main_draw_context);
        }
    }
    this->scene_data.light_count = scene_light_idx;

    auto end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    stats.scene_update_time = elapsed.count() / 1000.f;
}

LightID Scene::AddLight(const std::string &name)
{
    auto id = ++this->next_id;
    this->lights.push_back(id);
    return { id };
}