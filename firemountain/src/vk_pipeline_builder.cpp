#include <iostream>
#include "vk_pipeline_builder.hpp"


VkPipeline fmVK::PipelineBuilder::build_pipeline(VkDevice device, VkRenderPass pass)
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
        .pAttachments = &this->color_blend_attachment
    };

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .stageCount = (uint32_t) this->shader_stages.size(),
        .pStages = this->shader_stages.data(),
        .pVertexInputState = &this->vertex_input_info,
        .pInputAssemblyState = &this->input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &this->rasterizer,
        .pMultisampleState = &this->multisampling,
        .pDepthStencilState = &this->depth_stencil,
        .pColorBlendState = &color_blending,
        .layout = this->pipeline_layout,
        .renderPass = pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE
    };

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline) != VK_SUCCESS) {
        std::cout << "failed to create pipeline" << std::endl;
        return VK_NULL_HANDLE;
    }
    return pipeline;
}