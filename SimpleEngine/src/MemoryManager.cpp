#include "SimpleEngine/MemoryManager.h"

using namespace SEngine;

MemoryManager::MemoryManager(const vk::PhysicalDevice& physicalDevice, const vk::Device& device) {
    createAllocator(physicalDevice, device);
}

MemoryManager::~MemoryManager() {
    vmaDestroyAllocator(m_allocator);
}

void MemoryManager::createAllocator(const vk::PhysicalDevice& physicalDevice, const vk::Device& device) {
    VmaAllocatorCreateInfo info = {};
    info.physicalDevice = physicalDevice;
    info.device = device;

    vmaCreateAllocator(&info, &m_allocator);
}