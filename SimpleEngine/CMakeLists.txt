cmake_minimum_required (VERSION 3.13)

project ("SimpleEngine")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_library("SimpleEngine")
set_property(TARGET "SimpleEngine" PROPERTY CXX_STANDARD 17)
target_sources("SimpleEngine"
  PUBLIC
    "include/SimpleEngine/SimpleEngine.h"
    "src/vk_mem_alloc.cpp"
    "include/SimpleEngine/ErrorDialog.h"
    "src/ErrorDialog.cpp"
    "include/SimpleEngine/Engine.h"
    "src/Engine.cpp"
    "include/SimpleEngine/Window.h"
    "src/Window.cpp"
    "include/SimpleEngine/Graphics.h"
    "src/Graphics.cpp"
    "include/SimpleEngine/RenderGraph/RenderGraph.h"
    "src/RenderGraph.cpp"
    "include/SimpleEngine/RenderGraph/AcquireNode.h"
    "src/AcquireNode.cpp"
    "include/SimpleEngine/RenderGraph/PresentNode.h"
    "src/PresentNode.cpp"
    "include/SimpleEngine/RenderGraph/TransferNode.h"
    "src/TransferNode.cpp"
    "include/SimpleEngine/Buffer.h"
    "src/Buffer.cpp"
    "include/SimpleEngine/MemoryManager.h"
    "src/MemoryManager.cpp"
    "include/SimpleEngine/Image.h"
    "src/Image.cpp"
    "include/SimpleEngine/Utilities.h"
    "src/Utilities.cpp"
    "include/SimpleEngine/Scene.h"
    "src/Scene.cpp"
)

# Download automatically, you can also just copy the conan.cmake file
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
   message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
   file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/release/0.17/conan.cmake"
                 "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)
conan_cmake_run(CONANFILE ../conanfile.txt
    BASIC_SETUP CMAKE_TARGETS NO_OUTPUT_DIRS
    BUILD missing)

target_link_libraries("SimpleEngine"
    CONAN_PKG::glfw
    CONAN_PKG::sdl
    CONAN_PKG::vulkan-memory-allocator
    CONAN_PKG::entt
    CONAN_PKG::glm
    ${VULKAN_LIB}
)
target_include_directories("SimpleEngine"
  PUBLIC
    "include"
  PRIVATE
    ${CONAN_INCLUDE_DIRS}
)