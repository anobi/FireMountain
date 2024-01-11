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
    VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags);
    VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer cmd);

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
    VkSemaphoreSubmitInfo semaphore_submit_info(VkPipelineStageFlags2 stage_mask, VkSemaphore semaphore);

    VkSubmitInfo2 submit_info(
        VkCommandBufferSubmitInfo* cmd, 
        VkSemaphoreSubmitInfo* signal_semaphore_info,
        VkSemaphoreSubmitInfo* wait_semaphore_info
    );


    VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usage_flags, VkExtent3D extent);
    VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspect_mask);
    VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspect_flags);
    VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info(bool depth_test, bool depth_write, VkCompareOp compare_op);
}