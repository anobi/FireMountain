#include <fstream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <VkBootstrap.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

#include "vk_renderer.hpp"
#include "vk_init.hpp"
#include "vk_images.hpp"
#include "vk_pipeline_builder.hpp"

#include "fm_mesh_loader.hpp"


bool is_visible(const RenderObject& obj, const glm::mat4& view_projection) {
    std::array<glm::vec3, 8> corners {
        glm::vec3 { 1,  1,  1 },
        glm::vec3 { 1,  1, -1 },
        glm::vec3 { 1, -1,  1 },
        glm::vec3 { 1, -1, -1 },
        glm::vec3 {-1,  1,  1 },
        glm::vec3 {-1,  1, -1 },
        glm::vec3 {-1, -1,  1 },
        glm::vec3 {-1, -1, -1 },
    };
    glm::mat4 matrix = view_projection * obj.transform;
    glm::vec3 min = {  1.5f,  1.5f, 1.5f };
    glm::vec3 max = { -1.5f, -1.5, -1.5f };

    for (int c = 0; c < 8; c++) {
        // Project corners into clip space
        glm::vec4 v = matrix * glm::vec4(obj.bounds.origin + (corners[c] * obj.bounds.extents), 1.0f);

        // Perspective correction
        v.x = v.x / v.w;
        v.y = v.y / v.w;
        v.z = v.z / v.w;

        min = glm::min(glm::vec3 { v.x, v.y, v.z }, min);
        max = glm::max(glm::vec3 { v.x, v.y, v.z }, max);
    }

    if (min.z > 1.0f || max.z < 0.0f || min.x > 1.0f || max.x < -1.0f || min.y > 1.0f || max.y < -1.0f) {
        return false;
    } else {
        return true;
    }
}


int fmvk::Vulkan::Init(const uint32_t width, const uint32_t height, SDL_Window* window) {
    this->_window_extent = {
        .width = width,
        .height = height
    };
    this->_window = window;

    init_vulkan(this->_window);
    init_swapchain();
    init_render_targets();
    init_commands();
    init_sync_structures();
    init_descriptors();
    init_pipelines();
    init_default_textures();
    init_default_data();
    init_imgui();

    this->_is_initialized = true;

    return 0;
}

