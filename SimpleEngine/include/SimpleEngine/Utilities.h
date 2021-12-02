#pragma once
#include <vector>
#include <string>
#include <vulkan/vulkan_raii.hpp>

namespace SEngine {
    size_t align(size_t ptr, size_t alignment);
    std::vector<char> readFile(const std::string& filename);
    vk::raii::ShaderModule createShaderModule(vk::raii::Device& device, const std::vector<char>& byteCode);
    size_t getFormatSize(vk::Format format);
}