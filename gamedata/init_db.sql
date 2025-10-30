CREATE TABLE IF NOT EXISTS game_scenes (
    id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS meshes (
    id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    file_name TEXT UNIQUE NOT NULL,
    mesh_data BLOB,
    mesh_hash TEXT UNIQUE
);

CREATE TABLE IF NOT EXISTS scene_lights (
    id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    light_type TEXT,

    intensity TEXT NOT NULL DEFAULT "0.0",
    radius TEXT NOT NULL DEFAULT "0.0",

    color_r TEXT NOT NULL DEFAULT "1.0",
    color_g TEXT NOT NULL DEFAULT "1.0",
    color_b TEXT NOT NULL DEFAULT "1.0",

    dir_x TEXT NOT NULL DEFAULT "0.0",
    dir_y TEXT NOT NULL DEFAULT "0.0",
    dir_z TEXT NOT NULL DEFAULT "0.0"
);

CREATE TABLE IF NOT EXISTS scene_cameras (
    id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    active INTEGER NOT NULL DEFAULT 0,
    projection_type TEXT NOT NULL DEFAULT "perspective"
);

CREATE TABLE IF NOT EXISTS scene_objects (
    id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    scene_id INTEGER NOT NULL,
    mesh_id INTEGER,
    light_id INTEGER,
    camera_id INTEGER,

    pos_x TEXT NOT NULL DEFAULT "0.0",
    pos_y TEXT NOT NULL DEFAULT "0.0",
    pos_z TEXT NOT NULL DEFAULT "0.0",

    rot_x TEXT NOT NULL DEFAULT "0.0",
    rot_y TEXT NOT NULL DEFAULT "0.0",
    rot_z TEXT NOT NULL DEFAULT "0.0",
    rot_w TEXT NOT NULL DEFAULT "0.0",

    scale_x TEXT NOT NULL DEFAULT "1.0",
    scale_y TEXT NOT NULL DEFAULT "1.0",
    scale_z TEXT NOT NULL DEFAULT "1.0",

    FOREIGN KEY (scene_id) REFERENCES game_scenes (id),
    FOREIGN KEY (mesh_id) REFERENCES meshes (id),
    FOREIGN KEY (light_id) REFERENCES scene_lights (id),
    FOREIGN KEY (camera_id) REFERENCES scene_cameras (id),

    UNIQUE (scene_id, name)
);
