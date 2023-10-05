#include "fm_scene.hpp"


void Firemountain::Scene::Init() {

}

void Firemountain::Scene::Destroy() {
    
}


void Firemountain::Scene::AddMesh(const std::string& name, Mesh mesh) {
    this->_meshes[name] = mesh;
}

Material* Firemountain::Scene::create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name) {
    Material mat = {
        .pipeline = pipeline,
        .pipeline_layout = layout
    };
    this->_materials[name] = mat;
    return &this->_materials[name];
}

Material* Firemountain::Scene::get_material(const std::string& name) {
    auto i = this->_materials.find(name);
    if (i == this->_materials.end()) {
        return nullptr;
    } else {
        return &(*i).second;
    }
}

Mesh* Firemountain::Scene::get_mesh(const std::string& name) {
    auto i = this->_meshes.find(name);
    if (i == this->_meshes.end()) {
        return nullptr;
    } else {
        return &(*i).second;
    }
}