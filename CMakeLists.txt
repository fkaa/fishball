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

add_subdirectory("${PROJECT_SOURCE_DIR}/external/freetype2" ${CMAKE_BINARY_DIR}/freetype2/)
add_subdirectory("${PROJECT_SOURCE_DIR}/external/glfw" ${CMAKE_BINARY_DIR}/glfw/)

set(FISHCONV_SOURCES
    src/shared/types.h
    src/array.h
    src/file.h
    src/file.c

    tool/main.c
)
set(FISHBALL_SOURCES
    src/shared/error.h
    src/shared/types.h
    src/mathtypes.h
    src/math.h
    src/main.c
    src/array.h
    src/file.h
    src/file.c
    src/font.h
    src/font.c
    src/window.h
    src/window.c
    src/gfx.h
    src/gfx.c
    src/fbgl.h
    src/fbgl.c
    src/voxel.h
    src/voxel.c
)

add_executable(fishconv ${FISHCONV_SOURCES})
add_executable(fishball ${FISHBALL_SOURCES})

add_custom_target(
    assets ALL
    #    TARGET fishball POST_BUILD
    COMMAND ${CMAKE_BINARY_DIR}/fishconv
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/asset
    COMMENT "Building assets.."
    DEPENDS fishconv
    VERBATIM
)

add_dependencies(fishball assets)

target_include_directories(fishconv PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_include_directories(fishball PRIVATE "${PROJECT_SOURCE_DIR}/external/glfw/include")
target_include_directories(fishball PRIVATE "${PROJECT_SOURCE_DIR}/external/freetype2/include")

target_link_libraries(fishball freetype glfw)
