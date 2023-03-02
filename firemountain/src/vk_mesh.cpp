#include <vk_mesh.hpp>


VertexInputDescription Vertex::get_vertex_description() {
    VertexInputDescription description;

    VkVertexInputBindingDescription main_binding = {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    description.bindings.push_back(main_binding);

    VkVertexInputAttributeDescription attrib_position = {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, position)
    };
    description.attributes.push_back(attrib_position);
    
    VkVertexInputAttributeDescription attrib_normal = {
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, normal)
    };
    description.attributes.push_back(attrib_normal);

    VkVertexInputAttributeDescription attrib_color = {
        .location = 2,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, color)
    };
    description.attributes.push_back(attrib_color);

    return description;
}