#include <iostream>

#include <SDL2/SDL.h>
#include <glm/gtx/transform.hpp>

#include "firemountain.hpp"
#include "fm_mesh_loader.hpp"


int Firemountain::Init(const int width, const int height, SDL_Window* window) {

    this->_main_camera.position = glm::vec3(0.0f, 0.0f, 5.0f);

    this->vulkan.Init(width, height, window);
    this->vulkan._camera = &this->_main_camera;

    return 0;
}

void Firemountain::Frame() {
    this->vulkan.Draw(this->_renderables.data(), this->_renderables.size());
    // this->vulkan.Draw(this->loaded_Scenes, this->loaded_Scenes.size());
}

void Firemountain::Resize(const uint32_t width, const uint32_t height)
{
    this->vulkan.Resize(width, height);
}

void Firemountain::Destroy() {
    this->vulkan.Destroy();
    this->loaded_Scenes.clear();
    this->vulkan.loaded_Scenes.clear();
}

void Firemountain::ProcessImGuiEvent(SDL_Event* e)
{
    this->vulkan.ProcessImGuiEvent(e);
}

int mesh_index = 0;

bool Firemountain::AddMesh(const std::string& name, const char* path) {
    // this->_meshes[name] = MeshLoader::LoadGltf(path, &this->vulkan);
    auto mesh_file = MeshLoader::load_GLTF(&this->vulkan, path);
    assert(mesh_file.has_value());
    //this->loaded_Scenes[name] = *mesh_file;
    this->vulkan.loaded_Scenes[name] = *mesh_file;

    // RenderObject render_object;
    // render_object.vertex_buffer_address = this->_meshes[name][0]->mesh_buffers.vertex_buffer_address;
    // render_object.index_count = this->_meshes[name][0]->surfaces[0].count;
    // render_object.first_index = this->_meshes[name][0]->surfaces[0].start_index;
    // render_object.material = this->get_material("mesh");
    // render_object.transform = glm::translate(glm::vec3{
    //     0.0f + mesh_index * 2.0f,
    //     0.0f,
    //     0.0f
    // });

    // for (auto& m : this->_meshes[name]) {
    //     std::shared_ptr<MeshNode> node = std::make_shared<MeshNode>();
    //     node->mesh = m;
    //     node->local_transform = glm::mat4{ 1.0f };
    //     node->world_transform = glm::mat4{ 1.0f };
    //     for (auto& s : node->mesh->surfaces) {
    //         s.material = std::make_shared<GLTFMaterial>(this->vulkan.default_data);
    //     }

    //     this->vulkan.loaded_nodes[name] = std::move(node);
    // }

    // this->_renderables.push_back(render_object);
    // mesh_index += 1;
    
    return true;
}

// TODO: Calculate position in the actual engine, and just pass those things
// here to calculate the matrices, which are stored in fireomountain camera
void Firemountain::UpdateCamera(float pitch, float yaw, glm::vec3 velocity)
{
    this->_main_camera.pitch -= pitch;
    this->_main_camera.yaw += yaw;
    this->_main_camera.velocity = velocity;
}

// MaterialInstance* Firemountain::create_material(const std::string& name) {
//     MaterialInstance mat = {
//         .pipeline = this->vulkan.GetPipeline("mesh"),
//         .pipeline_layout = this->vulkan.GetPipelineLayout("mesh")
//     };
//     this->_materials[name] = mat;
//     return &this->_materials[name];
// }

MaterialInstance* Firemountain::get_material(const std::string& name) {
    auto i = this->_materials.find(name);
    if (i == this->_materials.end()) {
        return nullptr;
    } else {
        return &(*i).second;
    }
}