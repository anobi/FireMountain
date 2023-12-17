#include <vk_init.hpp>

VkCommandPoolCreateInfo VKInit::command_pool_create_info(
    uint32_t queue_family_index,
    VkCommandPoolCreateFlags flags
) {
    VkCommandPoolCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .queueFamilyIndex = queue_family_index
    };
    return info;
}

VkCommandBufferAllocateInfo VKInit::command_buffer_allocate_info(
    VkCommandPool pool,
    uint32_t count,
    VkCommandBufferLevel level
) {
    VkCommandBufferAllocateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = pool,
        .level = level,
        .commandBufferCount = count
    };
    return info;
}

VkPipelineShaderStageCreateInfo VKInit::pipeline_shader_stage_create_info(
        VkShaderStageFlagBits stage,
        VkShaderModule shader_module
) {
    VkPipelineShaderStageCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .stage = stage,
        .module = shader_module,
        .pName = "main"
    };
    return info;
}

VkPipelineVertexInputStateCreateInfo VKInit::vertex_input_state_create_info() {
    VkPipelineVertexInputStateCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .vertexBindingDescriptionCount = 0,
        .vertexAttributeDescriptionCount = 0
    };
    return info;
}

VkPipelineInputAssemblyStateCreateInfo VKInit::input_assembly_create_info(
        VkPrimitiveTopology topology
) {
    VkPipelineInputAssemblyStateCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .topology = topology,
        .primitiveRestartEnable = VK_FALSE
    };
    return info;
}

VkPipelineRasterizationStateCreateInfo VKInit::rasterization_state_create_info(
        VkPolygonMode polygon_mode
) {
    VkPipelineRasterizationStateCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = polygon_mode,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f
    };
    return info;
}

VkPipelineMultisampleStateCreateInfo VKInit::multisampling_state_create_info() {
    VkPipelineMultisampleStateCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };
    return info;
}

VkPipelineColorBlendAttachmentState VKInit::color_blend_attachment_state() {
    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .blendEnable = VK_FALSE,
        .colorWriteMask = 
              VK_COLOR_COMPONENT_R_BIT 
            | VK_COLOR_COMPONENT_G_BIT 
            | VK_COLOR_COMPONENT_B_BIT 
            | VK_COLOR_COMPONENT_A_BIT
    };
    return color_blend_attachment;
};

VkPipelineLayoutCreateInfo VKInit::pipeline_layout_create_info() {
    VkPipelineLayoutCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 0,
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };
    return info;
}

VkFenceCreateInfo VKInit::fence_create_info(VkFenceCreateFlags flags) {
    VkFenceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags
    };
    return info;
}

VkSemaphoreCreateInfo VKInit::samephore_create_info(VkSemaphoreCreateFlags flags) {
    VkSemaphoreCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags
    };
    return info;
}

VkImageCreateInfo VKInit::image_create_info(VkFormat format, VkImageUsageFlags usage_flags, VkExtent3D extent) {
    VkImageCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage_flags
    };
    return info;
}

VkImageViewCreateInfo VKInit::imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspect_flags) {
    VkImageViewCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange =  {
            .aspectMask = aspect_flags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    return info;
}

VkPipelineDepthStencilStateCreateInfo VKInit::depth_stencil_create_info(bool depth_test, bool depth_write, VkCompareOp compare_op) {
    VkPipelineDepthStencilStateCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .depthTestEnable = depth_test ? VK_TRUE : VK_FALSE,
        .depthWriteEnable = depth_write ? VK_TRUE : VK_FALSE,
        .depthCompareOp = depth_test ? compare_op : VK_COMPARE_OP_ALWAYS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };
    return info;
}