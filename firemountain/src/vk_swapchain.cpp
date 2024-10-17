#include <assert.h>
#include <VkBootstrap.h>

#include "vk_swapchain.hpp"
#include "vk_init.hpp"

void fmvk::Swapchain::Create(VkExtent2D window_extent, VkSurfaceKHR surface) {
	assert(this->_physical_device);
	assert(this->_device);
	assert(this->_instance);

    vkb::SwapchainBuilder swapchain_builder{ this->_physical_device, this->_device, surface };
    this->image_format = VK_FORMAT_B8G8R8A8_UNORM;
    auto result = swapchain_builder
        .set_desired_format(VkSurfaceFormatKHR { 
            .format = this->image_format,
            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        })
        .set_old_swapchain(this->swapchain)  // Old swapchain must be destroyed if one exists
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(window_extent.width, window_extent.height)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build();
    vkb::Swapchain vkb_swapchain = result.value();
    this->swapchain = vkb_swapchain.swapchain;
    this->extent = vkb_swapchain.extent;
    this->images = vkb_swapchain.get_images().value();
    this->image_views = vkb_swapchain.get_image_views().value();
}

void fmvk::Swapchain::Destroy(VkDevice device)
{
    vkDestroySwapchainKHR(device, this->swapchain, nullptr);
    for (int i = 0; i < this->image_views.size(); i++) {
        vkDestroyImageView(device, this->image_views[i], nullptr);
    }
}

void fmvk::Swapchain::SetContext(VkInstance instance, VkDevice device, VkPhysicalDevice gpu)
{
    this->_instance = instance;
    this->_device = device;
    this->_physical_device = gpu;
}
