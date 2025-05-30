#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include "vk_mem_alloc.h"

#include "vk_types.hpp"
#include "vk_image.hpp"
#include "vk_mesh.hpp"
#include "vk_pipeline.hpp"
#include "vk_swapchain.hpp"
#include "vk_descriptors.hpp"

#include "fm_utils.hpp"
#include "fm_renderable.hpp"
#include "vk_texture_cache.hpp"

struct SDL_Window;
union SDL_Event;

struct LoadedGLTF;


struct MeshNode : public Node {
    std::shared_ptr<MeshAsset> mesh;
    
    void Draw(const glm::mat4& top_matrix, DrawContext& ctx) override;
};

struct EngineStats {
    float frametime;
    int triangle_count;
    int drawcall_count;
    float scene_update_time;
    float mesh_draw_time;
};


namespace fmvk {
    class Vulkan;

    struct GLTFMetallic_Roughness {
        MaterialPipeline opaque_pipeline;
        MaterialPipeline transparent_pipeline;
        VkDescriptorSetLayout material_layout;

        struct MaterialConstants {
            glm::vec4 color_factors;
            glm::vec2 metal_roughness_factors;
            float has_metal_roughness_map;
            float has_color_map;

            glm::vec4 emissive_factor;
            float use_alpha_blending;
            float has_emissive_map;
            float has_normal_map;

            int color_tex_id;
            int metal_roughness_tex_id;
            int normal_tex_id;
            int emissive_tex_id;

            float padding_1;
            glm::vec4 extra[11];  // Padding for uniform buffers
        };
        static_assert(sizeof(MaterialConstants) == 256);  // Make sure the size is right

        // TODO: These need to be cleaned on exit
        struct MaterialResources {
            fmvk::Image::AllocatedImage color_image;
            VkSampler color_sampler;    

            fmvk::Image::AllocatedImage metal_roughness_image;
            VkSampler metal_roughness_sampler;

            fmvk::Image::AllocatedImage normal_image;
            VkSampler normal_sampler;

            fmvk::Image::AllocatedImage emissive_image;
            VkSampler emissive_sampler;

            VkBuffer data_buffer;
            uint32_t data_buffer_offset;
        };

        DescriptorWriter writer;

        void build_pipelines(const fmvk::Vulkan* renderer);
        void clear_resources(VkDevice device);

        MaterialInstance write_material(
            VkDevice device, 
            MaterialPass pass, 
            const MaterialResources& resources,
            DescriptorAllocatorGrowable& descriptor_allocator
        );
    };

    struct FrameData {
        VkCommandPool _command_pool;
        VkCommandBuffer _main_command_buffer;

        VkSemaphore _swapchain_semaphore;
        VkFence _render_fence;

        DescriptorAllocatorGrowable _frame_descriptors;
        DeletionQueue _deletion_queue;
    };

    constexpr int FRAME_OVERLAP = 2;

    class Vulkan {
    public:
        Vulkan() = default;
        ~Vulkan() = default;

        int Init(uint32_t width, uint32_t height, SDL_Window* window);
        void Draw(RenderObject* first_render_object, int render_object_count);
        void Resize(uint32_t width, uint32_t height);
        void Destroy();
        void ProcessImGuiEvent(const SDL_Event* e);
        
        GPUMeshBuffers UploadMesh(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);

        void CreateMaterial();
        void CreatePipeline(const char* shader_name);
        
        VkPipeline GetPipeline(const char* name) { return this->pipelines[name].pipeline; }
        VkPipelineLayout GetPipelineLayout(const char* name) { return this->pipelines[name].layout; }

        std::unordered_map<std::string, fmvk::Pipeline> pipelines;
        std::unordered_map<std::string, fmvk::ComputePipeline> compute_pipelines;

        unsigned int next_id = 0;
        MeshID AddMesh(const std::string& name, const std::shared_ptr<LoadedGLTF>& mesh);
        std::unordered_map<unsigned int, std::shared_ptr<LoadedGLTF>> loaded_meshes;

