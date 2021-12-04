#pragma once

#include <SimpleEngine/SimpleEngine.h>
#include <SimpleEngine/RenderGraph/RenderGraph.h>
#include <SimpleEngine/RenderGraph/AcquireNode.h>
#include <SimpleEngine/RenderGraph/TransferNode.h>
#include <entt/signal/sigh.hpp>
#include <glm/glm.hpp>

class RenderNode : public SEngine::RenderGraph::Node {
public:
    RenderNode(SEngine::Engine& engine, SEngine::RenderGraph& graph, SEngine::AcquireNode& acquireNode, SEngine::TransferNode& transferNode);

    void setCamera(SEngine::Camera& camera);

    SEngine::RenderGraph::BufferUsage& bufferUsage() { return *m_bufferUsage; }
    SEngine::RenderGraph::ImageUsage& imageUsage() { return *m_imageUsage; }

    void preRender(uint32_t currentFrame);
    void render(uint32_t currentFrame, vk::raii::CommandBuffer& commandBuffer);
    void postRender(uint32_t currentFrame) {}

private:
    SEngine::Engine* m_engine;
    SEngine::Graphics* m_graphics;
    SEngine::AcquireNode* m_acquireNode;
    SEngine::TransferNode* m_transferNode;
    SEngine::Camera* m_camera;

    std::unique_ptr<vk::raii::RenderPass> m_renderPass;
    std::vector<vk::raii::Framebuffer> m_framebuffers;
    std::unique_ptr<vk::raii::DescriptorSetLayout> m_descriptorLayout;
    std::unique_ptr<vk::raii::DescriptorPool> m_descriptorPool;
    std::unique_ptr<vk::DescriptorSet> m_descriptor;
    std::unique_ptr<vk::raii::PipelineLayout> m_pipelineLayout;
    std::unique_ptr<vk::raii::Pipeline> m_pipeline;

    std::unique_ptr<SEngine::Buffer> m_vertexBuffer;
    std::unique_ptr<SEngine::Buffer> m_uniformBuffer;

    std::unique_ptr<SEngine::RenderGraph::BufferUsage> m_bufferUsage;
    std::unique_ptr<SEngine::RenderGraph::ImageUsage> m_imageUsage;

    entt::scoped_connection m_swapchainConnection;

    struct UniformData {
        glm::mat4 projectionMatrix;
    };

    UniformData m_uniform;

    void createRenderPass();
    void createFramebuffers();

    void recreateResources(vk::raii::SwapchainKHR* swapchain);

    void createDescriptorLayout();
    void createDescriptorPool();
    void createDescriptor();
    void createUniformBuffer();
    void updateUniformBuffer();

    vk::raii::ShaderModule createShader(const std::string& filename);
    void createPipeline();
    void createVertexData();
};