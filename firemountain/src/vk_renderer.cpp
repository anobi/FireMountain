#include <fstream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <VkBootstrap.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <glm/gtx/transform.hpp>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

#include "vk_renderer.hpp"
#include "vk_init.hpp"
#include "vk_images.hpp"


int fmVK::Vulkan::Init(const uint32_t width, const uint32_t height, SDL_Window* window) {
    this->_window_extent = {
        .width = width,
        .height = height
    };
    this->_window = window;

    this->init_vulkan(this->_window);
    this->init_swapchain();
    this->init_commands();
    this->init_sync_structures();
    this->init_descriptors();
    this->init_pipelines();
    init_imgui();

    this->_is_initialized = true;

    return 0;
}

void fmVK::Vulkan::Draw(RenderObject* render_objects, int render_object_count) {


    VK_CHECK(vkWaitForFences(this->_device, 1, &get_current_frame()._render_fence, true, 1000000000));
    get_current_frame()._deletion_queue.flush();

    this->_draw_extent.width = this->_draw_image.extent.width;
    this->_draw_extent.height = this->_draw_image.extent.height;

    // Request image from the swapchain
    uint32_t swapchain_image_index;
    VK_CHECK(vkAcquireNextImageKHR(
        this->_device, 
        this->_swapchain, 
        1000000000, 
        this->get_current_frame()._swapchain_semaphore, 
        nullptr, 
        &swapchain_image_index
    ));

    // New draw
    VK_CHECK(vkResetFences(this->_device, 1, &get_current_frame()._render_fence));
    VK_CHECK(vkResetCommandBuffer(this->get_current_frame()._main_command_buffer, 0));

    auto cmd = this->get_current_frame()._main_command_buffer;
    auto command_buffer_begin = VKInit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    // Start drawing
    VK_CHECK(vkBeginCommandBuffer(cmd, &command_buffer_begin));

    VKUtil::transition_image(cmd, this->_draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    draw_background(cmd);

    // Draw meshes, transfer the draw image and the swapchain image to transfer layouts
    VKUtil::transition_image(cmd, this->_draw_image.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VKUtil::transition_image(cmd, this->_depth_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    draw_geometry(cmd, render_objects, render_object_count);

    VKUtil::transition_image(cmd, this->_draw_image.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    VKUtil::transition_image(cmd, this->_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy draw image into the swapchain
    VKUtil::copy_image_to_image(cmd, this->_draw_image.image, _swapchain_images[swapchain_image_index], this->_draw_extent, this->_swapchain_extent);

    // Draw Imgui
    VKUtil::transition_image(cmd, this->_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    draw_imgui(cmd, this->_swapchain_image_views[swapchain_image_index]);

    // Set swapchain image layout to PRESENT
    VKUtil::transition_image(cmd, this->_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // Finalize command buffer
    VK_CHECK(vkEndCommandBuffer(cmd));

    auto cmd_info = VKInit::command_buffer_submit_info(cmd);
    auto wait_info = VKInit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, this->get_current_frame()._swapchain_semaphore);
    auto signal_info = VKInit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, this->get_current_frame()._render_semaphore);

    VkSubmitInfo2 submit_info = VKInit::submit_info(&cmd_info, &signal_info, &wait_info);

    VK_CHECK(vkQueueSubmit2(this->_graphics_queue, 1, &submit_info, this->get_current_frame()._render_fence));

    // Present the image to the screen
    // TODO move this to vkinit utils
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &this->get_current_frame()._render_semaphore,
        .swapchainCount = 1,
        .pSwapchains = &this->_swapchain,
        .pImageIndices = &swapchain_image_index
    };
    VK_CHECK(vkQueuePresentKHR(this->_graphics_queue, &present_info));
    this->_frame_number += 1;
}

void fmVK::Vulkan::Destroy() {
    if(this->_is_initialized) {
        vkDeviceWaitIdle(this->_device);
        vkWaitForFences(this->_device, 1, &get_current_frame()._render_fence, true, 1000000000);
        
        this->_deletion_queue.flush();

        destroy_swapchain(); 

        vkDestroySurfaceKHR(this->_instance, this->_surface, nullptr);
        vmaDestroyAllocator(this->_allocator);
        vkDestroyDevice(this->_device, nullptr);
        vkb::destroy_debug_utils_messenger(this->_instance, this->_debug_messenger);
        vkDestroyInstance(this->_instance, nullptr);
    }
}

void fmVK::Vulkan::ProcessImGuiEvent(SDL_Event* e)
{
    ImGui_ImplSDL2_ProcessEvent(e);
}

GPUMeshBuffers fmVK::Vulkan::UploadMesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices) {
    const size_t vertex_buffer_size = vertices.size() * sizeof(Vertex);
    const size_t index_buffer_size = indices.size() * sizeof(uint32_t);
    GPUMeshBuffers surface;

    VkBufferUsageFlags vertex_buffer_flags = 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
        | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    surface.vertex_buffer = create_buffer(vertex_buffer_size, vertex_buffer_flags, VMA_MEMORY_USAGE_GPU_ONLY);

    VkBufferDeviceAddressInfo device_address_info  {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = surface.vertex_buffer.buffer
    };
    surface.vertex_buffer_address = vkGetBufferDeviceAddress(this->_device, &device_address_info);

    VkBufferUsageFlags index_buffer_flags = 
         VK_BUFFER_USAGE_INDEX_BUFFER_BIT 
         | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    surface.index_buffer = create_buffer(index_buffer_size, index_buffer_flags, VMA_MEMORY_USAGE_GPU_ONLY);

    AllocatedBuffer staging = create_buffer(
        vertex_buffer_size + index_buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY
    );
    void* data = staging.allocation->GetMappedData();
    memcpy(data, vertices.data(), vertex_buffer_size);
    memcpy((char*)data + vertex_buffer_size, indices.data(), index_buffer_size);
    immediate_submit([&](VkCommandBuffer cmd) {
        VkBufferCopy vertex_copy {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = vertex_buffer_size
        };
        vkCmdCopyBuffer(cmd, staging.buffer, surface.vertex_buffer.buffer, 1, &vertex_copy);

        VkBufferCopy index_copy {
            .srcOffset = vertex_buffer_size,
            .dstOffset = 0,
            .size = index_buffer_size
        };
        vkCmdCopyBuffer(cmd, staging.buffer, surface.index_buffer.buffer, 1, &index_copy);
    });

    destroy_buffer(staging);
    this->_deletion_queue.push_function([=, this]() {
        destroy_buffer(surface.index_buffer);
        destroy_buffer(surface.vertex_buffer);
    });

    return surface;
}


// =======================================================================================================
// Private methods
// =======================================================================================================


void fmVK::Vulkan::init_vulkan(SDL_Window *window) {
    vkb::InstanceBuilder builder;
    auto build = builder.set_app_name("FireMountain")
        .request_validation_layers(true)
        .use_default_debug_messenger()
        .require_api_version(1, 3, 0)
        .build();
    vkb::Instance vkb_instance = build.value();

    this->_instance = vkb_instance.instance;
    this->_debug_messenger = vkb_instance.debug_messenger;


    // TODO: How to do this without including SDL headers in this project?
    SDL_Vulkan_CreateSurface(this->_window, this->_instance, &this->_surface);

    // Vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features_13 {
        .synchronization2 = true,
        .dynamicRendering = true
    };

    // Vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features features_12 = {
        .descriptorIndexing = true,
        .bufferDeviceAddress = true
    };

    // Initialize device and physical device
    vkb::PhysicalDeviceSelector selector { vkb_instance };
    vkb::PhysicalDevice device = selector
        .set_minimum_version(1, 1)
        .set_required_features_13(features_13)
        .set_required_features_12(features_12)
        .set_surface(this->_surface)
        .select()
        .value();
    vkb::DeviceBuilder device_builder{ device };
    vkb::Device vkb_device = device_builder.build().value();

    this->_device = vkb_device.device;
    this->_gpu = device.physical_device;
    this->_graphics_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    this->_graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    VmaAllocatorCreateInfo allocator_info = {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = this->_gpu,
        .device = this->_device,
        .instance = this->_instance
    };
    vmaCreateAllocator(&allocator_info, &this->_allocator);
}


void fmVK::Vulkan::init_imgui() {
    // 1: Create descriptor pool
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 1000,
        .poolSizeCount = (uint32_t)std::size(pool_sizes),
        .pPoolSizes = pool_sizes
    };

    VkDescriptorPool imgui_pool;
    VK_CHECK(vkCreateDescriptorPool(this->_device, &pool_info, nullptr, &imgui_pool));

    // 2: Init library
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForVulkan(this->_window);
    ImGui_ImplVulkan_InitInfo init_info = {
        .Instance = this->_instance,
        .PhysicalDevice = this->_gpu,
        .Device = this->_device,
        .Queue = this->_graphics_queue,
        .DescriptorPool = imgui_pool,
        .MinImageCount = 3,
        .ImageCount = 3,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .UseDynamicRendering = true,
        .ColorAttachmentFormat = this->_swapchain_image_format
    };
    ImGui_ImplVulkan_Init(&init_info, VK_NULL_HANDLE);

    immediate_submit([&](VkCommandBuffer cmd) { ImGui_ImplVulkan_CreateFontsTexture(); });

    // ImGui_ImplVulkan_DestroyFontUploadObjects();
    ImGui_ImplVulkan_DestroyFontsTexture();

    this->_deletion_queue.push_function([=, this]() {
        vkDestroyDescriptorPool(this->_device, imgui_pool, nullptr);
        ImGui_ImplVulkan_Shutdown();
    });
}

