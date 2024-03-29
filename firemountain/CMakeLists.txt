cmake_minimum_required(VERSION 3.13)
project(LibFireMountain)
set(CMAKE_CXX_STANDARD 23)

add_library(${PROJECT_NAME} STATIC)

# ---------------------------------------------------------------
# External dependencies
# ---------------------------------------------------------------
find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

target_link_libraries(${PROJECT_NAME}
  Vulkan::Vulkan
  vk-bootstrap::vk-bootstrap
  GPUOpen::VulkanMemoryAllocator
  tinyobjloader
  fastgltf
  glm
  fmt
  imgui
)

include_directories(
  ${SDL2_INCLUDE_DIRS}
  ${IMGUI_INCLUDE_DIR}
)

# ---------------------------------------------------------------
# Internal sources, inclues and targets
# ---------------------------------------------------------------
target_sources(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/firemountain.cpp
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fm_scene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fm_mesh_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vk_mesh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vk_images.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vk_init.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vk_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vk_pipeline_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vk_pipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vk_descriptors.cpp
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(${PROJECT_NAME} 
  PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/lib
)