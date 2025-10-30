cmake_minimum_required(VERSION 3.5)
project(FirePlateau)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Python3 COMPONENTS Development REQUIRED)
add_library(pyembed STATIC src/python_embed.cpp)
target_include_directories(pyembed PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
target_link_libraries(pyembed PRIVATE Python3::Python)


if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 src/main.cpp)
else()
    add_executable(${PROJECT_NAME} src/main.cpp)
endif()

# find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)
# include_directories(${CMAKE_BINARY_DIR}/firemountain/include/)

target_sources(${PROJECT_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_scene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/display.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/camera.cpp
)
target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
    ${CMAKE_BINARY_DIR}/firemountain/include/
)
target_link_libraries(${PROJECT_NAME}
  PRIVATE
    SDL3::SDL3
    glm
    LibFireMountain
    pyembed
    sqlite3
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_BINARY_DIR}/assets/ $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_BINARY_DIR}/shaders/shaderbuilder.py $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/bin/
)
