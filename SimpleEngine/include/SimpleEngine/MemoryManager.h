#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.h>

namespace SEngine {
class MemoryManager {
public:
    MemoryManager(const vk::PhysicalDevice& physicalDevice, const vk::Device& device);
    MemoryManager(const MemoryManager& other) = delete;
    MemoryManager& operator = (const MemoryManager& other) = delete;
    MemoryManager(MemoryManager&& other) = default;
    MemoryManager& operator = (MemoryManager && other) = default;
    ~MemoryManager();

    VmaAllocator allocator() const { return m_allocator; }

private:
    VmaAllocator m_allocator;

    void createAllocator(const vk::PhysicalDevice& physicalDevice, const vk::Device& device);
};
}