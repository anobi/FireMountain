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

int Vulkan::Init(const uint32_t width, const uint32_t height, SDL_Window* window) {

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
    this->_window_extent = {
        .width = width,
        .height = height
    };

    // Create surface but how? I don't want to include SDL or it's members
    // in this renderer project.
    SDL_Vulkan_CreateSurface(window, this->_instance, &this->_surface);

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
    this->init_default_renderpass();
    this->init_framebuffers();
    this->init_sync_structures();
    this->_is_initialized = true;

    return 0;
}

void Vulkan::Draw() {
    VK_CHECK(vkWaitForFences(this->_device, 1, &this->_render_fence, true, 1000000000));
    VK_CHECK(vkResetFences(this->_device, 1, &this->_render_fence));

    // Request image from the swapchain
    uint32_t swapchain_image_index;
    VK_CHECK(vkAcquireNextImageKHR(
        this->_device, 
        this->_swapchain, 
        1000000000, 
        this->_present_semaphore, 
        nullptr, 
        &swapchain_image_index
    ));

    VK_CHECK(vkResetCommandBuffer(this->_command_buffer, 0));
    VkCommandBufferBeginInfo command_buffer_begin = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };
    VK_CHECK(vkBeginCommandBuffer(this->_command_buffer, &command_buffer_begin));

    VkRenderPassBeginInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = this->_render_pass,
        .framebuffer = this->_framebuffers[swapchain_image_index],
        .renderArea = {
            .offset = {
                .x = 0,
                .y = 0
            },
            .extent = this->_window_extent
        },
        .clearValueCount = 1,
        .pClearValues = &this->_clear_value
    };
    vkCmdBeginRenderPass(this->_command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(this->_command_buffer);
    VK_CHECK(vkEndCommandBuffer(this->_command_buffer));

    // Submit the framebuffer to the GPU
    VkPipelineStageFlags wait_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &this->_present_semaphore,
        .pWaitDstStageMask = &wait_stage_flags,
        .commandBufferCount = 1,
        .pCommandBuffers = &this->_command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &this->_render_semaphore
    };
    VK_CHECK(vkQueueSubmit(this->_queue, 1, &submit_info, this->_render_fence));

    // Present the image to the screen
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &this->_render_semaphore,
        .swapchainCount = 1,
        .pSwapchains = &this->_swapchain,
        .pImageIndices = &swapchain_image_index
    };
    VK_CHECK(vkQueuePresentKHR(this->_queue, &present_info));
    this->_frame += 1;
}

void Vulkan::Destroy() {
    if(this->_is_initialized) {
        vkDestroyCommandPool(this->_device, this->_command_pool, nullptr);
        vkDestroySwapchainKHR(this->_device, this->_swapchain, nullptr);
        vkDestroyRenderPass(this->_device, this->_render_pass, nullptr);
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
        .set_desired_extent(this->_window_extent.width, this->_window_extent.height)
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

void Vulkan::init_default_renderpass() {
    VkAttachmentDescription color_attachment = {
        .format = this->_swapchain_image_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref
    };

    VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass
    };

    VK_CHECK(vkCreateRenderPass(
        this->_device, 
        &render_pass_info, 
        nullptr, 
        &this->_render_pass
    ));
}

void Vulkan::init_framebuffers() {
    VkFramebufferCreateInfo framebuffer_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = nullptr,
        .renderPass = this->_render_pass,
        .attachmentCount = 1,
        .width = this->_window_extent.width,
        .height = this->_window_extent.height,
        .layers = 1
    };

    auto swapchain_imagecount = this->_swapchain_images.size();
    this->_framebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);
    for (auto i = 0; i < swapchain_imagecount; i++) {
        framebuffer_info.pAttachments = &this->_swapchain_image_views[i];
        VK_CHECK(vkCreateFramebuffer(
            this->_device, 
            &framebuffer_info, 
            nullptr, 
            &this->_framebuffers[i]
        ));
    }
}

void Vulkan::init_sync_structures() {
    VkFenceCreateInfo fence_create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    VK_CHECK(vkCreateFence(this->_device, &fence_create_info, nullptr, &this->_render_fence));

    VkSemaphoreCreateInfo semaphore_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0
    };
    VK_CHECK(vkCreateSemaphore(this->_device, &semaphore_create_info, nullptr, &this->_present_semaphore));
    VK_CHECK(vkCreateSemaphore(this->_device, &semaphore_create_info, nullptr, &this->_render_semaphore));
}