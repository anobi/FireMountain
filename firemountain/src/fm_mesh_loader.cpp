#include <iostream>

#include <tiny_obj_loader.h>
#include <fmt/core.h>

#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "fm_mesh_loader.hpp"


const int FACE_VERTICES = 3;
constexpr bool OVERRIDE_COLORS = true;


VkFilter extract_filter(fastgltf::Filter filter) {
    switch(filter) {
        case fastgltf::Filter::Nearest:
        case fastgltf::Filter::NearestMipMapLinear:
        case fastgltf::Filter::NearestMipMapNearest:
            return VK_FILTER_NEAREST;

        case fastgltf::Filter::Linear:
        case fastgltf::Filter::LinearMipMapLinear:
        case fastgltf::Filter::LinearMipMapNearest:
        default:
            return VK_FILTER_LINEAR;
        
    }
}


VkSamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter) {
    switch (filter) {
        case fastgltf::Filter::NearestMipMapNearest:
        case fastgltf::Filter::LinearMipMapNearest:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;

        case fastgltf::Filter::NearestMipMapLinear:
        case fastgltf::Filter::LinearMipMapLinear:
        default:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
}


std::optional<AllocatedImage> load_image(fmVK::Vulkan* engine, fastgltf::Asset& asset, fastgltf::Image& image) {
    AllocatedImage new_image {};
    int width, height, nr_channels;

    std::visit(fastgltf::visitor {
        [](auto& arg) {},
        [&](fastgltf::sources::URI& file_path) {
            assert(file_path.fileByteOffset == 0);
            assert(file_path.uri.isLocalPath());

            const std::string path(file_path.uri.path().begin(), file_path.uri.path().end());
            unsigned char* data = stbi_load(path.c_str(), &width, &height, &nr_channels, 4);
            if (data) {
                VkExtent3D image_size = {
                    .width = width,
                    .height = height,
                    .depth = 1
                };
                new_image = engine->create_image(data, image_size, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);
                stbi_image_free(data);
            }
        },
        [&](fastgltf::sources::Vector& vector) {
            unsigned char* data = stbi_load_from_memory(
                vector.bytes.data(), 
                static_cast<int>(vector.bytes.size()),
                &width,
                &height,
                &nr_channels,
                4
            );
            if (data) {
                VkExtent3D image_size = {
                    .width = width,
                    .height = height,
                    .depth = 1
                };
                new_image = engine->create_image(data, image_size, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);
                stbi_image_free(data);
            }
        },
        [&](fastgltf::sources::BufferView& view) {
            auto& buffer_view = asset.bufferViews[view.bufferViewIndex];
            auto& buffer = asset.buffers[buffer_view.bufferIndex];
            std::visit(fastgltf::visitor {
                [](auto& arg) {},
                [&](fastgltf::sources::Vector& vector) {
                    unsigned char* data = stbi_load_from_memory(
                        vector.bytes.data() + buffer_view.byteOffset, 
                        static_cast<int>(buffer_view.byteLength),
                        &width,
                        &height,
                        &nr_channels,
                        4
                    );
                    if (data) {
                        VkExtent3D image_size = {
                            .width = width,
                            .height = height,
                            .depth = 1
                        };
                        new_image = engine->create_image(data, image_size, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);
                        stbi_image_free(data);
                    }
                }
            }, buffer.data);
        },
    }, image.data);

    if (new_image.image == VK_NULL_HANDLE) {
        return {};
    } else {
        return new_image;
    }
}

bool MeshLoader::LoadObj(const char *path, std::vector<Vertex> *vertices, std::vector<uint32_t> *indices)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warnings;
    std::string errors;
    tinyobj::LoadObj(&attrib, &shapes, &materials, &warnings, &errors, path, nullptr);
    if(!warnings.empty()) {
        fmt::println("WARNING: {}", warnings);
    }
    if(!errors.empty()) {
        fmt::println("{}", errors);
        return false;
    }

    for (size_t s = 0; s < shapes.size(); s++) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            for (size_t v = 0; v < FACE_VERTICES; v++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                indices->push_back(idx.vertex_index);

                tinyobj::real_t vertex_x = attrib.vertices[3 * idx.vertex_index + 0];
                tinyobj::real_t vertex_y = attrib.vertices[3 * idx.vertex_index + 1];
                tinyobj::real_t vertex_z = attrib.vertices[3 * idx.vertex_index + 2];

                tinyobj::real_t normal_x = attrib.normals[3 * idx.normal_index + 0];
                tinyobj::real_t normal_y = attrib.normals[3 * idx.normal_index + 1];
                tinyobj::real_t normal_z = attrib.normals[3 * idx.normal_index + 2];

                Vertex vertex = {
                    .position = {vertex_x, vertex_y, vertex_z},
                    .normal = {normal_x, normal_y, normal_z},
                    .color = {normal_x, normal_y, normal_z, 1.0f}
                };

                vertices->push_back(vertex);
            }
            index_offset += FACE_VERTICES;
        }
    }
    return true;
}


