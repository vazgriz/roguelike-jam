cmake_minimum_required (VERSION 3.13)

set(VULKAN_INCLUDE_DIR "" CACHE PATH "Path to Vulkan SDK include directory")
set(VULKAN_LIB "" CACHE FILEPATH "Path to vulkan-1.lib")

function(compile_shaders target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "FOLDER" "SOURCES")
    foreach(source ${arg_SOURCES})
        add_custom_command(
            OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${source}.spv
            DEPENDS ${arg_FOLDER}/${source}
            COMMAND
                ${SHADER_COMPILER}
                -o ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${source}.spv
                ${CMAKE_CURRENT_SOURCE_DIR}/${arg_FOLDER}/${source}
        )
        target_sources(${target} PRIVATE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${source}.spv)
    endforeach()
endfunction()

add_subdirectory("SimpleEngine")
add_subdirectory("test")