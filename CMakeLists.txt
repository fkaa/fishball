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

set(TOML_INCLUDE_DIR
    external/toml/
)
set(TOML_SOURCES
    external/toml/toml.h
    external/toml/toml.c
)

set(FISHCONV_SOURCES
    src/shared/types.h
    src/shared/error.h
    src/shared/error.c
    src/array.h
    src/file.h
    src/file.c

    tool/main.c
    tool/bal.h
    tool/export.h
    tool/export.c
    tool/bdf.h
    tool/bdf.c
    tool/time.h
    tool/time.c

    tool/mem.c
    tool/mem.h

    "${TOML_SOURCES}"
)

set(FISHBALL_SOURCES
    src/shared/types.h
    src/shared/error.h
    src/shared/error.c
    src/mathtypes.h
    src/math.h
    src/main.c
    src/array.h
    src/file.h
    src/file.c
    src/import.h
    src/import.c
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

    tool/helper.c
    tool/helper.h

    "${PROJECT_SOURCE_DIR}/external/remotery/lib/Remotery.c"
)

add_executable(fishconv ${FISHCONV_SOURCES})
add_executable(fishball ${FISHBALL_SOURCES})

add_custom_target(
    asset_build ALL
    #    TARGET fishball POST_BUILD
    COMMAND ${CMAKE_BINARY_DIR}/fishconv
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/asset
    COMMENT "Building assets.."
    DEPENDS fishconv
    VERBATIM
)

add_dependencies(fishball asset_build)

target_include_directories(fishconv PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_include_directories(fishconv PRIVATE "${TOML_INCLUDE_DIR}")

target_include_directories(fishball PRIVATE "${PROJECT_SOURCE_DIR}/tool")
target_include_directories(fishball PRIVATE "${PROJECT_SOURCE_DIR}/external/glfw/include")
target_include_directories(fishball PRIVATE "${PROJECT_SOURCE_DIR}/external/remotery/lib")

target_link_libraries(fishball glfw)
