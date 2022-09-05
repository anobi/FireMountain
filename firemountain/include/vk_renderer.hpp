#pragma once

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
        VkExtent2D _window_extent;
        VkInstance _instance;
        VkPhysicalDevice _gpu;
        VkDevice _device;
        VkSurfaceKHR _surface;
        VkDebugUtilsMessengerEXT _debug_messenger;
    };
}