        LightID AddLight(const std::string& name);
        std::vector<unsigned int> lights;
        
        VkDevice _device{};
        VmaAllocator _allocator{};
        VkDescriptorSetLayout _gpu_scene_data_descriptor_layout{};

        MaterialInstance default_data{};
        fmvk::Image::AllocatedImage _texture_missing_error_image{};
        fmvk::Image::AllocatedImage _default_texture_white{};
        fmvk::Image::AllocatedImage _default_texture_black{};
        fmvk::Image::AllocatedImage _default_texture_grey{};

        VkSampler _default_sampler_linear{};
        VkSampler _default_sampler_nearest{};

        GLTFMetallic_Roughness metal_roughness_material;

        // New stuff, where these go?
        // fmvk::Image::AllocatedImage create_image(void *data, VkDevice device, VmaAllocator allocator, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
        fmvk::Image::AllocatedImage create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);

        EngineStats stats {};

        // These should be private, but the current gltf pipeline build prevents it
        fmvk::Image::AllocatedImage _draw_image {};
        fmvk::Image::AllocatedImage _depth_image {};
        void init_render_targets();

    private:
        int _frame_number = 0;
        bool _is_initialized = false;
        bool _resize_requested = false;
        VkClearValue _clear_value = {
            .color = {{0.02f, 0.02f, 0.02f, 1.0f}}
        };

        SDL_Window* _window {};
        VkExtent2D _window_extent {};
        VkExtent2D _requested_extent {};
        VkInstance _instance {};
        VkPhysicalDevice _gpu {};
        // VkDevice _device;

        VkSurfaceKHR _surface {};
        VkDebugUtilsMessengerEXT _debug_messenger {};
        DeletionQueue _deletion_queue;

        void init_vulkan(SDL_Window *window);

        fmvk::Swapchain _swapchain;
        void init_swapchain();

        VkQueue _graphics_queue {};
        uint32_t _graphics_queue_family {};

        VkCommandPool _command_pool {};
        VkCommandBuffer _command_buffer {};
        void init_commands();

        FrameData _frames[FRAME_OVERLAP];
        FrameData& get_current_frame() { return this->_frames[this->_frame_number % FRAME_OVERLAP]; }
        void init_sync_structures();

        // Immediate submit structures
        VkFence _immediate_fence {};
        VkCommandBuffer _immediate_command_buffer {};
        VkCommandPool _immediate_command_pool {};
        void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function) const;

        // Pipelines
        void init_pipelines();

        // Draw resources
        // AllocatedImage _draw_image;
        // AllocatedImage _depth_image;
        VkExtent2D _draw_extent {};
        float _render_scale = 1.0f;

        // TODO: move these to FM
        // ----------------------
    public:
        DrawContext _main_draw_context;
        void update_scene(const fmCamera* camera, std::vector<RenderSceneObj> scene);
    private:
        // ----------------------
        // End of TODO
 
        void init_imgui();
        void draw_imgui(VkCommandBuffer cmd, VkImageView image_view) const;
        void draw_background(VkCommandBuffer cmd);
        void draw_geometry(VkCommandBuffer cmd, RenderObject* render_objects, uint32_t render_object_count);



        // Descriptor sets
        DescriptorAllocatorGrowable global_descriptor_allocator;
        VkDescriptorSet _draw_image_descriptors{};
        VkDescriptorSetLayout _draw_image_descriptor_layout{};
        void init_descriptors();

        GPUSceneData scene_data;

        // Lock the previous camera in place to be able to fly around without updating frustum etc
        bool ghost_mode = false;
        glm::mat4 ghost_view = glm::mat4 { 0.0f };
        glm::mat4 ghost_projection = glm::mat4 { 0.0f };
        glm::vec3 ghost_camera_position = glm::vec3 { 0.0f };

        // Images & Textures
    public:
        fmvk::TextureCache texture_cache;
    private:
        void init_default_textures();
        void init_default_data();
    };
}


