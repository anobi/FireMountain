#include <iostream>
#include <fstream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <VkBootstrap.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <glm/gtx/transform.hpp>

#include <vk_renderer.hpp>
#include <vk_init.hpp>


int fmVK::Vulkan::Init(const uint32_t width, const uint32_t height, SDL_Window* window) {

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

    VmaAllocatorCreateInfo allocator_info = {
        .physicalDevice = this->_gpu,
        .device = this->_device,
        .instance = this->_instance
    };
    vmaCreateAllocator(&allocator_info, &this->_allocator);

    this->init_swapchain();
    this->init_commands();
    this->init_default_renderpass();
    this->init_framebuffers();
    this->init_sync_structures();
    this->init_pipelines();

    this->_is_initialized = true;

    return 0;
}

void fmVK::Vulkan::Draw(RenderObject* render_objects, int render_object_count) {
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

    VkClearValue clear_value = {
        .color = {{0.01f, 0.01f, 0.01f, 1.0f}}
    };
    VkClearValue depth_clear = {
        .depthStencil = {
            .depth = 1.0f
        }
    };
    VkClearValue clear_values[2] = {
        clear_value,
        depth_clear
    };

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
        .clearValueCount = 2,
        .pClearValues = &clear_values[0]
    };
    
    vkCmdBeginRenderPass(this->_command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    VkDeviceSize offset = 0;

    // Update and push constants
    glm::vec3 camera_position = {0.0f, 0.0f, -5.0f};
    glm::mat4 view = glm::translate(glm::mat4(1.0f), camera_position);
    glm::mat4 projection = glm::perspective(
        glm::radians(70.0f), 
        (float) this->_window_extent.width / (float) this->_window_extent.height,
        0.1f,
        200.0f
    );
    projection[1][1] *= -1;

    Mesh* bound_mesh = nullptr;
    Material* bound_material = nullptr;
    for (int i = 0; i < render_object_count; i++) {
        RenderObject& object = render_objects[i];
        if (object.material != bound_material) {
            vkCmdBindPipeline(
                this->_command_buffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                object.material->pipeline
            );
            bound_material = object.material;
        }

        glm::mat4 model = object.transform;
        glm::mat4 mvp = projection * view * model;
        MeshPushConstants constants = { .render_matrix = mvp};

        vkCmdPushConstants(
            this->_command_buffer, 
            object.material->pipeline_layout, 
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(MeshPushConstants),
            &constants
        );

        if (object.mesh != bound_mesh) {
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(
                this->_command_buffer, 
                0, 
                1, 
                &object.mesh->_vertex_buffer.buffer,
                &offset
            );
            bound_mesh = object.mesh;
        }
        
        vkCmdDraw(this->_command_buffer, object.mesh->vertices.size(), 1, 0, 0);
    }
    
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

void fmVK::Vulkan::Destroy() {
    if(this->_is_initialized) {
        vkWaitForFences(this->_device, 1, &this->_render_fence, true, 1000000000);
        this->_deletion_queue.flush();

        vmaDestroyAllocator(this->_allocator);
        vkDestroyDevice(this->_device, nullptr);
        vkDestroySurfaceKHR(this->_instance, this->_surface, nullptr);
        vkb::destroy_debug_utils_messenger(this->_instance, this->_debug_messenger);
        vkDestroyInstance(this->_instance, nullptr);
    }
}

void fmVK::Vulkan::UploadMesh(Mesh &mesh) {
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = mesh.vertices.size() * sizeof(Vertex),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    };

    VmaAllocationCreateInfo vma_alloc_info = {
        .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
    };

    VK_CHECK(vmaCreateBuffer(
        this->_allocator, 
        &buffer_info, 
        &vma_alloc_info,
        &mesh._vertex_buffer.buffer,
        &mesh._vertex_buffer.allocation,
        nullptr
    ));

    this->_deletion_queue.push_function([=, this]() {
        vmaDestroyBuffer(this->_allocator, mesh._vertex_buffer.buffer, mesh._vertex_buffer.allocation);
    });

    void* data;
    vmaMapMemory(this->_allocator, mesh._vertex_buffer.allocation, &data);
    memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
    vmaUnmapMemory(this->_allocator, mesh._vertex_buffer.allocation);
}


