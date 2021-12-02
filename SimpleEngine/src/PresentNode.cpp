#include "SimpleEngine/RenderGraph/PresentNode.h"
#include "SimpleEngine/Engine.h"
#include "SimpleEngine/Graphics.h"

using namespace SEngine;

PresentNode::PresentNode(Engine& engine, RenderGraph& graph, vk::PipelineStageFlagBits stage, AcquireNode& acquireNode)
    : RenderGraph::Node(graph, engine.getGraphics().presentQueue()) {
    m_presentQueue = &engine.getGraphics().presentQueue().queue;
    m_acquireNode = &acquireNode;

    vk::SemaphoreCreateInfo info = {};
    m_semaphore = std::make_unique<vk::raii::Semaphore>(graph.device(), info);

    //Dummy ImageUsage. Used to set up semaphore. This should not have any images submitted to it.
    m_imageUsage = std::make_unique<RenderGraph::ImageUsage>(*this, vk::ImageLayout::ePresentSrcKHR, vk::AccessFlagBits{}, stage);

    addExternalSignal(*m_semaphore);
}

void PresentNode::postRender(uint32_t currentFrame) {
    uint32_t imageIndex = m_acquireNode->swapchainIndex();

    vk::PresentInfoKHR info = {};
    info.swapchainCount = 1;
    info.pSwapchains = &*m_acquireNode->swapchain();
    info.pImageIndices = &imageIndex;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &**m_semaphore;

    m_presentQueue->presentKHR(info);
}