void fmVK::Vulkan::init_swapchain() {

    create_swapchain();

    VkExtent3D render_image_extent = {this->_window_extent.width, this->_window_extent.height, 1};
    this->_draw_image.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    this->_draw_image.extent = render_image_extent;

    VkImageUsageFlags draw_image_usage = 
          VK_IMAGE_USAGE_TRANSFER_SRC_BIT
        | VK_IMAGE_USAGE_TRANSFER_DST_BIT
        | VK_IMAGE_USAGE_STORAGE_BIT
        | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    VkImageCreateInfo draw_image_info = VKInit::image_create_info(
        this->_draw_image.format,
        draw_image_usage,
        render_image_extent
    );
    VmaAllocationCreateInfo draw_image_allocinfo = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };
    vmaCreateImage(this->_allocator, &draw_image_info, &draw_image_allocinfo,&this->_draw_image.image,&this->_draw_image.allocation,nullptr);
    
    
    VkImageViewCreateInfo draw_view_info = VKInit::imageview_create_info(this->_draw_image.format,this->_draw_image.image,VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(this->_device, &draw_view_info, nullptr, &this->_draw_image.view));

    //
    // Create depth buffer images and views
    //
    this->_depth_image.format = VK_FORMAT_D32_SFLOAT;
    
    VkImageCreateInfo depth_image_info = VKInit::image_create_info(
        this->_depth_image.format,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        render_image_extent
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
        this->_depth_image.format,
        this->_depth_image.image,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );
    VK_CHECK(vkCreateImageView(this->_device, &depth_view_info, nullptr, &this->_depth_image.view));

    this->_deletion_queue.push_function([=, this]() {
        vkDestroyImageView(this->_device, this->_draw_image.view, nullptr);
        vmaDestroyImage(this->_allocator, this->_draw_image.image, this->_draw_image.allocation);
        vkDestroyImageView(this->_device, this->_depth_image.view, nullptr);
        vmaDestroyImage(this->_allocator, this->_depth_image.image, this->_depth_image.allocation);
    });
}