// =======================================================================================================
// Private methods
// =======================================================================================================

void fmVK::Vulkan::init_swapchain() {
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


    VkExtent3D depth_image_extent = {this->_window_extent.width, this->_window_extent.height, 1};
    this->_depth_format = VK_FORMAT_D32_SFLOAT;
    
    VkImageCreateInfo depth_image_info = VKInit::image_create_info(
        this->_depth_format,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        depth_image_extent
    );
    VmaAllocationCreateInfo depth_image_allocation_info = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };
    vmaCreateImage(
        this->_allocator, 
        &depth_image_info, 
        &depth_image_allocation_info, 
        &this->_depth_image.image,
        &this->_depth_image.allocation,
        nullptr
    );
    VkImageViewCreateInfo depth_view_info = VKInit::imageview_create_info(
        this->_depth_format,
        this->_depth_image.image,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );

    VK_CHECK(vkCreateImageView(this->_device, &depth_view_info, nullptr, &this->_depth_image_view));


    this->_deletion_queue.push_function([=, this]() {
        vkDestroyImageView(this->_device, this->_depth_image_view, nullptr);
        vmaDestroyImage(this->_allocator, this->_depth_image.image, this->_depth_image.allocation);
        vkDestroySwapchainKHR(this->_device, this->_swapchain, nullptr);
    });
}

void fmVK::Vulkan::init_commands() {
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

    this->_deletion_queue.push_function([=, this]() {
        vkDestroyCommandPool(this->_device, this->_command_pool, nullptr);
    });
}

void fmVK::Vulkan::init_default_renderpass() {
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

    VkAttachmentDescription depth_attachment = {
        .flags = 0,
        .format = this->_depth_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    VkAttachmentReference depth_attachment_ref = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };



    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
        .pDepthStencilAttachment = &depth_attachment_ref
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };
    VkSubpassDependency depth_dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    };

    VkSubpassDependency dependencies[2] = {
        dependency,
        depth_dependency
    };
    VkAttachmentDescription attachments[2] = {
        color_attachment,
        depth_attachment
    };
    VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 2,
        .pAttachments = &attachments[0],
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 2,
        .pDependencies = &dependencies[0]
    };

    VK_CHECK(vkCreateRenderPass(
        this->_device, 
        &render_pass_info, 
        nullptr, 
        &this->_render_pass
    ));

    this->_deletion_queue.push_function([=, this]() {
        vkDestroyRenderPass(this->_device, this->_render_pass, nullptr);
    });
}

void fmVK::Vulkan::init_framebuffers() {
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
        VkImageView attachments[2];
        attachments[0] = this->_swapchain_image_views[i];
        attachments[1] = this->_depth_image_view;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.attachmentCount = 2;

        VK_CHECK(vkCreateFramebuffer(
            this->_device, 
            &framebuffer_info,
            nullptr,
            &this->_framebuffers[i]
        ));

        this->_deletion_queue.push_function([=, this]() {
            vkDestroyFramebuffer(this->_device, this->_framebuffers[i], nullptr);
            vkDestroyImageView(this->_device, this->_swapchain_image_views[i], nullptr);
        });
    }
}

void fmVK::Vulkan::init_sync_structures() {
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

    this->_deletion_queue.push_function([=, this]() {
        vkDestroySemaphore(this->_device, this->_render_semaphore, nullptr);
        vkDestroySemaphore(this->_device, this->_present_semaphore, nullptr);
        vkDestroyFence(this->_device, this->_render_fence, nullptr);
    });
}

void fmVK::Vulkan::init_pipelines() {
    fmVK::Pipeline mesh_pipeline;
    mesh_pipeline.Init(this->_device, this->_window_extent, this->_render_pass, "mesh");

    this->pipelines["mesh"] = mesh_pipeline;
    this->_deletion_queue.push_function([=, this]() { this->pipelines["mesh"].Cleanup(); });
}
