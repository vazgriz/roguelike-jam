cmake_minimum_required (VERSION 3.13)

project ("Roguelike")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/Roguelike)

add_executable("Roguelike")
set_property(TARGET "Roguelike" PROPERTY CXX_STANDARD 17)
target_sources("Roguelike"
  PUBLIC
    "main.cpp"
    "RenderNode.h"
    "RenderNode.cpp"
    "Tiled/TiledReader.h"
    "Tiled/TiledReader.cpp"
    "Decompress.h"
    "Decompress.cpp"
)
target_link_libraries("Roguelike"
    "SimpleEngine"
)

if(WIN32 AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options("Roguelike" PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
endif()

compile_shaders("Roguelike"
  FOLDER
    "."
  SOURCES
    "shaders/triangle.vert"
    "shaders/triangle.frag"
)

add_custom_target(copy_data
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/data ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data
)
add_dependencies("Roguelike" copy_data)