void fmVK::Vulkan::create_swapchain() {
    vkb::SwapchainBuilder swapchain_builder{ this->_gpu, this->_device, this->_surface };
    this->_swapchain_image_format = VK_FORMAT_B8G8R8A8_UNORM;
    vkb::Swapchain vkb_swapchain = swapchain_builder
        .set_desired_format(VkSurfaceFormatKHR { 
            .format = this->_swapchain_image_format, 
            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        })
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(this->_window_extent.width, this->_window_extent.height)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();
    this->_swapchain = vkb_swapchain.swapchain;
    this->_swapchain_extent = vkb_swapchain.extent;
    this->_swapchain_images = vkb_swapchain.get_images().value();
    this->_swapchain_image_views = vkb_swapchain.get_image_views().value();
}

void fmVK::Vulkan::destroy_swapchain()
{
        vkDestroySwapchainKHR(this->_device, this->_swapchain, nullptr);
        for (int i = 0; i < this->_swapchain_image_views.size(); i++) {
            vkDestroyImageView(this->_device, this->_swapchain_image_views[i], nullptr);
        }
}

void fmVK::Vulkan::init_commands() {
    // Init command pool
    VkCommandPoolCreateInfo cmdp_info = VKInit::command_pool_create_info(
        this->_graphics_queue_family,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    );

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateCommandPool(this->_device, &cmdp_info, nullptr, &this->_frames[i]._command_pool));
        VkCommandBufferAllocateInfo alloc_info = VKInit::command_buffer_allocate_info(this->_frames[i]._command_pool, 1);
        VK_CHECK(vkAllocateCommandBuffers(this->_device, &alloc_info, &this->_frames[i]._main_command_buffer));
    
        this->_deletion_queue.push_function([=, this]() {
            vkDestroyCommandPool(this->_device, this->_frames[i]._command_pool, nullptr);
        });
    }

    // Init immediate command pool
    VK_CHECK(vkCreateCommandPool(this->_device, &cmdp_info, nullptr, &this->_immediate_command_pool));
    VkCommandBufferAllocateInfo immediata_alloc_info = VKInit::command_buffer_allocate_info(this->_immediate_command_pool, 1);
    VK_CHECK(vkAllocateCommandBuffers(this->_device, &immediata_alloc_info, &this->_immediate_command_buffer));

    this->_deletion_queue.push_function([=, this]() {
        vkDestroyCommandPool(this->_device, this->_immediate_command_pool, nullptr);
    });
}

