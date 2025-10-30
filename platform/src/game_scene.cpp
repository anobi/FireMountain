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

void GameScene::load(const char* scene_name, sqlite3* db) {
    sqlite3_stmt* stmt;

    // Prepare the query
    auto q = fmt::format("SELECT id FROM game_scenes WHERE name=?;");
    int rc = sqlite3_prepare_v2(db, q.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fmt::println("SQL Error: {}", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
    }

    // Bind the name to ? in the query
    rc = sqlite3_bind_text(stmt, 1, scene_name, strlen(scene_name), SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        fmt::println("SQL Error: {}", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        fmt::println("SQL Error: {}", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
    }

    if (rc == SQLITE_DONE) {
        fmt::println("SQL: scene not found ({})", scene_name);
        sqlite3_finalize(stmt);
    }

    int scene_id = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);


    // Load the scene objects
    // ----------------------

    // Prepare the query
    q = fmt::format("SELECT " \
        // Scene Object
        // 0      1        2           3
        "o.id, o.name, o.mesh_id, o.light_id, " \
        //  4        5        6
        "o.pos_x, o.pos_y, o.pos_z, " \
        //  7        8        9       10
        "o.rot_x, o.rot_y, o.rot_z, o.rot_w, " \
        //  11         12         13
        "o.scale_x, o.scale_y, o.scale_z, " \
        // Mesh
        //    14
        "m.file_name," \
        // Lights
        //    15            16         17
        "l.light_type, l.intensity, l.radius, " \
        //   18        19         20
        "l.color_r, l.color_g, l.color_b, " \
        //  21       22       23
        "l.dir_x, l.dir_y, l.dir_z " \
        "FROM scene_objects o " \
        "LEFT OUTER JOIN meshes m ON m.id = o.mesh_id " \
        "LEFT OUTER JOIN scene_lights l ON l.id = o.light_id " \
        "WHERE o.scene_id=?;");
    rc = sqlite3_prepare_v2(db, q.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fmt::println("SQL Error: {}", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
    }

    // Bind the name to ? in the query
    rc = sqlite3_bind_int(stmt, 1, scene_id);
    if (rc != SQLITE_OK) {
        fmt::println("SQL Error: {}", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        GameSceneObject obj;
        // int obj_id = sqlite3_column_int(stmt, 0);
        auto asd = sqlite3_column_text(stmt, 1);
        obj.name = std::string(reinterpret_cast<const char*>(asd));
        int mesh_id = sqlite3_column_int(stmt, 2);
        int light_id = sqlite3_column_int(stmt, 3);

        obj.transform.position.x = atof(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        obj.transform.position.y = atof(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        obj.transform.position.z = atof(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));

        obj.transform.rotation.x = atof(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
        obj.transform.rotation.y = atof(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)));
        obj.transform.rotation.z = atof(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9)));
        obj.transform.rotation.w = atof(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10)));

        obj.transform.scale.x = atof(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11)));
        obj.transform.scale.y = atof(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12)));
        obj.transform.scale.z = atof(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13)));

        if (mesh_id) {
            obj.mesh_file = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14)));
        }

        if(light_id) {
            // This could be a int cast to enum, but meh
            auto light_type = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 15)));
            if (light_type ==  "area") obj.light_type = LightType::Area;
            else if (light_type == "point") obj.light_type = LightType::Point;
            else if (light_type == "spot") obj.light_type = LightType::Spot;

            obj.light_intensity = atof(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 16)));
            obj.light_range = atof(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 17)));

            obj.light_color.r = atof(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 18)));
            obj.light_color.g = atof(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 19)));
            obj.light_color.b = atof(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 20)));

            obj.light_direction.x = atof(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 21)));
            obj.light_direction.y = atof(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 22)));
            obj.light_direction.z = atof(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 23)));
            obj.is_light = true;
        }

        this->objects[obj.name] = obj;
    }
    sqlite3_finalize(stmt);
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
        if (not o.mesh_file.empty()) {
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
