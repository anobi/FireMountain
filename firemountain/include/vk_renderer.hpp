#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "vk_mem_alloc.h"

#include "vk_mesh.hpp"
#include "vk_types.hpp"
#include "vk_pipeline.hpp"

#include "fm_utils.hpp"
#include "fm_renderable.hpp"


namespace fmVK {

    struct FrameData {
        VkCommandPool _command_pool;
        VkCommandBuffer _main_command_buffer;
        VkSemaphore _render_semaphore;
        VkSemaphore _swapchain_semaphore;
        VkFence _render_fence;
        DeletionQueue _deletion_queue;
    };

    constexpr unsigned int FRAME_OVERLAP = 2;

    class Vulkan {
    public:
        Vulkan() {};
        ~Vulkan() {};

        int Init(const uint32_t width, const uint32_t height, SDL_Window* window);
        void Draw(RenderObject* first_render_object, int render_object_count);
        void Destroy();
        void UploadMesh(Mesh &mesh);

        void CreateMaterial();
        void CreatePipeline(const char* shader_name);

        VkPipeline GetPipeline(const char* name) { return this->pipelines[name].pipeline; }
        VkPipelineLayout GetPipelineLayout(const char* name) { return this->pipelines[name].layout; }

        std::unordered_map<std::string, fmVK::Pipeline> pipelines;

    private:
        int _frame_number = 0;
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
        void init_vulkan(SDL_Window *window);

        VkSwapchainKHR _swapchain;
        VkExtent2D _swapchain_extent;
        VkFormat _swapchain_image_format;
        std::vector<VkImage> _swapchain_images;
        std::vector<VkImageView> _swapchain_image_views;
        void init_swapchain();
        void create_swapchain();
        void destroy_swapchain();

        VkQueue _graphics_queue;
        uint32_t _graphics_queue_family;
        VkCommandPool _command_pool;
        VkCommandBuffer _command_buffer;
        void init_commands();

        FrameData _frames[FRAME_OVERLAP];
        FrameData& get_current_frame() { return this->_frames[this->_frame_number % FRAME_OVERLAP]; }
        void init_sync_structures();

        // Pipelines
        void init_pipelines();

        // Draw resources
        AllocatedImage _draw_image;
        VkExtent2D _draw_extent;

        // Depth buffer resources
        VkFormat _depth_format;
        VkImageView _depth_image_view;
        AllocatedImage _depth_image;

        void draw_background(VkCommandBuffer cmd);


        // New stuff, where these go?
        AllocatedBuffer create_buffer(size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage);
        void destroy_buffer(const AllocatedBuffer &buffer);
    };
}