void fmVK::Vulkan::init_sync_structures() {
    VkFenceCreateInfo fence_create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    

    VkSemaphoreCreateInfo semaphore_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0
    };

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateFence(this->_device, &fence_create_info, nullptr, &this->_frames[i]._render_fence));
        VK_CHECK(vkCreateSemaphore(this->_device, &semaphore_create_info, nullptr, &this->_frames[i]._swapchain_semaphore));
        VK_CHECK(vkCreateSemaphore(this->_device, &semaphore_create_info, nullptr, &this->_frames[i]._render_semaphore));
    }
    
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        this->_deletion_queue.push_function([=, this]() {
            vkDestroySemaphore(this->_device, this->_frames[i]._render_semaphore, nullptr);
            vkDestroySemaphore(this->_device, this->_frames[i]._swapchain_semaphore, nullptr);
            vkDestroyFence(this->_device, this->_frames[i]._render_fence, nullptr);
        });
    }

    VK_CHECK(vkCreateFence(this->_device, &fence_create_info, nullptr, &this->_immediate_fence));
    this->_deletion_queue.push_function([=, this]() { vkDestroyFence(this->_device, this->_immediate_fence, nullptr); });
}

void fmVK::Vulkan::immediate_submit(std::function<void(VkCommandBuffer cmd)> &&function) {
    VK_CHECK(vkResetFences(this->_device, 1, &_immediate_fence));
    VK_CHECK(vkResetCommandBuffer(this->_immediate_command_buffer, 0));

    VkCommandBuffer cmd = this->_immediate_command_buffer;
    VkCommandBufferBeginInfo cmd_begin_info = VKInit::command_buffer_begin_info(
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    );
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmd_info = VKInit::command_buffer_submit_info(cmd);
    VkSubmitInfo2 submit = VKInit::submit_info(&cmd_info, nullptr, nullptr);
    VK_CHECK(vkQueueSubmit2(this->_graphics_queue, 1, &submit, this->_immediate_fence));
    VK_CHECK(vkWaitForFences(this->_device, 1, &this->_immediate_fence, true, 9999999999));
}

void fmVK::Vulkan::init_pipelines() {
    fmVK::ComputePipeline background_pipeline;
    background_pipeline.Init(this->_device, "bg_gradient", this->_draw_image_descriptor_layout);
    this->compute_pipelines["background"] = background_pipeline;

    fmVK::Pipeline mesh_pipeline;
    mesh_pipeline.Init(this->_device, this->_window_extent, "mesh", this->_draw_image_descriptor_layout, this->_draw_image);
    this->pipelines["mesh"] = mesh_pipeline;

    this->_deletion_queue.push_function([=, this]() { 
        for (auto &p : this->pipelines) {
            p.second.Cleanup(this->_device);
        }

        for (auto &p : this->compute_pipelines) {
            p.second.Cleanup(this->_device);
        }
    });
}

void fmVK::Vulkan::draw_imgui(VkCommandBuffer cmd, VkImageView image_view)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame(this->_window);
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();
    ImGui::Render();

    VkRenderingAttachmentInfo color_attachment = VKInit::attachment_info(image_view, nullptr, VK_IMAGE_LAYOUT_GENERAL);
    VkRenderingInfo render_info = VKInit::rendering_info(this->_swapchain_extent, &color_attachment, nullptr);
    vkCmdBeginRendering(cmd, &render_info);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRendering(cmd);
}

