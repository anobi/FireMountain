//
// Created by anobi on 2025-08-12.
//

#pragma once
#include <string>
#include <unordered_map>
#include <vector>

std::unordered_map<std::string, std::vector<std::string>>
py_build_shaders(const std::string& shader_folder,
                 const std::string& module_dir, // dir that contains shaderbuilder.py
                 const std::string& module_name = "shaderbuilder",
                 const std::string& func_name   = "build_shaders");
