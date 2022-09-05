#pragma once

#include <vector>

#include "vk_types.hpp"


namespace fmVK {
    class Vulkan {
    public:
        Vulkan() {};
        ~Vulkan() {};

        int Init(const uint8_t width, const uint8_t height, void* window);
        void Frame();
        void Destroy();

    private:
        bool _initialized = false;

        VkExtent2D _window_extent;
        VkInstance _instance;
        VkPhysicalDevice _gpu;
        VkDevice _device;
        VkSurfaceKHR _surface;
        VkDebugUtilsMessengerEXT _debug_messenger;

        VkSwapchainKHR _swapchain;
        VkFormat _swapchain_image_format;
        std::vector<VkImage> _swapchain_images;
        std::vector<VkImageView> _swapchain_image_views;

        void init_swapchain();
    };
}