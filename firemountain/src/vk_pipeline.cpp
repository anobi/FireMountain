#include <iostream>
#include <fstream>
#include <string>
#include <VkBootstrap.h>

#include "vk_pipeline.hpp"
#include "vk_init.hpp"
#include "vk_mesh.hpp"
#include "vk_pipeline_builder.hpp"


int fmVK::Pipeline::Init(const VkDevice device, const VkExtent2D window_extent, const char* shader_name)
{
    PipelineBuilder pipeline_builder;
    pipeline_builder.viewport = VkViewport {
            .x = 0.0f,
            .y = 0.0f,
            .width = (float) window_extent.width,
            .height = (float) window_extent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f
    };
    pipeline_builder.scissor = VkRect2D {
            .offset = {0, 0},
            .extent = window_extent
    };
    pipeline_builder._pipeline_layout = this->layout;
    pipeline_builder._input_assembly = VKInit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipeline_builder._rasterizer = VKInit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    pipeline_builder._color_blend_attachment = VKInit::color_blend_attachment_state();
    pipeline_builder._vertex_input_info = VKInit::vertex_input_state_create_info();
    pipeline_builder._multisampling = VKInit::multisampling_state_create_info();
    pipeline_builder._depth_stencil = VKInit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);


    // Compute layout
    // -------------------------------------------------------------------------
    VkPipelineLayoutCreateInfo compute_layout = VKInit::pipeline_layout_create_info();
    compute_layout.pSetLayouts = &this->_draw_image_descriptor_layout;
    compute_layout.setLayoutCount = 1;
    VkPushConstantRange compute_constants = {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(ComputePushConstants)
    };
    VK_CHECK(vkCreatePipelineLayout(device, &compute_layout, nullptr, &this->layout));


    // Pipeline layout
    // -------------------------------------------------------------------------
    VkPipelineLayoutCreateInfo pipeline_layout_info = VKInit::pipeline_layout_create_info();
    VkPushConstantRange push_constant = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(MeshPushConstants)
    };
    pipeline_layout_info.pPushConstantRanges = &push_constant;
    pipeline_layout_info.pushConstantRangeCount = 1;
    VK_CHECK(vkCreatePipelineLayout(
        device,
        &pipeline_layout_info,
        nullptr,
        &this->layout
    ));
    pipeline_builder._pipeline_layout = this->layout;


    // Shaders
    // -------------------------------------------------------------------------
    VertexInputDescription vertex_description = Vertex::get_vertex_description();
    pipeline_builder._vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = (uint32_t) vertex_description.bindings.size(),
        .pVertexBindingDescriptions = vertex_description.bindings.data(),
        .vertexAttributeDescriptionCount = (uint32_t) vertex_description.attributes.size(),
        .pVertexAttributeDescriptions = vertex_description.attributes.data()
    };
    pipeline_builder._shader_stages.clear();

    // TODO: Get shader paths from pipeline name
    if (!load_shader_module("shaders/mesh.frag.spv", device, &this->fragment_shader)) {
        std::cout << "Error building fragment shader module" << std::endl;
    }
    else {
        std::cout << "Fragment shader module loaded." << std::endl;
    }

    if (!load_shader_module("shaders/mesh.vert.spv", device, &this->vertex_shader)) {
        std::cout << "Error building vertex shader module" << std::endl;
    } 
    else {
        std::cout << "Vertex shader module loaded." << std::endl;
    }

    if (!load_shader_module("shaders/mesh.comp.spv", device, &this->compute_shader)) {
        std::cout << "Error building compute shader module" << std::endl;
    } 
    else {
        std::cout << "Compute shader module loaded." << std::endl;
    }

    pipeline_builder.set_shaders(this->vertex_shader, this->fragment_shader);

    pipeline_builder.enable_blending_additive();

    this->pipeline = pipeline_builder.build_pipeline(device);

    vkDestroyShaderModule(device, this->fragment_shader, nullptr);
    vkDestroyShaderModule(device, this->vertex_shader, nullptr);

    return true;
}

void fmVK::Pipeline::Cleanup(const VkDevice device) {
    vkDestroyPipeline(device, this->pipeline, nullptr);
    vkDestroyPipelineLayout(device, this->layout, nullptr);
}


bool fmVK::Pipeline::load_shader_module(const char *file_path, const VkDevice device, VkShaderModule *out)
{
    // TODO: Move this to a file reading utility function
    std::ifstream file(file_path, std::ios::ate | std::ios::binary);
    if(!file.is_open()) {
        return false;
    }

    size_t file_size = (size_t) file.tellg();
    std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));

    file.seekg(0);
    file.read((char*) buffer.data(), file_size);
    file.close();
    // End of TODO

    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .codeSize = buffer.size() * sizeof(uint32_t),
        .pCode = buffer.data()
    };

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
        return false;
    }

    *out = shader_module;
    return true;
}