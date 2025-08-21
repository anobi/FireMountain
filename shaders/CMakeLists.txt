cmake_minimum_required(VERSION 3.5)
project(Shaders)
set(CMAKE_CXX_STANDARD 23)

# ---------------------------------------------------------------
# Compile shaders
# ---------------------------------------------------------------
find_package(Vulkan)
find_program(SLANGC slangc HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/ $ENV{HOME}/.local/bin)

set(SLANG_SHADERS_AND_ENTRY_POINTS
  src/bg_gradient.slang csMain compute
  src/mesh.slang vsMain vertex
  src/mesh.slang psMain fragment
  src/basic.slang vsMain vertex
  src/basic.slang psMain fragment
)

set(SHADER_DIR ${CMAKE_BINARY_DIR}/build/shaders)

list(LENGTH SLANG_SHADERS_AND_ENTRY_POINTS COUNT)
math(EXPR GROUPS "${COUNT} / 3 - 1")
foreach(GROUP RANGE 0 ${GROUPS})
  math(EXPR FILE_INDEX "${GROUP} * 3")
  math(EXPR ENTRY_INDEX "${FILE_INDEX} + 1")
  math(EXPR STAGE_INDEX "${FILE_INDEX} + 2")

  list(GET SLANG_SHADERS_AND_ENTRY_POINTS ${FILE_INDEX} SHADER_FILE)
  list(GET SLANG_SHADERS_AND_ENTRY_POINTS ${ENTRY_INDEX} ENTRY_POINT)
  list(GET SLANG_SHADERS_AND_ENTRY_POINTS ${STAGE_INDEX} STAGE)

  get_filename_component(FILE_NAME ${SHADER_FILE} NAME_WE)
  file(MAKE_DIRECTORY ${SHADER_DIR})

  set(OUTPUT_FILE ${SHADER_DIR}/${FILE_NAME}_${STAGE}.spv)

  add_custom_command(
          OUTPUT ${OUTPUT_FILE}
          COMMAND ${SLANGC} ${SHADER_FILE}
          -entry ${ENTRY_POINT}
          -stage ${STAGE}
          -target spirv
          -o ${OUTPUT_FILE}
          DEPENDS ${SHADER_FILE}
          COMMENT "Compiling ${FILE_NAME} ${STAGE} (${ENTRY_POINT})"
  )
  list(APPEND SPV_SHADERS ${OUTPUT_FILE})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SPV_SHADERS})