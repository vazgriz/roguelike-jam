#pragma once
#include "SimpleEngine/RenderGraph/AcquireNode.h"
#include <memory>
#include <vulkan/vulkan.hpp>

namespace SEngine {
class Engine;

class PresentNode : public RenderGraph::Node {
public:
    PresentNode(Engine& engine, RenderGraph& graph, vk::PipelineStageFlagBits stage, AcquireNode& acquireNode);

    RenderGraph::ImageUsage& imageUsage() const { return *m_imageUsage; }

    void preRender(uint32_t currentFrame) {}
    void render(uint32_t currentFrame, vk::raii::CommandBuffer& commandBuffer) {}
    void postRender(uint32_t currentFrame);

private:
    const vk::raii::Queue* m_presentQueue;
    AcquireNode* m_acquireNode;
    std::unique_ptr<vk::raii::Semaphore> m_semaphore;
    std::unique_ptr<RenderGraph::ImageUsage> m_imageUsage;
};
}