void fmvk::Vulkan::Draw(RenderObject* render_objects, int render_object_count, glm::mat4 view_projection_matrix) {
    auto start = std::chrono::system_clock::now();

    VK_CHECK(vkWaitForFences(this->_device, 1, &get_current_frame()._render_fence, true, 1000000000));
    get_current_frame()._deletion_queue.flush();
    get_current_frame()._frame_descriptors.clear_pools(this->_device);

    if (this->_resize_requested) {
        // Update extents
        this->_window_extent.width = this->_requested_extent.width;
        this->_window_extent.height = this->_requested_extent.height;
        
        // Recreate swapchain
        vkDeviceWaitIdle(this->_device);
        this->_swapchain.Destroy(this->_device);
        this->_swapchain.Create(this->_window_extent, this->_surface);

        // Re-create draw and depth targets with new extent
        destroy_image(this->_draw_image);
        destroy_image(this->_depth_image);
        init_render_targets();

        this->_resize_requested = false;
    }

    this->_draw_extent.width = std::min(this->_swapchain.extent.width, this->_draw_image.extent.width) * this->_render_scale;
    this->_draw_extent.height = std::min(this->_swapchain.extent.height, this->_draw_image.extent.height) * this->_render_scale;

    // update_scene(view_projection_matrix);

    // Request image from the swapchain
    uint32_t swapchain_image_index;
    VkResult e = vkAcquireNextImageKHR(
        this->_device,
        this->_swapchain.swapchain,
        1000000000, 
        get_current_frame()._swapchain_semaphore, 
        nullptr, 
        &swapchain_image_index
    );
    if (e == VK_ERROR_OUT_OF_DATE_KHR) {
        this->_resize_requested = true;
        return;
    }

    // New draw
    VK_CHECK(vkResetFences(this->_device, 1, &get_current_frame()._render_fence));
    VK_CHECK(vkResetCommandBuffer(get_current_frame()._main_command_buffer, 0));

    auto cmd = get_current_frame()._main_command_buffer;
    auto command_buffer_begin = VKInit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    // Start drawing
    VK_CHECK(vkBeginCommandBuffer(cmd, &command_buffer_begin));

    VKUtil::transition_image(cmd, this->_draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    draw_background(cmd);
    
    // Draw meshes, transfer the draw image and the swapchain image to transfer layouts
    VKUtil::transition_image(cmd, this->_draw_image.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VKUtil::transition_image(cmd, this->_depth_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    draw_geometry(cmd, render_objects, render_object_count);


    // Transition draw image and swapchain
    VKUtil::transition_image(cmd, this->_draw_image.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    VKUtil::transition_image(cmd, this->_swapchain.images[swapchain_image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy draw image into the swapchain
    VKUtil::copy_image_to_image(cmd, this->_draw_image.image, _swapchain.images[swapchain_image_index], this->_draw_extent, this->_swapchain.extent);

    // Draw Imgui
    VKUtil::transition_image(cmd, this->_swapchain.images[swapchain_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    draw_imgui(cmd, this->_swapchain.image_views[swapchain_image_index]);

    // Set swapchain image layout to PRESENT
    VKUtil::transition_image(cmd, this->_swapchain.images[swapchain_image_index], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // Finalize command buffer
    VK_CHECK(vkEndCommandBuffer(cmd));

    auto cmd_info = VKInit::command_buffer_submit_info(cmd);
    auto wait_info = VKInit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, get_current_frame()._swapchain_semaphore);
    auto signal_info = VKInit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame()._render_semaphore);

    VkSubmitInfo2 submit_info = VKInit::submit_info(&cmd_info, &signal_info, &wait_info);

    VK_CHECK(vkQueueSubmit2(this->_graphics_queue, 1, &submit_info, get_current_frame()._render_fence));

    // Present the image to the screen
    // TODO move this to vkinit utils
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &get_current_frame()._render_semaphore,
        .swapchainCount = 1,
        .pSwapchains = &this->_swapchain.swapchain,
        .pImageIndices = &swapchain_image_index
    };
    VkResult present_result = vkQueuePresentKHR(this->_graphics_queue, &present_info);
    if (present_result == VK_ERROR_OUT_OF_DATE_KHR) {
        this->_resize_requested = true;
    }

    this->_frame_number += 1;

    auto end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    stats.frametime = elapsed.count() / 1000.f;
}

void fmvk::Vulkan::Resize(const uint32_t width, const uint32_t height)
{
    this->_resize_requested = true;
    this->_requested_extent = {
        .width = width,
        .height = height
    };
}

void fmvk::Vulkan::Destroy() {
    if(this->_is_initialized) {
        vkDeviceWaitIdle(this->_device);
        this->loaded_meshes.clear();

        for (auto& frame : this->_frames) {
            frame._deletion_queue.flush();
        }

        this->_deletion_queue.flush();

        destroy_image(this->_draw_image);
        destroy_image(this->_depth_image);
        this->_swapchain.Destroy(this->_device);
        vkDestroySurfaceKHR(this->_instance, this->_surface, nullptr);
        vmaDestroyAllocator(this->_allocator);
        vkDestroyDevice(this->_device, nullptr);
        vkb::destroy_debug_utils_messenger(this->_instance, this->_debug_messenger);
        vkDestroyInstance(this->_instance, nullptr);
    }
}

void fmvk::Vulkan::ProcessImGuiEvent(SDL_Event* e)
{
    ImGui_ImplSDL2_ProcessEvent(e);
}

GPUMeshBuffers fmvk::Vulkan::UploadMesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices) {
    const size_t vertex_buffer_size = vertices.size() * sizeof(Vertex);
    const size_t index_buffer_size = indices.size() * sizeof(uint32_t);
    GPUMeshBuffers new_surface;

    VkBufferUsageFlags vertex_buffer_flags = 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
        | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    new_surface.vertex_buffer = fmvk::Buffer::create_buffer(
        vertex_buffer_size, vertex_buffer_flags, VMA_MEMORY_USAGE_GPU_ONLY, this->_allocator);

    VkBufferDeviceAddressInfo device_address_info  {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = new_surface.vertex_buffer.buffer
    };
    new_surface.vertex_buffer_address = vkGetBufferDeviceAddress(this->_device, &device_address_info);

    VkBufferUsageFlags index_buffer_flags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    new_surface.index_buffer = fmvk::Buffer::create_buffer(
        index_buffer_size, index_buffer_flags, VMA_MEMORY_USAGE_GPU_ONLY, this->_allocator);

    fmvk::Buffer::AllocatedBuffer staging = fmvk::Buffer::create_buffer(
        vertex_buffer_size + index_buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY,
        this->_allocator
    );
    void* data = staging.allocation->GetMappedData();
    memcpy(data, vertices.data(), vertex_buffer_size);
    memcpy((char*)data + vertex_buffer_size, indices.data(), index_buffer_size);
    immediate_submit([&](VkCommandBuffer cmd) {
        VkBufferCopy vertex_copy = { 0 };
        vertex_copy.srcOffset = 0;
        vertex_copy.dstOffset = 0;
        vertex_copy.size = vertex_buffer_size;
        vkCmdCopyBuffer(cmd, staging.buffer, new_surface.vertex_buffer.buffer, 1, &vertex_copy);

        VkBufferCopy index_copy = { 0 };
        index_copy.srcOffset = vertex_buffer_size;
        index_copy.dstOffset = 0;
        index_copy.size = index_buffer_size;
        vkCmdCopyBuffer(cmd, staging.buffer, new_surface.index_buffer.buffer, 1, &index_copy);
    });

    fmvk::Buffer::destroy_buffer(staging, this->_allocator);

    return new_surface;
}

MeshID fmvk::Vulkan::AddMesh(const std::string &name, std::shared_ptr<LoadedGLTF> mesh)
{
    auto id = this->next_id++;
    this->loaded_meshes.emplace(id, mesh);
    return {id};
}

// =======================================================================================================
// Private methods
// =======================================================================================================

void fmvk::Vulkan::init_vulkan(SDL_Window *window) {
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


void fmvk::Vulkan::init_imgui() {
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
        .ColorAttachmentFormat = this->_swapchain.image_format
    };
    ImGui_ImplVulkan_Init(&init_info, VK_NULL_HANDLE);

    immediate_submit([&](VkCommandBuffer cmd) { ImGui_ImplVulkan_CreateFontsTexture(); });

    ImGui_ImplVulkan_DestroyFontsTexture();
    this->_deletion_queue.push_function([=, this]() {
        ImGui_ImplVulkan_Shutdown();
        vkDestroyDescriptorPool(this->_device, imgui_pool, nullptr);
    });
}

void fmvk::Vulkan::init_swapchain() {
    this->_swapchain.SetContext(this->_instance, this->_device, this->_gpu);
    this->_swapchain.Create(this->_window_extent, this->_surface);
}

// Create render and depth buffer images
void fmvk::Vulkan::init_render_targets() {
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
    vmaCreateImage(this->_allocator, &draw_image_info, &draw_image_allocinfo,&this->_draw_image.image, &this->_draw_image.allocation, nullptr);
    
    
    VkImageViewCreateInfo draw_view_info = VKInit::imageview_create_info(this->_draw_image.format, this->_draw_image.image,VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(this->_device, &draw_view_info, nullptr, &this->_draw_image.view));

    //
    // Create depth buffer images and views
    //
    this->_depth_image.format = VK_FORMAT_D32_SFLOAT;
    VkImageUsageFlags depth_image_usages {};
    depth_image_usages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    
    VkImageCreateInfo depth_image_info = VKInit::image_create_info(
        this->_depth_image.format,
        depth_image_usages,
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

    // this->_deletion_queue.push_function([=, this]() {
    //     destroy_image(this->_draw_image);
    //     destroy_image(this->_depth_image);
    // });
}

void fmvk::Vulkan::init_commands() {
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

void fmvk::Vulkan::init_sync_structures() {
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
    this->_deletion_queue.push_function([=, this]() { 
        vkDestroyFence(this->_device, this->_immediate_fence, nullptr); 
    });
}

void fmvk::Vulkan::immediate_submit(std::function<void(VkCommandBuffer cmd)> &&function) {
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

void fmvk::Vulkan::init_pipelines() {
    fmvk::ComputePipeline background_pipeline;
    background_pipeline.Init(this->_device, "bg_gradient", this->_draw_image_descriptor_layout);
    this->compute_pipelines["background"] = background_pipeline;

    this->metal_roughness_material.build_pipelines(this);

    this->_deletion_queue.push_function([=, this]() {
        for (auto &p : this->pipelines) {
            p.second.Cleanup(this->_device);
        }

        for (auto &p : this->compute_pipelines) {
            p.second.Cleanup(this->_device);
        }
        this->metal_roughness_material.clear_resources(this->_device);
    });
}

// TODO: Move to Firemountain actual
void fmvk::Vulkan::update_scene(glm::mat4 view_projection_matrix, std::vector<RenderSceneObj> scene)
{
    auto start = std::chrono::system_clock::now();

    this->_main_draw_context.opaque_surfaces.clear();

    this->scene_data.camera_position = glm::vec3(0.0f);
    this->scene_data.viewprojection = view_projection_matrix;
    this->scene_data.ambient_color = glm::vec4(0.5f);
    this->scene_data.sunlight_color = glm::vec4(1.8f);
    this->scene_data.sunlight_direction = glm::vec4(0.0, 1.0, 0.5, 2.0);

    GPULightData point_light_1 = {
        .colorIntensity = { 0.8f, 0.2f, 0.2f, 1.0f },
        .attentuation = 0.4f,
        .position = { 0.0f, 5.0f, 0.0f },
        .type = LightType::Point
    };
    this->scene_data.lights[0] = point_light_1;
    this->scene_data.light_count = 1;

    for (auto o : scene) {
        auto mesh = this->loaded_meshes.at(o.mesh_id.id);
        mesh->Draw(o.transform, this->_main_draw_context);
    }

    // for (auto& s : this->loaded_meshes) {
    //     s.second->Draw(glm::mat4 { 1.0f }, this->_main_draw_context);
    // }

    auto end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    stats.scene_update_time = elapsed.count() / 1000.f;
}

void fmvk::Vulkan::draw_imgui(VkCommandBuffer cmd, VkImageView image_view)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame(this->_window);
    ImGui::NewFrame();

    // Setup stats window
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(300, 120));
    ImGui::Begin("Stats");
    ImGui::Text("Frametime %f ms", stats.frametime);
    ImGui::Text("Draw time %f ms", stats.mesh_draw_time);
    ImGui::Text("Update time %f ms", stats.scene_update_time);
    ImGui::Text("Triangles %i", stats.triangle_count);
    ImGui::Text("Draw calls %i", stats.drawcall_count);
    ImGui::End();

    // Setup camera info window
    ImGui::SetNextWindowPos(ImVec2(10, 130));
    ImGui::SetNextWindowSize(ImVec2(300, 85));
    ImGui::Begin("Camera");

    // Disabled until we figure out how to draw this from outside the library
    // ImGui::Text("Pitch: %.2f", this->_camera->pitch);
    // ImGui::Text("Yaw: %.2f", this->_camera->yaw);
    // ImGui::Text("Position x: %.2f y: %.2f z: %.2f",
    //     this->_camera->position.x, 
    //     this->_camera->position.y, 
    //     this->_camera->position.z
    // );

    ImGui::End();
    ImGui::Render();

    VkRenderingAttachmentInfo color_attachment = VKInit::attachment_info(image_view, nullptr, VK_IMAGE_LAYOUT_GENERAL);
    VkRenderingInfo render_info = VKInit::rendering_info(this->_swapchain.extent, &color_attachment, nullptr);
    vkCmdBeginRendering(cmd, &render_info);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRendering(cmd);
}

void fmvk::Vulkan::draw_background(VkCommandBuffer cmd)
{
    // TODO: I'm missing the ComputeEffect thing
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

void fmvk::Vulkan::draw_geometry(VkCommandBuffer cmd, RenderObject* render_objects, uint32_t render_object_count) {
    stats.drawcall_count = 0;
    stats.triangle_count = 0;
    auto start = std::chrono::system_clock::now();

    std::vector<uint32_t> opaque_draws;
    opaque_draws.reserve(this->_main_draw_context.opaque_surfaces.size());
    for (uint32_t i = 0; i < this->_main_draw_context.opaque_surfaces.size(); i++) {
        if (is_visible(this->_main_draw_context.opaque_surfaces[i], scene_data.viewprojection)) {
            opaque_draws.push_back(i);
        }
    }

    std::sort(opaque_draws.begin(), opaque_draws.end(), 
        [&](const auto& iA, const auto& iB) {
            const RenderObject& A = this->_main_draw_context.opaque_surfaces[iA];
            const RenderObject& B = this->_main_draw_context.opaque_surfaces[iB];
            if (A.material == B.material) {
                return A.index_buffer < B.index_buffer;
            } else {
                return A.material < B.material;
            }
    });

    VkRenderingAttachmentInfo color_attachment = VKInit::attachment_info(this->_draw_image.view, nullptr, VK_IMAGE_LAYOUT_GENERAL);
    VkRenderingAttachmentInfo depth_attachment = VKInit::depth_attachment_info(this->_depth_image.view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    VkRenderingInfo render_info = VKInit::rendering_info(this->_draw_extent, &color_attachment, &depth_attachment);
    vkCmdBeginRendering(cmd, &render_info);

    // Allocate uniform buffer for the scene data
    fmvk::Buffer::AllocatedBuffer gpu_scene_data_buffer = fmvk::Buffer::create_buffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, this->_allocator);
    get_current_frame()._deletion_queue.push_function([=, this]() {
        fmvk::Buffer::destroy_buffer(gpu_scene_data_buffer, this->_allocator);
    });

    // Write the buffer
    GPUSceneData* scene_uniform_data = (GPUSceneData*) gpu_scene_data_buffer.allocation->GetMappedData();
    *scene_uniform_data = this->scene_data;

    // Create a descriptor set that binds the buffer and update it
    VkDescriptorSet global_descriptor = get_current_frame()._frame_descriptors.allocate(
        this->_device, 
        this->_gpu_scene_data_descriptor_layout
    );
    DescriptorWriter writer;
    writer.write_buffer(0, gpu_scene_data_buffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.update_set(this->_device, global_descriptor);

    MaterialPipeline* last_pipeline = nullptr;
    MaterialInstance* last_material = nullptr;
    VkBuffer last_index_buffer = VK_NULL_HANDLE;
    auto draw = [&](const RenderObject& object) {
        if (object.material != last_material) {
            last_material = object.material;

            if (object.material->pipeline != last_pipeline) {
                last_pipeline = object.material->pipeline;

                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline->pipeline);
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline->layout, 0, 1, &global_descriptor, 0, nullptr);
                
                VkViewport viewport = {
                    .x = 0,
                    .y = 0,
                    .width = (float) this->_window_extent.width,
                    .height = (float) this->_window_extent.height,
                    .minDepth = 0.0f,
                    .maxDepth = 1.0f
                };
                vkCmdSetViewport(cmd, 0, 1, &viewport);

                VkRect2D scissor = {
                    .offset = { .x = 0, .y = 0},
                    .extent = { 
                        .width = this->_window_extent.width, 
                        .height = this->_window_extent.height 
                    }
                };
                vkCmdSetScissor(cmd, 0, 1, &scissor);
            }
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline->layout, 1, 1, &object.material->material_set, 0, nullptr);
        }

        if (object.index_buffer != last_index_buffer) {
            last_index_buffer = object.index_buffer;
            vkCmdBindIndexBuffer(cmd, object.index_buffer, 0, VK_INDEX_TYPE_UINT32);
        }
        
        GPUDrawPushConstants constants = {
            .world_matrix = object.transform,
            .vertex_buffer = object.vertex_buffer_address
        };
        vkCmdPushConstants(cmd, object.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &constants);
        vkCmdDrawIndexed(cmd, object.index_count, 1, object.first_index, 0, 0);

        stats.drawcall_count++;
        stats.triangle_count += object.index_count / 3;
    };

    for (auto& r : opaque_draws) {
        draw(this->_main_draw_context.opaque_surfaces[r]);
    }
    for (auto& r : this->_main_draw_context.transparent_surfaces) {
        draw(r);
    }

    vkCmdEndRendering(cmd);

    this->_main_draw_context.opaque_surfaces.clear();
    this->_main_draw_context.transparent_surfaces.clear();

    auto end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    stats.mesh_draw_time = elapsed.count() / 1000.0f;

}

// =================
//  Descriptor sets 
// ================= 

void fmvk::Vulkan::init_descriptors() {
    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
    };
    this->global_descriptor_allocator.init(this->_device, 10, sizes);
    this->_deletion_queue.push_function([&]() {
        this->global_descriptor_allocator.destroy_pools(this->_device);
    });

    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        this->_draw_image_descriptor_layout  = builder.build(this->_device, VK_SHADER_STAGE_COMPUTE_BIT);
    }
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        this->_gpu_scene_data_descriptor_layout = builder.build(this->_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT );
    }

    _deletion_queue.push_function([&]() {
        vkDestroyDescriptorSetLayout(_device, this->_draw_image_descriptor_layout , nullptr);
        vkDestroyDescriptorSetLayout(_device, this->_gpu_scene_data_descriptor_layout, nullptr);
    });

    this->_draw_image_descriptors = this->global_descriptor_allocator.allocate(this->_device, this->_draw_image_descriptor_layout);
    {
        DescriptorWriter writer;
        writer.write_image(0, this->_draw_image.view, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.update_set(this->_device, this->_draw_image_descriptors);
    }
    
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4}
        };

        this->_frames[i]._frame_descriptors = DescriptorAllocatorGrowable {};
        this->_frames[i]._frame_descriptors.init(this->_device, 1000, frame_sizes);
        this->_deletion_queue.push_function([&, i]() {
            this->_frames[i]._frame_descriptors.destroy_pools(this->_device); 
        });
    }
}