void fmVK::Vulkan::draw_background(VkCommandBuffer cmd)
{
    auto bg_pipeline = this->compute_pipelines["background"];
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, bg_pipeline.pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, bg_pipeline.layout, 0, 1, &this->_draw_image_descriptors, 0, nullptr);
    
    ComputePushConstants pc = {
        .data_1 = glm::fvec4(0.10f, 0.10f, 0.10f, 1.0f),
        .data_2 = glm::fvec4(0.16f, 0.18f, 0.20f, 1.0f)
    };

    vkCmdPushConstants(cmd, bg_pipeline.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &pc);
    vkCmdDispatch(cmd, std::ceil(this->_draw_extent.width / 16.0), std::ceil(this->_draw_extent.height / 16.0), 1);
}

void fmVK::Vulkan::draw_geometry(VkCommandBuffer cmd, RenderObject* render_objects, uint32_t render_object_count) {
    VkRenderingAttachmentInfo color_attachment = VKInit::attachment_info(this->_draw_image.view, nullptr, VK_IMAGE_LAYOUT_GENERAL);
    VkRenderingAttachmentInfo depth_attachment = VKInit::depth_attachment_info(this->_depth_image.view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    VkRenderingInfo render_info = VKInit::rendering_info(this->_draw_extent, &color_attachment, &depth_attachment);
    vkCmdBeginRendering(cmd, &render_info);

    VkViewport viewport = {
        .x = 0,
        .y = 0,
        .width = (float) this->_draw_extent.width,
        .height = (float) this->_draw_extent.height,
        .minDepth = 0.01f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = { .x = 0, .y = 0},
        .extent = this->_draw_extent
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Update and push constants
    glm::vec3 camera_position = {0.0f, 0.0f, -5.0f};
    glm::mat4 view = glm::translate(camera_position);
    glm::mat4 projection = glm::perspective(
        glm::radians(70.0f), 
        (float) this->_window_extent.width / (float) this->_window_extent.height,
        0.1f,
        200.0f
    );
    projection[1][1] *= -1;

    GPUMeshBuffers* bound_mesh = nullptr;
    Material* bound_material = nullptr;
    for (int i = 0; i < render_object_count; i++) {
        RenderObject& object = render_objects[i];
        if (object.material != bound_material) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
            bound_material = object.material;
        }

        glm::mat4 model = object.transform;
        glm::mat4 mvp = projection * view * model;
        GPUDrawPushConstants constants = { 
            .world_matrix = mvp,
            .vertex_buffer = object.mesh->vertex_buffer_address
        };
        vkCmdPushConstants(cmd, object.material->pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT,0, sizeof(GPUDrawPushConstants), &constants);

        if (object.mesh != bound_mesh) {
            VkDeviceSize offset = 0;
            vkCmdBindIndexBuffer(cmd, object.mesh->index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
            bound_mesh = object.mesh;
        }
        vkCmdDrawIndexed(cmd, object.index_count, 1, object.first_index, 0, 0);
    }
    vkCmdEndRendering(cmd);
}

AllocatedBuffer fmVK::Vulkan::create_buffer(size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage) {
    VkBufferCreateInfo buffer_info = { 
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .size = alloc_size,
        .usage = usage
    };
    VmaAllocationCreateInfo vma_malloc_info = {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = memory_usage
    };

    AllocatedBuffer buffer;
    VK_CHECK(vmaCreateBuffer(this->_allocator, &buffer_info, &vma_malloc_info, &buffer.buffer, &buffer.allocation, &buffer.info));
    
    return buffer;
}

void fmVK::Vulkan::destroy_buffer(const AllocatedBuffer &buffer) {
    vmaDestroyBuffer(this->_allocator, buffer.buffer, buffer.allocation);
}


// =================
//  Descriptor sets
// =================

void fmVK::Vulkan::init_descriptors() {
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
    };

    this->global_descriptor_allocator.init_pool(this->_device, 10, sizes);

    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        this->_draw_image_descriptor_layout  = builder.build(this->_device, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    this->_draw_image_descriptors = this->global_descriptor_allocator.allocate(this->_device, this->_draw_image_descriptor_layout);

    VkDescriptorImageInfo image_info = {
        .imageView = this->_draw_image.view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };

    VkWriteDescriptorSet draw_image_write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = this ->_draw_image_descriptors,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo = &image_info
    };
    vkUpdateDescriptorSets(this->_device, 1, &draw_image_write, 0, nullptr);

    this->_deletion_queue.push_function([&]() {
        vkDestroyDescriptorSetLayout(this->_device, this->_draw_image_descriptor_layout, nullptr);
        this->global_descriptor_allocator.destroy_pool(this->_device);
    });
}
