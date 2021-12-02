cmake_minimum_required (VERSION 3.13)

set(SHADER_COMPILER "" CACHE STRING "")

project ("HelloTriangle")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/test/HelloTriangle)

add_executable("HelloTriangle")
set_property(TARGET "HelloTriangle" PROPERTY CXX_STANDARD 17)
target_sources("HelloTriangle"
  PUBLIC
    "main.cpp"
    "RenderNode.h"
    "RenderNode.cpp"
)
target_link_libraries("HelloTriangle"
    "SimpleEngine"
)

if(WIN32 AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options("HelloTriangle" PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
endif()

compile_shaders("HelloTriangle"
  FOLDER
    "."
  SOURCES
    "shaders/triangle.vert"
    "shaders/triangle.frag"
)