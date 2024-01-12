#pragma once

#include <vector>
#include "vk_types.hpp"


namespace fmVK {
    class Pipeline {
    public:
        Pipeline() {}
        ~Pipeline() {}
        int Init(const VkDevice device, const VkExtent2D window_extent, const char* shader_name);
        void Cleanup(const VkDevice device);

        VkPipeline pipeline;
        VkPipelineLayout layout;

        VkShaderModule fragment_shader;
        VkShaderModule vertex_shader;
        VkShaderModule compute_shader;
    private:
        bool load_shader_module(const char *file_path, const  VkDevice device, VkShaderModule *out);
    };
}