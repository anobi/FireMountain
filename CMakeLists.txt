cmake_minimum_required(VERSION 3.13)
project(FireMountain)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 23)

add_subdirectory("firemountain")
add_subdirectory("platform")

include(FetchContent)

FetchContent_Declare(
  vk-bootstrap
  GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap.git
  GIT_TAG        v1.3.274
)
FetchContent_Declare(
  VulkanMemoryAllocator
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG        5e43c795daf43dd09398d8307212e85025215052  # Master
  SYSTEM
)
FetchContent_Declare(
  tinyobjloader
  GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
  GIT_TAG        f48bd0bfb9b00ed1f5b13dbcdbd7909ca8bd49b5  # v2.0.0rc10
)
FetchContent_Declare(
  fastgltf
  GIT_REPOSITORY https://github.com/spnda/fastgltf.git
  GIT_TAG        73fa4209b883f4614fda2f516a53ebc1c53aec70
)
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        1.0.1
)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        10.2.1
)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.90.1
)
FetchContent_MakeAvailable(
  vk-bootstrap
  VulkanMemoryAllocator
  tinyobjloader
  fastgltf
  glm
  fmt
  imgui
)

# ---------------------------------------------------------------
# Build imgui as library
# ---------------------------------------------------------------
find_package(SDL2 REQUIRED)
add_library(imgui STATIC
  "_deps/imgui-src/imgui.cpp"
  "_deps/imgui-src/imgui_draw.cpp"
  "_deps/imgui-src/imgui_demo.cpp"
  "_deps/imgui-src/imgui_tables.cpp"
  "_deps/imgui-src/imgui_widgets.cpp"
  "_deps/imgui-src/backends/imgui_impl_sdl2.cpp"
  "_deps/imgui-src/backends/imgui_impl_vulkan.cpp"
)
set(IMGUI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/_deps/imgui-src)
set(IMGUI_BACKEND_DIR ${CMAKE_CURRENT_SOURCE_DIR}/_deps/imgui-src/backends)

target_include_directories(imgui
  PUBLIC
    ${SDL2_INCLUDE_DIRS}
    ${IMGUI_INCLUDE_DIR}
    ${IMGUI_INCLUDE_DIR}/backends
)


# ---------------------------------------------------------------
# Compile shaders
# ---------------------------------------------------------------
find_package(Vulkan)
find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
file(GLOB SHADER_SOURCE_FILES
    ${SHADER_DIR}/*.frag
    ${SHADER_DIR}/*.vert
    ${SHADER_DIR}/*.comp
)


foreach(SHADER IN LISTS SHADER_SOURCE_FILES)
  message(STATUS "BUILDING SHADER")
  get_filename_component(FILE_NAME ${SHADER} NAME)
  message(STATUS ${SHADER})
  add_custom_command(OUTPUT ${SHADER_DIR}/spv/${FILE_NAME}.spv
    COMMAND ${GLSL_VALIDATOR} -V ${SHADER} -o ${SHADER_DIR}/spv/${FILE_NAME}.spv
    DEPENDS ${SHADER}
    COMMENT "Compiling ${FILE_NAME}")
  list(APPEND SPV_SHADERS ${SHADER_DIR}/spv/${FILE_NAME}.spv)
endforeach(SHADER)

add_custom_target(Shaders ALL DEPENDS ${SPV_SHADERS})