AllocatedImage fmvk::Vulkan::create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
    AllocatedImage new_image = {
        .extent = size,
        .format = format
    };
    VkImageCreateInfo image_info = VKInit::image_create_info(format, usage, size);
    if (mipmapped) {
        image_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
    }
    VmaAllocationCreateInfo alloc_info = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };
    VK_CHECK(vmaCreateImage(this->_allocator, &image_info, &alloc_info, &new_image.image, &new_image.allocation, nullptr));

    VkImageAspectFlags aspect_flag = VK_IMAGE_ASPECT_COLOR_BIT;
    if (format == VK_FORMAT_D32_SFLOAT) {
        aspect_flag = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    VkImageViewCreateInfo view_info = VKInit::imageview_create_info(format, new_image.image, aspect_flag);
    view_info.subresourceRange.levelCount = image_info.mipLevels;
    VK_CHECK(vkCreateImageView(this->_device, &view_info, nullptr, &new_image.view));

    return new_image;
}

AllocatedImage fmvk::Vulkan::create_image(void *data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
    size_t data_size = size.depth * size.width * size.height * 4;
    fmvk::Buffer::AllocatedBuffer upload_buffer = fmvk::Buffer::create_buffer(
        data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, this->_allocator);
    memcpy(upload_buffer.info.pMappedData, data, data_size);

    AllocatedImage new_image = create_image(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

    immediate_submit([&](VkCommandBuffer cmd) {
        VKUtil::transition_image(cmd, new_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        VkBufferImageCopy copy_region = {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .imageExtent = size
        };

        vkCmdCopyBufferToImage(
            cmd, 
            upload_buffer.buffer, 
            new_image.image, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            1, 
            &copy_region
        );

        if (mipmapped) {
            VkExtent2D mip_extent = VkExtent2D { new_image.extent.width, new_image.extent.height};
            VKUtil::generate_mipmaps(cmd, new_image.image, mip_extent);
        } else {
            VKUtil::transition_image(cmd, new_image.image, 
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );
        }
    });

    fmvk::Buffer::destroy_buffer(upload_buffer, this->_allocator);
    return new_image;
}

void fmvk::Vulkan::destroy_image(const AllocatedImage &image)
{
    vkDestroyImageView(this->_device, image.view, nullptr);
    vmaDestroyImage(this->_allocator, image.image, image.allocation);
}

void fmvk::Vulkan::init_default_textures()
{
    constexpr uint32_t white = std::byteswap(0xFFFFFFFF);
    this->_default_texture_white = create_image(
        (void*) &white,
        VkExtent3D(1, 1, 1), 
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT
    );

    constexpr uint32_t grey = std::byteswap(0x404040FF);
    this->_default_texture_grey = create_image(
        (void*) &grey,
        VkExtent3D(1, 1, 1), 
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT
    );

    constexpr uint32_t black = std::byteswap(0x000000FF);
    this->_default_texture_black = create_image(
        (void*) &black,
        VkExtent3D(1, 1, 1), 
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT
    );

    constexpr uint32_t magenta = std::byteswap(0xFF00FFFF);
    std::array<uint32_t, 16 * 16> pixels;
    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 16; y++) {
            pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
        }
    }
    this->_texture_missing_error_image = create_image(
        pixels.data(),
        VkExtent3D(16, 16, 1),
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT
    );

    VkSamplerCreateInfo nearest_sampler_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST
    };
    vkCreateSampler(this->_device, &nearest_sampler_info, nullptr, &this->_default_sampler_nearest);

    VkSamplerCreateInfo linear_sampler_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR
    };
    vkCreateSampler(this->_device, &linear_sampler_info, nullptr, &this->_default_sampler_linear);

    this->_deletion_queue.push_function([=, this]() {
        destroy_image(this->_default_texture_white);
        destroy_image(this->_default_texture_grey);
        destroy_image(this->_default_texture_black);
        destroy_image(this->_texture_missing_error_image);
        vkDestroySampler(this->_device, this->_default_sampler_nearest, nullptr);
        vkDestroySampler(this->_device, this->_default_sampler_linear, nullptr);
    });
}


