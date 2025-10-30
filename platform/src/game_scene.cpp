#include <fmt/core.h>
#include <sqlite3.h>

#include "game_scene.hpp"
#include "fmt/base.h"
#include "fmt/format.h"
#include "vk_types.hpp"

static int callback(void *data, int argc, char **argv, char **col_names){
    int i;
    for(i = 0; i < argc; i++){
        fmt::println("{} = {}", col_names[i], argv[i] ? argv[i] : "NULL");
        if (strcmp(col_names[i], "id") == 0) {
            data = argv[1];
        }
    }
    return 0;
}

void GameScene::load(std::string scene_name, sqlite3* db) {
    auto scene_q = fmt::format("SELECT * FROM game_scenes WHERE name = {};", scene_name);
}

void GameScene::save(sqlite3* db) {
    char* errmsg = 0;
    auto sq = fmt::format(
        "INSERT INTO game_scenes (name) VALUES ('{0}') " \
        "ON CONFLICT (name) DO UPDATE SET name='{0}';"
        , this->name);
    int rc = sqlite3_exec(db, sq.c_str(), callback, 0, &errmsg);
    if (rc != SQLITE_OK) {
        fmt::println("SQL Error: {}", errmsg);
        sqlite3_free(errmsg);
    }
    auto s_id = sqlite3_last_insert_rowid(db);
    fmt::println("{}", s_id);

    for(auto [n, o] : this->objects) {
        auto t = o.transform;
        auto oq = fmt::format(
            "INSERT INTO scene_objects " \
            "(name, scene_id, pos_x, pos_y, pos_z, rot_x, rot_y, rot_z, rot_w, scale_x, scale_y, scale_z) " \
            "VALUES ('{}', {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {});"
            , o.name, s_id, t.position.x, t.position.y, t.position.z
            , t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w
            , t.scale.x, t.scale.y, t.scale.z);
        rc = sqlite3_exec(db, oq.c_str(), callback, 0, &errmsg);
        if (rc != SQLITE_OK) {
            fmt::println("SQL Error (scene_objects): {}", errmsg);
            fmt::println("{}", oq);
            sqlite3_free(errmsg);
        }
        auto o_id = sqlite3_last_insert_rowid(db);

        // Save and connect scene meshes
        if (o.mesh_file) {
            auto mq = fmt::format("INSERT INTO meshes (file_name) VALUES ('{}');", o.mesh_file);
            rc = sqlite3_exec(db, mq.c_str(), 0, 0, &errmsg);
            if (rc != SQLITE_OK) {
                fmt::println("SQL Error (meshes): {}", errmsg);
                fmt::println("{}", mq);
                sqlite3_free(errmsg);
            }
            auto m_id = sqlite3_last_insert_rowid(db);

            auto muq = fmt::format("UPDATE scene_objects SET mesh_id = {} WHERE id = {};", m_id, o_id);
            rc = sqlite3_exec(db, muq.c_str(), callback, 0, &errmsg);
            if (rc != SQLITE_OK) {
                fmt::println("SQL Error (updating scene mesh): {}", errmsg);
                fmt::println("{}", muq);
                sqlite3_free(errmsg);
            }
        }

        // Save and connect light stuffs if the object is a light
        if (o.light_type) {
            std::string light_type_s;
            switch (o.light_type) {
                case LightType::Area: light_type_s = "area";
                case LightType::Point: light_type_s = "point";
                case LightType::Spot: light_type_s = "spot";
                default: break;
            }
            auto lq = fmt::format(
                "INSERT INTO scene_lights " \
                "(light_type, intensity, radius, color_r, color_g, color_b, dir_x, dir_y, dir_z) " \
                "VALUES ('{}', {}, {}, {}, {}, {}, {}, {}, {});"
                , light_type_s, o.light_intensity, o.light_range
                , o.light_color.r, o.light_color.g, o.light_color.b
                , o.light_direction.x, o.light_direction.y, o.light_direction.z);
            rc = sqlite3_exec(db, lq.c_str(), 0, 0, &errmsg);
            if (rc != SQLITE_OK) {
                fmt::println("SQL Error (scene_lights): {}", errmsg);
                fmt::println("{}", lq);
                sqlite3_free(errmsg);
            }
            auto l_id = sqlite3_last_insert_rowid(db);

            auto luq = fmt::format("UPDATE scene_objects SET light_id = {} WHERE id = {};", l_id, o_id);
            rc = sqlite3_exec(db, luq.c_str(), callback, 0, &errmsg);
            if (rc != SQLITE_OK) {
                fmt::println("SQL Error (updating scene light): {}", errmsg);
                fmt::println("{}", luq);
                sqlite3_free(errmsg);
            }
        }
    }
    fmt::println("* Scene saved! [({}){}]", s_id, this->name);
}
