#pragma once

#include <vector>

#include "vk_types.hpp"


namespace fmvk {
    class PipelineBuilder {
    public:
        PipelineBuilder() { clear(); }
        VkViewport viewport;
        VkRect2D scissor;

        VkPipeline build_pipeline(VkDevice device);
        void clear();

        void set_shaders(VkShaderModule vertex_shader, VkShaderModule fragment_shader);
        void set_input_topology(VkPrimitiveTopology topology);
        void set_polygon_mode(VkPolygonMode mode);
        void set_cull_mode(VkCullModeFlags cull_mode, VkFrontFace front_face);
        void set_multisampling_none();
        void set_color_attachment_format(VkFormat format);
        void set_depth_format(VkFormat format);

        void disable_blending();

        void enable_depth_test(bool depth_write_enable, VkCompareOp op);
        void disable_depth_test();

        void enable_blending_additive();
        void enable_blending_alphablend();

        std::vector<VkPipelineShaderStageCreateInfo> _shader_stages;
        VkPipelineLayout _pipeline_layout;
        VkPipelineInputAssemblyStateCreateInfo _input_assembly;
        VkPipelineRasterizationStateCreateInfo _rasterizer;
        VkPipelineColorBlendAttachmentState _color_blend_attachment;
        VkPipelineVertexInputStateCreateInfo _vertex_input_info;
        VkPipelineMultisampleStateCreateInfo _multisampling;
        VkPipelineDepthStencilStateCreateInfo _depth_stencil;
        VkPipelineRenderingCreateInfo _render_info;
        VkFormat _color_attachment_format;
    };
}