#pragma once

#include <vector>

#include "vk_types.hpp"


namespace fmVK {
    class Vulkan {
    public:
        Vulkan() {};
        ~Vulkan() {};

        int Init(const uint32_t width, const uint32_t height, SDL_Window* window);
        void Draw();
        void Destroy();

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

        void init_pipelines();
        
        bool load_shader_module(const char* file_path, VkShaderModule* out);
    };
}