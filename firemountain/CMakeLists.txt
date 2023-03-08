cmake_minimum_required(VERSION 3.13)
project(LibFireMountain)
set(CMAKE_CXX_STANDARD 20)

add_library(${PROJECT_NAME} STATIC)

# ---------------------------------------------------------------
# External dependencies
# ---------------------------------------------------------------
find_package(Vulkan REQUIRED)

add_subdirectory(external/vk-bootstrap)
add_subdirectory(external/VulkanMemoryAllocator)
add_subdirectory(external/tinyobjloader)

target_link_libraries(${PROJECT_NAME} 
  Vulkan::Vulkan
  vk-bootstrap::vk-bootstrap
  VulkanMemoryAllocator
  tinyobjloader
)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# ---------------------------------------------------------------
# Internal sources, inclues and targets
# ---------------------------------------------------------------
target_sources(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/firemountain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vk_init.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vk_renderer.cpp
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vk_mesh.cpp
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