std::optional<std::shared_ptr<LoadedGLTF>> MeshLoader::load_GLTF(fmVK::Vulkan* engine, std::string_view file_path) {
    fmt::println("Loading GLTF: {}", file_path);

    std::shared_ptr<LoadedGLTF> scene = std::make_shared<LoadedGLTF>();
    scene->creator = engine;
    LoadedGLTF& file = *scene.get();

    fastgltf::Parser parser {};

    constexpr auto gltf_options = 
        fastgltf::Options::DontRequireValidAssetMember 
        | fastgltf::Options::AllowDouble
        | fastgltf::Options::LoadGLBBuffers
        | fastgltf::Options::LoadExternalBuffers;

    fastgltf::GltfDataBuffer data;
    data.loadFromFile(file_path);

    fastgltf::Asset gltf;
    std::filesystem::path path = file_path;

    auto type = fastgltf::determineGltfFileType(&data);
    if (type == fastgltf::GltfType::glTF) {
        auto load = parser.loadGltf(&data, path.parent_path(), gltf_options);
        if (load) {
            gltf = std::move(load.get());
        } else {
            std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
            return {};
        }
    } else if (type == fastgltf::GltfType::GLB) {
        auto load = parser.loadGltfBinary(&data, path.parent_path(), gltf_options);
        if (load) {
            gltf = std::move(load.get());
        } else {
            std::cerr << "Failed to load GLB: " << fastgltf::to_underlying(load.error()) << std::endl;
            return {};
        }

    } else {
        std::cerr << "Failed to determine glTF container type" << std::endl;
        return {};
    }

    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3}
    };
    file.descriptor_pool.init(engine->_device, gltf.materials.size(), sizes);
    
    for (fastgltf::Sampler& sampler : gltf.samplers) {
        VkSamplerCreateInfo sampler_info = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            .magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest)),
            .minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest)),
            .mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest)),
            .minLod = 0,
            .maxLod = VK_LOD_CLAMP_NONE
        };

        VkSampler new_sampler;
        vkCreateSampler(engine->_device, &sampler_info, nullptr, &new_sampler);

        file.samplers.push_back(new_sampler);
    }

    // Temporary containers to load everything into
    std::vector<std::shared_ptr<MeshAsset>> meshes;
    std::vector<std::shared_ptr<Node>> nodes;
    std::vector<AllocatedImage> images;
    std::vector<std::shared_ptr<GLTFMaterial>> materials;

    // Load textures
    for (fastgltf::Image& image : gltf.images) {
        images.push_back(engine->_texture_missing_error_image);
    }

    // TODO: need to "publish" the buffer creation function and GLTF Materials
    file.material_data_buffer = engine->create_buffer(
        sizeof(fmVK::GLTFMetallic_Roughness::MaterialConstants) * gltf.materials.size(),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );
    int data_index = 0;
    fmVK::GLTFMetallic_Roughness::MaterialConstants* scene_material_constants = (fmVK::GLTFMetallic_Roughness::MaterialConstants*)file.material_data_buffer.info.pMappedData;

    for (fastgltf::Material& mat : gltf.materials) {
        std::shared_ptr<GLTFMaterial> new_material = std::make_shared<GLTFMaterial>();
        materials.push_back(new_material);
        file.materials[mat.name.c_str()] = new_material;

        fmVK::GLTFMetallic_Roughness::MaterialConstants constants = {
            .color_factors = glm::vec4 {
                mat.pbrData.baseColorFactor[0],
                mat.pbrData.baseColorFactor[1],
                mat.pbrData.baseColorFactor[2],
                mat.pbrData.baseColorFactor[3],
            },
            .metal_roughness_factors = glm::vec4 {
                mat.pbrData.metallicFactor,
                mat.pbrData.roughnessFactor,
                0.0,
                0.0
            }
        };
        scene_material_constants[data_index] = constants;

        MaterialPass pass_type = MaterialPass::FM_MATERIAL_PASS_OPAQUE;
        if (mat.alphaMode == fastgltf::AlphaMode::Blend) {
            pass_type = MaterialPass::FM_MATERIAL_PASS_TRANSPARENT;
        }

        fmVK::GLTFMetallic_Roughness::MaterialResources material_resources = {
            .color_image = engine->_default_texture_white,
            .color_sampler = engine->_default_sampler_linear,
            .metal_roughness_image = engine->_default_texture_white,
            .metal_roughness_sampler = engine->_default_sampler_linear,
            .data_buffer = file.material_data_buffer.buffer,
            .data_buffer_offset = data_index * sizeof(fmVK::GLTFMetallic_Roughness::MaterialConstants)
        };

        if (mat.pbrData.baseColorTexture.has_value()) {
            size_t img = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
            size_t sampler = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();
            material_resources.color_image = images[img];
            material_resources.color_sampler = file.samplers[sampler];
        }

        new_material->data = engine->metal_roughness_material.write_material(engine->_device, pass_type, material_resources, file.descriptor_pool);
        data_index++;
    }

    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    for (fastgltf::Mesh& mesh : gltf.meshes) {
        std::shared_ptr<MeshAsset> new_mesh = std::make_shared<MeshAsset>();
        meshes.push_back(new_mesh);
        file.meshes[mesh.name.c_str()] = new_mesh;
        new_mesh->name = mesh.name;

        indices.clear();
        vertices.clear();

        for (auto&& p : mesh.primitives) {
            GeoSurface new_surface;
            new_surface.start_index = (uint32_t) indices.size();
            new_surface.count = (uint32_t) gltf.accessors[p.indicesAccessor.value()].count;
            size_t initial_vertex = vertices.size();
            
            {   // Load indices
                fastgltf::Accessor& index_accessor = gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + index_accessor.count);
                fastgltf::iterateAccessor<uint32_t>(gltf, index_accessor,
                    [&](uint32_t idx) {
                        indices.push_back(idx + initial_vertex);
                });
            }

            {   // Load vertex positions
                fastgltf::Accessor& position_accessor = gltf.accessors[p.findAttribute("POSITION")->second];
                vertices.resize(vertices.size() + position_accessor.count);
                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                    gltf,
                    position_accessor,
                    [&](glm::vec3 v, size_t index) {
                        Vertex new_vertex = {
                            .position = v,
                            .uv_x = 0,
                            .normal = { 1.0f, 0.0f, 0.0f },
                            .uv_y = 0,
                            .color = glm::vec4 { 1.0f }
                        };
                        vertices[initial_vertex + index] = new_vertex;
                });
            }

            // Load vertex normals
            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).second],
                [&](glm::vec3 v, size_t index) {
                    vertices[initial_vertex + index].normal = v;
                });
            }

            // Load UVs
            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*uv).second],
                [&](glm::vec2 v, size_t index) {
                    vertices[initial_vertex + index].uv_x = v.x;
                    vertices[initial_vertex + index].uv_y = v.y;
                });
            }

            auto colors = p.findAttribute("COLOR_0");
            if (colors != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).second],
                [&](glm::vec4 v, size_t index) {
                    vertices[initial_vertex + index].color = v;
                });
            }

            if (p.materialIndex.has_value()) {
                new_surface.material = materials[p.materialIndex.value()];
            } else {
                new_surface.material = materials[0];
            }

            // Calculate bounds for culling
            glm::vec3 min_pos = vertices[initial_vertex].position;
            glm::vec3 max_pos = vertices[initial_vertex].position;
            for (int i = initial_vertex; i < vertices.size(); i++) {
                min_pos = glm::min(min_pos, vertices[i].position);
                max_pos = glm::max(max_pos, vertices[i].position);
            }
            new_surface.bounds = {
                .origin = (max_pos + min_pos) / 2.0f,
                .sphere_radius = glm::length(new_surface.bounds.extents),
                .extents = (max_pos - min_pos) / 2.0f
            };

            new_mesh->surfaces.push_back(new_surface);
        }
        new_mesh->mesh_buffers = engine->UploadMesh(vertices, indices);
        
    }

    // Load nodes and their meshes
    for (fastgltf::Node& node : gltf.nodes) {
        std::shared_ptr<Node> new_node;

        if (node.meshIndex.has_value()) {
            new_node = std::make_shared<MeshNode>();
            static_cast<MeshNode*>(new_node.get())->mesh = meshes[*node.meshIndex];
        } else {
            new_node = std::make_shared<Node>();
        }

        nodes.push_back(new_node);
        file.nodes[node.name.c_str()];

        std::visit(fastgltf::visitor {
            [&](fastgltf::Node::TransformMatrix matrix) {
                memcpy(&new_node->local_transform, matrix.data(), sizeof(matrix));
            },
            [&](fastgltf::TRS trs) {
                glm::vec3 t(trs.translation[0], trs.translation[1], trs.translation[2]);
                glm::quat r(trs.rotation[3], trs.rotation[0], trs.rotation[1], trs.rotation[2]);
                glm::vec3 s(trs.scale[0], trs.scale[1], trs.scale[2]);

                glm::mat4 tm = glm::translate(glm::mat4(1.0f), t);
                glm::mat4 rm = glm::toMat4(r);
                glm::mat4 sm = glm::scale(glm::mat4(1.0f), s);

                new_node->local_transform = tm * rm * sm;
            }
        }, node.transform);
    }

    // Setup transform hierarchy
    for (int i= 0; i < gltf.nodes.size(); i++) {
        fastgltf::Node& node = gltf.nodes[i];
        std::shared_ptr<Node>& scene_node = nodes[i];
        for (auto& c : node.children) {
            scene_node->children.push_back(nodes[c]);
            nodes[c]->parent = scene_node;
        }
    }

    // Find the top nodes (nodes with no parents)
    for (auto& node : nodes) {
        if (node->parent.lock() == nullptr) {
            file.top_nodes.push_back(node);
            node->refresh_transform(glm::mat4 { 1.0f });
        }
    }

    // Load textures
    for (fastgltf::Image& image : gltf.images) {
        std::optional<AllocatedImage> img = load_image(engine, gltf, image);
        if (img.has_value()) {
            images.push_back(*img);
            file.images[image.name.c_str()] = *img;
        } else {
            images.push_back(engine->_texture_missing_error_image);
            fmt::println("[GLTF] failed to load texture ", image.name);
        }
    }

    return scene;
}

