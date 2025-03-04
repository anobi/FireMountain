cmake_minimum_required(VERSION 3.13)
project(FirePlateau)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 src/main.cpp)
else()
    add_executable(${PROJECT_NAME} src/main.cpp)
endif()

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)

include_directories(${CMAKE_BINARY_DIR}/firemountain/include/)

target_sources(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/display.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/camera.cpp
)
target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    SDL3::SDL3
    glm
    LibFireMountain
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_BINARY_DIR}/assets/ $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/
)