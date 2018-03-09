cmake_minimum_required(VERSION 3.10)
project(fishball C)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(BUILD_SHARED_LIBS ON)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
OPTION(GLFW_BUILD_DOCS OFF)
OPTION(GLFW_INSTALL OFF)

add_subdirectory("${PROJECT_SOURCE_DIR}/external/glfw" ${CMAKE_BINARY_DIR}/glfw/)

set(FISHBALL_SOURCES
    src/main.c
    src/array.h
    src/window.h
    src/window.c
    src/gfx.h
    src/gfx.c
    src/fbgl.h
    src/fbgl.c
    src/voxel.h
    src/voxel.c
)

add_executable(fishball ${FISHBALL_SOURCES})
target_include_directories(fishball PRIVATE "${PROJECT_SOURCE_DIR}/external/glfw/include")
target_link_libraries(fishball glfw)
