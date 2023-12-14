cmake_minimum_required(VERSION 3.13)
project(FireMountain)
set(CMAKE_CXX_STANDARD 23)


include(FetchContent)

FetchContent_Declare(
  vk-bootstrap
  GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap.git
  GIT_TAG        99be60aa1901007d6e4112592dec73d2ebc75efe  # v1.3.269
)
FetchContent_Declare(
  VulkanMemoryAllocator
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG        5e43c795daf43dd09398d8307212e85025215052  # Master
)
FetchContent_Declare(
  tinyobjloader
  GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
  GIT_TAG        f48bd0bfb9b00ed1f5b13dbcdbd7909ca8bd49b5  # v2.0.0rc10
)
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        b85861aa0914241f6403697ea0b73cbb658afae4  # Master
)
FetchContent_MakeAvailable(
  vk-bootstrap
  VulkanMemoryAllocator
  tinyobjloader
  glm
)


add_subdirectory("firemountain")
add_subdirectory("platform")

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