VertexInputDescription Vertex::get_vertex_description() {
    VertexInputDescription description;

    VkVertexInputBindingDescription main_binding = {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    description.bindings.push_back(main_binding);

    VkVertexInputAttributeDescription attrib_position = {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, position)
    };
    description.attributes.push_back(attrib_position);
    
    VkVertexInputAttributeDescription attrib_normal = {
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, normal)
    };
    description.attributes.push_back(attrib_normal);

    VkVertexInputAttributeDescription attrib_color = {
        .location = 2,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, color)
    };
    description.attributes.push_back(attrib_color);

    return description;
}




void LoadedGLTF::Draw(const glm::mat4 &top_matrix, DrawContext &ctx)
{
    for (auto& n : this->top_nodes) {
        n->Draw(top_matrix, ctx);
    }
}

void LoadedGLTF::clear_all()
{
    VkDevice device = creator->_device;
    this->descriptor_pool.destroy_pools(device);
    this->creator->destroy_buffer(this->material_data_buffer);

    for (auto& [k, v] : meshes) {
        this->creator->destroy_buffer(v->mesh_buffers.index_buffer);
        this->creator->destroy_buffer(v->mesh_buffers.vertex_buffer);
    }

    for (auto& [k, v] : images) {
        // Don't destroy the default images
        if (v.image == this->creator->_texture_missing_error_image.image) {
            continue;
        }
        this->creator->destroy_image(v);
    }

    for (auto& sampler : samplers) {
        vkDestroySampler(device, sampler, nullptr);
    }
}
