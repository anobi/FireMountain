cmake_minimum_required(VERSION 3.13)
project(FireMountain)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 23)


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(FetchContent)

FetchContent_Declare(
    vk-bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap.git
    GIT_TAG        v1.4.307
)
FetchContent_Declare(
    VulkanMemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG        v3.2.1
    SYSTEM
)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.2.8
)
FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG        f48bd0bfb9b00ed1f5b13dbcdbd7909ca8bd49b5  # v2.0.0rc10
)
FetchContent_Declare(
    fastgltf
    GIT_REPOSITORY https://github.com/spnda/fastgltf.git
    GIT_TAG        v0.8.0
)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        11.0.2
)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.8
)
set(SLANG_OVERRIDE_IMGUI_PATH ${CMAKE_CURRENT_SOURCE_DIR}/_deps/imgui-src)
FetchContent_Declare(
        slang
        GIT_REPOSITORY https://github.com/shader-slang/slang.git
        GIT_TAG        v2025.11
)
FetchContent_MakeAvailable(
    SDL3
    vk-bootstrap
    VulkanMemoryAllocator
    tinyobjloader
    fastgltf
    glm
    fmt
    imgui
    slang
)

find_package(Vulkan REQUIRED)

# ---------------------------------------------------------------
# Build imgui as library
# ---------------------------------------------------------------
add_library(imgui_lib STATIC
  "_deps/imgui-src/imgui.cpp"
  "_deps/imgui-src/imgui_draw.cpp"
  "_deps/imgui-src/imgui_demo.cpp"
  "_deps/imgui-src/imgui_tables.cpp"
  "_deps/imgui-src/imgui_widgets.cpp"
  "_deps/imgui-src/backends/imgui_impl_sdl3.cpp"
  "_deps/imgui-src/backends/imgui_impl_vulkan.cpp"
)
set(IMGUI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/_deps/imgui-src)
set(IMGUI_BACKEND_DIR ${CMAKE_CURRENT_SOURCE_DIR}/_deps/imgui-src/backends)

target_link_libraries(imgui_lib
    Vulkan::Vulkan
    SDL3::SDL3
)

add_subdirectory("firemountain")
add_subdirectory("shaders")
add_subdirectory("platform")

target_include_directories(imgui_lib
  PUBLIC
    ${IMGUI_INCLUDE_DIR}
    ${IMGUI_INCLUDE_DIR}/backends
)