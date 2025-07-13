#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "fm_renderable.hpp"
#include "vk_types.hpp"
#include "vk_mesh.hpp"


class Scene {
public:
    void Init();
    void Destroy();
    void Update(const fmCamera* camera);

    LightID AddLight(const std::string& name);


private:
    unsigned int next_id = 0;
    std::vector<unsigned int> lights;

    std::vector<RenderObject> _render_objects;
};