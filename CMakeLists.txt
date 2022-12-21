cmake_minimum_required(VERSION 3.13)
project(FireMountain)
set(CMAKE_CXX_STANDARD 20)

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