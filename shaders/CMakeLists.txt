cmake_minimum_required(VERSION 3.13)
project(Shaders)
set(CMAKE_CXX_STANDARD 23)

# ---------------------------------------------------------------
# Compile shaders
# ---------------------------------------------------------------
find_package(Vulkan)
find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)
find_program(SLANGC slangc HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/ $ENV{HOME}/.local/bin)

set(SHADERS
    src/mesh.frag
    src/mesh.vert
)

set(SLANG_SHADERS
  src/bg_gradient.slang
)

set(SHADER_DIR ${CMAKE_BINARY_DIR}/build/shaders)

foreach(SHADER IN LISTS SHADERS)
  message(STATUS "BUILDING SHADER")
  get_filename_component(FILE_NAME ${SHADER} NAME)
  message(STATUS ${SHADER})
  add_custom_command(OUTPUT ${SHADER_DIR}/${FILE_NAME}.spv
    COMMAND ${GLSL_VALIDATOR} -V ${SHADER} -o ${SHADER_DIR}/${FILE_NAME}.spv
    DEPENDS ${SHADER}
    COMMENT "Compiling ${FILE_NAME}")
  list(APPEND SPV_SHADERS ${SHADER_DIR}/${FILE_NAME}.spv)
endforeach(SHADER)

foreach(SHADER IN LISTS SLANG_SHADERS)
  message(STATUS "BUILDING SLANG SHADER")
  get_filename_component(FILE_NAME ${SHADER} NAME_WE)
  message(STATUS ${SHADER})
  add_custom_command(OUTPUT ${SHADER_DIR}/${FILE_NAME}.spv
          COMMAND ${SLANGC} ${SHADER} -target spirv -o ${SHADER_DIR}/${FILE_NAME}.spv
          DEPENDS ${SHADER}
          COMMENT "Compiling ${FILE_NAME}")
  list(APPEND SPV_SHADERS ${SHADER_DIR}/${FILE_NAME}.spv)
endforeach(SHADER)

add_custom_target(Shaders ALL DEPENDS ${SPV_SHADERS})