set(BUILD_SHARED_LIBS ON)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
OPTION(GLFW_BUILD_DOCS OFF)
OPTION(GLFW_INSTALL OFF)

add_subdirectory("${PROJECT_SOURCE_DIR}/external/glfw" ${CMAKE_BINARY_DIR}/glfw/)

string(RANDOM PDB_NAME)

add_library(window SHARED window.c)
target_compile_definitions(window PRIVATE -DDLL_EXPORT)
target_include_directories(window PRIVATE "${PROJECT_SOURCE_DIR}/external/glfw/include")
target_include_directories(window PRIVATE "${PROJECT_SOURCE_DIR}/src/")
target_link_libraries(window plugin glfw)