// This should also be in Firemountain
void fmvk::Vulkan::init_default_data()
{
    GLTFMetallic_Roughness::MaterialResources material_resources = {
        .color_image = this->_texture_missing_error_image,
        .color_sampler = this->_default_sampler_linear,
        .metal_roughness_image = this->_default_texture_white,
        .metal_roughness_sampler = this->_default_sampler_linear
    };

    fmvk::Buffer::AllocatedBuffer material_constants = fmvk::Buffer::create_buffer(
        sizeof(GLTFMetallic_Roughness::MaterialConstants),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        this->_allocator
    );

    GLTFMetallic_Roughness::MaterialConstants* scene_uniform_data = (GLTFMetallic_Roughness::MaterialConstants*) material_constants.allocation->GetMappedData();
    scene_uniform_data->color_factors = glm::vec4 { 1.0f, 1.0f, 1.0f, 1.0f};
    scene_uniform_data->metal_roughness_factors = glm::vec4 { 1.0f, 0.5f, 0.0f, 0.0f };

    // TODO: This might cause problems
    this->_deletion_queue.push_function([=, this]() {
        fmvk::Buffer::destroy_buffer(material_constants, this->_allocator);
    });

    material_resources.data_buffer = material_constants.buffer;
    material_resources.data_buffer_offset = 0;

    this->default_data = metal_roughness_material.write_material(
        this->_device,
        MaterialPass::FM_MATERIAL_PASS_OPAQUE,
        material_resources,
        this->global_descriptor_allocator
    );
}

