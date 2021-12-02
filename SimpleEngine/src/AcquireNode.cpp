#include "SimpleEngine/RenderGraph/AcquireNode.h"

#include "SimpleEngine/Engine.h"
#include "SimpleEngine/Graphics.h"

using namespace SEngine;

AcquireNode::AcquireNode(Engine& engine, RenderGraph& graph)
    : RenderGraph::Node(graph, engine.getGraphics().graphicsQueue())
{
    m_swapchain = engine.getGraphics().swapchain();

    vk::SemaphoreCreateInfo info = {};
    m_semaphore = std::make_unique<vk::raii::Semaphore>(graph.device(), info);

    addExternalWait(*m_semaphore, vk::PipelineStageFlagBits::eColorAttachmentOutput);

    m_imageUsage = std::make_unique<RenderGraph::ImageUsage>(*this, vk::ImageLayout::eUndefined, vk::AccessFlagBits{}, vk::PipelineStageFlagBits::eBottomOfPipe);

    m_swapchainConnection = engine.getGraphics().onSwapchainChanged().connect<&AcquireNode::onSwapchainChanged>(this);
}

void AcquireNode::preRender(uint32_t currentFrame) {
    auto result = m_swapchain->acquireNextImage(std::numeric_limits<uint64_t>::max(), **m_semaphore, nullptr);
    m_swapchainIndex = std::get<1>(result);

    vk::Result resultVK = std::get<0>(result);
    if (resultVK == vk::Result::eSuboptimalKHR || resultVK == vk::Result::eErrorOutOfDateKHR) {
        throw std::runtime_error("bad acquire");
    }
}

void AcquireNode::onSwapchainChanged(vk::raii::SwapchainKHR* swapchain) {
    m_swapchain = swapchain;
}