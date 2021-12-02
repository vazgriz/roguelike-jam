#include "SimpleEngine/Buffer.h"
#include "SimpleEngine/Engine.h"
#include "SimpleEngine/Graphics.h"
#include "SimpleEngine/MemoryManager.h"
#include "SimpleEngine/RenderGraph/RenderGraph.h"

using namespace SEngine;

BufferState::BufferState(Engine* engine, size_t size, vk::raii::Buffer&& buffer, VmaAllocation allocation) : buffer(std::move(buffer)) {
    this->engine = engine;
    this->allocation = allocation;
    this->size = size;
}

BufferState::BufferState(BufferState&& other) noexcept : buffer(std::move(other.buffer)) {
    allocation = other.allocation;
    other.allocation = {};
    engine = other.engine;
    size = other.size;
}

BufferState& BufferState::operator = (BufferState&& other) noexcept {
    if (&other != this) {
        allocation = other.allocation;
        other.allocation = VK_NULL_HANDLE;
        engine = other.engine;
        size = other.size;
    }
    return *this;
}

BufferState::~BufferState() {
    vmaFreeMemory(engine->getGraphics().memory().allocator(), allocation);
}

Buffer::Buffer(Engine& engine, const vk::BufferCreateInfo& info, const VmaAllocationCreateInfo& allocInfo) {
    m_engine = &engine;

    VmaAllocator allocator = engine.getGraphics().memory().allocator();

    VkBuffer buffer;
    VmaAllocation allocation;
    vmaCreateBuffer(allocator, &(VkBufferCreateInfo)info, &allocInfo, &buffer, &allocation, &m_allocationInfo);

    m_bufferState = std::make_unique<BufferState>(m_engine, info.size, vk::raii::Buffer(engine.getGraphics().device(), buffer), allocation);
}

Buffer::~Buffer() {
    m_engine->getRenderGraph().queueDestroy(std::move(*m_bufferState));
}

void* Buffer::getMapping() const {
    return m_allocationInfo.pMappedData;
}