#include "vk_renderer.hpp"

#include "VkBootstrap.h"


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

    return 0;
}

void Vulkan::Frame() {

}

void Vulkan::Destroy() {
    
}