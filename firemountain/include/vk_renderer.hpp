#pragma once

#include <vector>
#include <vk_mem_alloc.h>

#include "vk_mesh.hpp"
#include "fm_utils.hpp"
#include "vk_types.hpp"


namespace fmVK {
    class Vulkan {
    public:
        Vulkan() {};
        ~Vulkan() {};

        int Init(const uint32_t width, const uint32_t height, SDL_Window* window);
        void Draw();
        void Destroy();
        void UploadMesh(Mesh &mesh);

        // Testing
        // TODO: oh yea the renderer has it's own copies of the meshes
        // Mesh _triangle_mesh;
        // Mesh _monke_mesh;

    private:
        int _frame = 0;
        bool _is_initialized = false;
        VkClearValue _clear_value = {
            .color = {{0.02f, 0.02f, 0.02f, 1.0f}}
        };

        VkExtent2D _window_extent;
        VkInstance _instance;
        VkPhysicalDevice _gpu;
        VkDevice _device;
        VkSurfaceKHR _surface;
        VkDebugUtilsMessengerEXT _debug_messenger;
        DeletionQueue _deletion_queue;
        VmaAllocator _allocator;
        void init_vulkan();

        VkSwapchainKHR _swapchain;
        VkFormat _swapchain_image_format;
        std::vector<VkImage> _swapchain_images;
        std::vector<VkImageView> _swapchain_image_views;
        void init_swapchain();

        VkQueue _queue;
        uint32_t _graphics_queue_family;
        VkCommandPool _command_pool;
        VkCommandBuffer _command_buffer;
        void init_commands();

        VkRenderPass _render_pass;
        std::vector<VkFramebuffer> _framebuffers;
        void init_default_renderpass();
        void init_framebuffers();

        VkSemaphore _present_semaphore;
        VkSemaphore _render_semaphore;
        VkFence _render_fence;
        void init_sync_structures();

        // Pipelines
        VkPipelineLayout _pipeline_layout;
        VkPipelineLayout _mesh_pipeline_layout;
        VkPipeline _mesh_pipeline;
        void init_pipelines();
        bool load_shader_module(const char* file_path, VkShaderModule* out);

        // Depth buffer
        VkFormat _depth_format;
        VkImageView _depth_image_view;
        AllocatedImage _depth_image;
    };

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