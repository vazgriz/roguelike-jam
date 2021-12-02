#pragma once
#include "SimpleEngine/RenderGraph/RenderGraph.h"
#include <vulkan/vulkan.hpp>
#include <iostream>
#include <entt/signal/sigh.hpp>

namespace SEngine {
    class AcquireNode : public RenderGraph::Node {
    public:
        AcquireNode(Engine& engine, RenderGraph& graph);

        RenderGraph::ImageUsage& imageUsage() const { return *m_imageUsage; }

        vk::raii::SwapchainKHR& swapchain() const { return *m_swapchain; }
        uint32_t swapchainIndex() const { return m_swapchainIndex; }

        void preRender(uint32_t currentFrame);
        void render(uint32_t currentFrame, vk::raii::CommandBuffer& commandBuffer) {}
        void postRender(uint32_t currentFrame) {}

    private:
        vk::raii::SwapchainKHR* m_swapchain;
        std::unique_ptr<vk::raii::Semaphore> m_semaphore;
        std::unique_ptr<RenderGraph::ImageUsage> m_imageUsage;
        uint32_t m_swapchainIndex;

        entt::scoped_connection m_swapchainConnection;

        void onSwapchainChanged(vk::raii::SwapchainKHR* swapchain);
    };
}