#pragma once
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_raii.hpp>

namespace SEngine {
class Engine;
class RenderGraph;

struct BufferState {
    Engine* engine;
    vk::raii::Buffer buffer;
    VmaAllocation allocation;
    size_t size;

    BufferState(Engine* engine, size_t size, vk::raii::Buffer&& buffer, VmaAllocation allocation);
    BufferState(const BufferState& other) = delete;
    BufferState& operator = (const BufferState& other) = delete;
    BufferState(BufferState&& other) noexcept;
    BufferState& operator = (BufferState&& other) noexcept;
    ~BufferState();
};

class Buffer {
public:
    Buffer(Engine& engine, const vk::BufferCreateInfo& info, const VmaAllocationCreateInfo& allocInfo);
    ~Buffer();

    const vk::Buffer& buffer() const { return *m_bufferState->buffer; }
    void* getMapping() const;
    size_t size() const { return m_bufferState->size; }
    size_t offset() const { return m_allocationInfo.offset; }

private:
    Engine* m_engine;
    std::unique_ptr<BufferState> m_bufferState;
    VmaAllocationInfo m_allocationInfo;
};
}