#pragma once

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <unordered_map>
#include <sqlite3.h>

#include "firemountain.hpp"


struct Transform {
    glm::vec3 position = { 0.0f, 0.0f, 0.0f};
    glm::quat rotation = { 1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale = { 1.0f, 1.0f, 1.0f};
};

struct GameSceneObject {
    const char* mesh_file;
    const char* name;
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


struct GameScene {
    std::string id;
    std::string name;
    std::unordered_map<std::string, GameSceneObject> objects;

    void load(std::string id, sqlite3* db);
    void save(sqlite3* db);
};
