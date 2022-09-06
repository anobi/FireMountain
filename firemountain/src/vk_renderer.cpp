#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <VkBootstrap.h>

#include <vk_renderer.hpp>
#include <vk_init.hpp>


#define VK_CHECK(x)                                                     \
    do {                                                                \
		VkResult err = x;                                               \
		if (err) {                                                      \
			std::cout <<"Detected Vulkan error: " << err << std::endl;  \
			abort();                                                    \
		}                                                               \
	} while (0)

using namespace fmVK;

int Vulkan::Init(const uint8_t width, const uint8_t height, void* window) {

    // Initialized Vulkan instance and debug messenger
    vkb::InstanceBuilder builder;
    auto build = builder.set_app_name("FireMountain")
        .request_validation_layers(true)
        .require_api_version(1, 1, 0)
        .use_default_debug_messenger()
        .build();
    vkb::Instance vkb_instance = build.value();
    this->_instance = vkb_instance.instance;
    this->_debug_messenger = vkb_instance.debug_messenger;

    // Create surface but how? I don't want to include SDL or it's members
    // in this renderer project.
    SDL_Vulkan_CreateSurface((SDL_Window*) window, this->_instance, &this->_surface);

    // Initialize device and physical device
    vkb::PhysicalDeviceSelector selector { vkb_instance };
    vkb::PhysicalDevice device = selector
        .set_minimum_version(1, 1)
        .set_surface(this->_surface)
        .select()
        .value();
    vkb::DeviceBuilder device_builder{ device };
    vkb::Device vkb_device = device_builder.build().value();
    this->_device = vkb_device.device;
    this->_gpu = device.physical_device;

    this->_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    this->_graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    this->init_swapchain();
    this->init_commands();

    this->_is_initialized = true;
    return 0;
}

void Vulkan::Frame() {

}

void Vulkan::Destroy() {
    if(this->_is_initialized) {
        vkDestroyCommandPool(this->_device, this->_command_pool, nullptr);
        vkDestroySwapchainKHR(this->_device, this->_swapchain, nullptr);
        for (auto i = 0; i < this->_swapchain_image_views.size(); i++) {
            vkDestroyImageView(this->_device, this->_swapchain_image_views[i], nullptr);
        }
        vkDestroyDevice(this->_device, nullptr);
        vkDestroySurfaceKHR(this->_instance, this->_surface, nullptr);
        vkb::destroy_debug_utils_messenger(this->_instance, this->_debug_messenger);
        vkDestroyInstance(this->_instance, nullptr);
    }
}

void Vulkan::init_swapchain() {
    vkb::SwapchainBuilder swapchain_builder{ this->_gpu, this->_device, this->_surface };
    vkb::Swapchain vkb_swapchain = swapchain_builder
        .use_default_format_selection()
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .build()
        .value();
    this->_swapchain = vkb_swapchain.swapchain;
    this->_swapchain_images = vkb_swapchain.get_images().value();
    this->_swapchain_image_views = vkb_swapchain.get_image_views().value();
    this->_swapchain_image_format = vkb_swapchain.image_format;
}

void Vulkan::init_commands() {
    // Init command pool
    VkCommandPoolCreateInfo command_pool_info = VKInit::command_pool_create_info(
        this->_graphics_queue_family,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    );
    VK_CHECK(vkCreateCommandPool(
        this->_device, 
        &command_pool_info, 
        nullptr, 
        &this->_command_pool
    ));

    // Init command buffer
    VkCommandBufferAllocateInfo cmd_buffer_alloc_info = VKInit::command_buffer_allocate_info(
        this->_command_pool
    );
    VK_CHECK(vkAllocateCommandBuffers(
        this->_device,
        &cmd_buffer_alloc_info,
        &this->_command_buffer
    ));
}