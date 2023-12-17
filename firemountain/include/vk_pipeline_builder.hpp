#pragma once

#include <vector>

#include "vk_types.hpp"


namespace fmVK {
    class PipelineBuilder {
    public:
        VkViewport viewport;
        VkRect2D scissor;
        VkPipelineLayout pipeline_layout;
        VkPipelineInputAssemblyStateCreateInfo input_assembly;
        VkPipelineRasterizationStateCreateInfo rasterizer;
        VkPipelineColorBlendAttachmentState color_blend_attachment;
        VkPipelineVertexInputStateCreateInfo vertex_input_info;
        VkPipelineMultisampleStateCreateInfo multisampling;
        VkPipelineDepthStencilStateCreateInfo depth_stencil;
        std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
        
        VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);
    };
}