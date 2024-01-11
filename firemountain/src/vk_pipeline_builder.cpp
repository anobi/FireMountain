#include <iostream>
#include "vk_pipeline_builder.hpp"
#include "vk_init.hpp"


VkPipeline fmVK::PipelineBuilder::build_pipeline(VkDevice device)
{
    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .viewportCount = 1,
        .pViewports = &this->viewport,
        .scissorCount = 1,
        .pScissors = &this->scissor,
    };

    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &this->_color_blend_attachment
    };

    VkDynamicState state[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = &state[0]
    };

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .stageCount = (uint32_t) this->_shader_stages.size(),
        .pStages = this->_shader_stages.data(),
        .pVertexInputState = &this->_vertex_input_info,
        .pInputAssemblyState = &this->_input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &this->_rasterizer,
        .pMultisampleState = &this->_multisampling,
        .pDepthStencilState = &this->_depth_stencil,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_info,
        .layout = this->_pipeline_layout,
        .basePipelineHandle = VK_NULL_HANDLE
    };

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline) != VK_SUCCESS) {
        std::cout << "failed to create pipeline" << std::endl;
        return VK_NULL_HANDLE;
    }
    return pipeline;
}

void fmVK::PipelineBuilder::clear() {
    this->_input_assembly = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    this->_rasterizer = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    this->_color_blend_attachment = {};
    this->_multisampling = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    this->_pipeline_layout = {};
    this->_depth_stencil = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    this->_render_info = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
    this->_shader_stages.clear();
}

void fmVK::PipelineBuilder::set_shaders(VkShaderModule vertex_shader, VkShaderModule fragment_shader) {
    this->_shader_stages.clear();
    this->_shader_stages.push_back(VKInit::pipeline_shader_stage_create_info(
        VK_SHADER_STAGE_VERTEX_BIT, vertex_shader
    ));
    this->_shader_stages.push_back(VKInit::pipeline_shader_stage_create_info(
        VK_SHADER_STAGE_FRAGMENT_BIT, fragment_shader
    ));
}

void fmVK::PipelineBuilder::set_input_topology(VkPrimitiveTopology topology) {
    this->_input_assembly.topology = topology;
    this->_input_assembly.primitiveRestartEnable = VK_FALSE;
}

void fmVK::PipelineBuilder::set_polygon_mode(VkPolygonMode mode) {
    this->_rasterizer.polygonMode = mode;
    this->_rasterizer.lineWidth = 1.0f;
}

void fmVK::PipelineBuilder::set_cull_mode(VkCullModeFlags cull_mode, VkFrontFace front_face) {
    this->_rasterizer.cullMode = cull_mode;
    this->_rasterizer.frontFace = front_face;
}

void fmVK::PipelineBuilder::set_multisampling_none() {
    this->_multisampling.sampleShadingEnable = VK_FALSE;
    this->_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    this->_multisampling.minSampleShading = 1.0f;
    this->_multisampling.pSampleMask = nullptr;
    this->_multisampling.alphaToCoverageEnable = VK_FALSE;
    this->_multisampling.alphaToOneEnable = VK_FALSE;
}

void fmVK::PipelineBuilder::set_color_attachment_format(VkFormat format) {
    this->_color_attachment_format = format;
    this->_render_info.colorAttachmentCount = 1;
    this->_render_info.pColorAttachmentFormats = &_color_attachment_format;
}

void fmVK::PipelineBuilder::set_depth_format(VkFormat format) {
    this->_render_info.depthAttachmentFormat = format;
}

void fmVK::PipelineBuilder::disable_blending() {
    this->_color_blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT
        | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT
        | VK_COLOR_COMPONENT_A_BIT;
    this->_color_blend_attachment.blendEnable = VK_FALSE;
}

void fmVK::PipelineBuilder::disable_depth_test() {
    this->_depth_stencil.depthBoundsTestEnable = VK_FALSE;
    this->_depth_stencil.depthWriteEnable = VK_FALSE;
    this->_depth_stencil.depthCompareOp = VK_COMPARE_OP_NEVER;
    this->_depth_stencil.depthBoundsTestEnable = VK_FALSE;
    this->_depth_stencil.stencilTestEnable = VK_FALSE;
    this->_depth_stencil.front = {};
    this->_depth_stencil.back = {};
    this->_depth_stencil.minDepthBounds = 0.0f;
    this->_depth_stencil.maxDepthBounds = 1.0f;
}
