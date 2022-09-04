cmake_minimum_required(VERSION 3.13)
project(FirePlateau)
set(CMAKE_CXX_STANDARD 20)


if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 src/main.cpp)
else()
    add_executable(${PROJECT_NAME} src/main.cpp)
endif()

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${CMAKE_BINARY_DIR}/firemountain/include/)

target_sources(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/display.cpp
)
target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
)

target_link_libraries(${PROJECT_NAME} 
  PUBLIC 
    ${SDL2_LIBRARIES}
    LibFireMountain
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build
)