void MeshNode::Draw(const glm::mat4 &top_matrix, DrawContext &ctx)
{
    glm::mat4 node_matrix = top_matrix * this->world_transform;
    for (auto& s : this->mesh->surfaces) {
        RenderObject object = {
            .index_count = s.count,
            .first_index = s.start_index,
            .index_buffer = this->mesh->mesh_buffers.index_buffer.buffer,
            .material = &s.material->data,
            .bounds = s.bounds,
            .transform = node_matrix,
            .vertex_buffer_address = this->mesh->mesh_buffers.vertex_buffer_address
        };

        if (s.material->data.pass_type == MaterialPass::FM_MATERIAL_PASS_TRANSPARENT) {
            ctx.transparent_surfaces.push_back(object);
        } else {
            ctx.opaque_surfaces.push_back(object);
        }
    }
    Node::Draw(top_matrix, ctx);
}

void fmvk::GLTFMetallic_Roughness::build_pipelines(fmvk::Vulkan* renderer)
{
    // Shaders
    // -------------------------------------------------------------------------
    // TODO: Get shader paths from pipeline name. Use fmt::format
    VkShaderModule fragment_shader;
    if (!fmvk::load_shader_module("shaders/mesh.frag.spv", renderer->_device, &fragment_shader)) {
        fmt::println("Error building fragment shader module");
    }
    else {
        fmt::println("Fragment shader module loaded.");
    }

    VkShaderModule vertex_shader;
    if (!fmvk::load_shader_module("shaders/mesh.vert.spv", renderer->_device, &vertex_shader)) {
        fmt::println("Error building vertex shader module");
    } 
    else {
        fmt::println("Vertex shader module loaded.");
    }

    // Pipeline layout
    // -------------------------------------------------------------------------
    
    VkPushConstantRange push_constant_range = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(GPUDrawPushConstants)
    };

    DescriptorLayoutBuilder layout_builder;
    layout_builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    layout_builder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    layout_builder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    this->material_layout = layout_builder.build(
        renderer->_device,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
    );

    VkDescriptorSetLayout layouts[] = {
        renderer->_gpu_scene_data_descriptor_layout,
        this->material_layout
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info = VKInit::pipeline_layout_create_info();
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pSetLayouts = layouts;
    pipeline_layout_info.setLayoutCount = 2;

    VkPipelineLayout opaque_layout;
    VK_CHECK(vkCreatePipelineLayout(renderer->_device, &pipeline_layout_info,nullptr, &opaque_layout));
    this->opaque_pipeline.layout = opaque_layout;

    VkPipelineLayout transparent_layout;
    VK_CHECK(vkCreatePipelineLayout(renderer->_device, &pipeline_layout_info,nullptr, &transparent_layout));
    this->transparent_pipeline.layout = transparent_layout;

    // Pipeline builder
    // -------------------------------------------------------------------------
    fmvk::PipelineBuilder pipeline_builder;
    pipeline_builder.set_shaders(vertex_shader, fragment_shader);
    pipeline_builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipeline_builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    pipeline_builder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pipeline_builder.set_multisampling_none();
    pipeline_builder.disable_blending();
    pipeline_builder.enable_depth_test(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    // Set formats
    pipeline_builder.set_color_attachment_format(renderer->_draw_image.format);
    pipeline_builder.set_depth_format(renderer->_depth_image.format);

    // Build opaque pipeline
    pipeline_builder._pipeline_layout = opaque_layout;
    this->opaque_pipeline.pipeline = pipeline_builder.build_pipeline(renderer->_device);

    // Create and build transparent pipeline
    pipeline_builder.enable_blending_additive();
    pipeline_builder.enable_depth_test(false, VK_COMPARE_OP_GREATER_OR_EQUAL);
    pipeline_builder._pipeline_layout = transparent_layout;
    this->transparent_pipeline.pipeline = pipeline_builder.build_pipeline(renderer->_device);

    vkDestroyShaderModule(renderer->_device, fragment_shader, nullptr);
    vkDestroyShaderModule(renderer->_device, vertex_shader, nullptr);
}

void fmvk::GLTFMetallic_Roughness::clear_resources(VkDevice device)
{
    vkDestroyDescriptorSetLayout(device, this->material_layout, nullptr);
    vkDestroyPipelineLayout(device, this->opaque_pipeline.layout, nullptr);
    vkDestroyPipeline(device, this->opaque_pipeline.pipeline, nullptr);
    vkDestroyPipelineLayout(device, this->transparent_pipeline.layout, nullptr);
    vkDestroyPipeline(device, this->transparent_pipeline.pipeline, nullptr);
}

MaterialInstance fmvk::GLTFMetallic_Roughness::write_material(VkDevice device, MaterialPass pass, const MaterialResources &resources, DescriptorAllocatorGrowable &descriptor_allocators)
{
    MaterialInstance data;
    data.pass_type = pass;
    if (pass == MaterialPass::FM_MATERIAL_PASS_TRANSPARENT) {
        data.pipeline = &this->transparent_pipeline;
    } else {
        data.pipeline = &this->opaque_pipeline;
    }
    data.material_set = descriptor_allocators.allocate(device, this->material_layout);

    this->writer.clear();
    this->writer.write_buffer(
        0,
        resources.data_buffer,
        sizeof(MaterialConstants),
        resources.data_buffer_offset,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    );
    this->writer.write_image(
        1,
        resources.color_image.view,
        resources.color_sampler,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    );
    this->writer.write_image(
        2,
        resources.metal_roughness_image.view,
        resources.metal_roughness_sampler,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    );
    this->writer.update_set(device, data.material_set);

    return data;
}


