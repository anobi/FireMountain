#pragma once

#include <vector>
#include "vk_types.hpp"
#include "vk_image.hpp"

namespace fmvk {
    struct Swapchain {
        VkSwapchainKHR swapchain;

        VkExtent2D extent;
        VkFormat image_format;

        std::vector<VkImage> images;
        std::vector<VkImageView> image_views;
        std::vector<VkSemaphore> image_semaphores;

        void Create(VkExtent2D window_extent, VkSurfaceKHR surface);
        void Destroy(VkDevice device);
        void SetContext(VkInstance instance, VkDevice device, VkPhysicalDevice gpu);

    private:
        VkInstance _instance;
        VkDevice _device;
        VkPhysicalDevice _physical_device;
    };
};