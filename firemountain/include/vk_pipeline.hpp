#pragma once

#include <vector>
#include "vk_types.hpp"


namespace fmVK {

    bool load_shader_module(const char *file_path, const  VkDevice device, VkShaderModule *out);

    struct ComputePipeline {
        const char* name;
        VkPipeline pipeline;
        VkPipelineLayout layout;
        ComputePushConstants data;

        int Init(const VkDevice device, const char* shader_name, VkDescriptorSetLayout descriptor_layout);
        void Cleanup(const VkDevice device);
    };

    class Pipeline {
    public:
        Pipeline() {}
        ~Pipeline() {}
        int Init(const VkDevice device, const VkExtent2D window_extent, const char* shader_name, VkDescriptorSetLayout layout, AllocatedImage alloc_image);
        void Cleanup(const VkDevice device);

        VkPipeline pipeline;
        VkPipelineLayout layout;

        VkShaderModule fragment_shader;
        VkShaderModule vertex_shader;
        VkShaderModule compute_shader;
    };
}