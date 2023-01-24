#pragma once

#include "vk_types.hpp"

namespace VKInit {
    VkCommandPoolCreateInfo command_pool_create_info(
        uint32_t queue_family_index,
        VkCommandPoolCreateFlags flags = 0
    );

    VkCommandBufferAllocateInfo command_buffer_allocate_info(
        VkCommandPool pool,
        uint32_t count = 1,
        VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
    );

    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(
        VkShaderStageFlagBits stage,
        VkShaderModule shader_module
    );

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info();

    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info(
        VkPrimitiveTopology topology
    );

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info(
        VkPolygonMode polygon_mode
    );

    VkPipelineMultisampleStateCreateInfo multisampling_state_create_info();
    VkPipelineColorBlendAttachmentState color_blend_attachment_state();

    VkPipelineLayoutCreateInfo pipeline_layout_create_info();

    VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);
    VkSemaphoreCreateInfo samephore_create_info(VkSemaphoreCreateFlags flags = 0);
}