#pragma once

#include <SimpleEngine/SimpleEngine.h>
#include <SimpleEngine/RenderGraph/RenderGraph.h>
#include <SimpleEngine/RenderGraph/AcquireNode.h>
#include <SimpleEngine/RenderGraph/TransferNode.h>
#include <entt/signal/sigh.hpp>
#include <glm/glm.hpp>

#include "Tiled/TiledReader.h"

class RenderNode : public SEngine::RenderGraph::Node {
public:
    RenderNode(SEngine::Engine& engine, SEngine::RenderGraph& graph, SEngine::AcquireNode& acquireNode, SEngine::TransferNode& transferNode);

    void setCamera(SEngine::Camera& camera);
    void loadMap(Tiled::Tileset& tileset, Tiled::Map& map);

    SEngine::RenderGraph::BufferUsage& bufferUsage() { return *m_bufferUsage; }
    SEngine::RenderGraph::ImageUsage& imageUsage() { return *m_imageUsage; }
    SEngine::RenderGraph::ImageUsage& textureUsage() { return *m_textureUsage; }

    void preRender(uint32_t currentFrame);
    void render(uint32_t currentFrame, vk::raii::CommandBuffer& commandBuffer);
    void postRender(uint32_t currentFrame) {}


private:
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 uv;
    };

    SEngine::Engine* m_engine;
    SEngine::Graphics* m_graphics;
    SEngine::AcquireNode* m_acquireNode;
    SEngine::TransferNode* m_transferNode;
    SEngine::Camera* m_camera;
    Tiled::Map* m_map;

    std::unique_ptr<SEngine::RenderGraph::ImageUsage> m_textureUsage;

    std::vector<SEngine::Image> m_spritesheets;
    std::vector<vk::raii::ImageView> m_spritesheetViews;
    std::vector<Vertex> m_vertexData;

    std::unique_ptr<vk::raii::RenderPass> m_renderPass;
    std::vector<vk::raii::Framebuffer> m_framebuffers;
    std::unique_ptr<vk::raii::DescriptorSetLayout> m_descriptorLayout;
    std::unique_ptr<vk::raii::DescriptorPool> m_descriptorPool;
    std::unique_ptr<vk::DescriptorSet> m_descriptor;
    std::unique_ptr<vk::raii::PipelineLayout> m_pipelineLayout;
    std::unique_ptr<vk::raii::Pipeline> m_pipeline;

    std::unique_ptr<SEngine::Buffer> m_vertexBuffer;
    std::unique_ptr<SEngine::Buffer> m_uniformBuffer;
    std::unique_ptr<vk::raii::Sampler> m_sampler;

    std::unique_ptr<SEngine::RenderGraph::BufferUsage> m_bufferUsage;
    std::unique_ptr<SEngine::RenderGraph::ImageUsage> m_imageUsage;

    entt::scoped_connection m_swapchainConnection;

    struct UniformData {
        glm::mat4 projectionMatrix;
        glm::mat4 viewMatrix;
    };

    UniformData m_uniform;

    void createRenderPass();
    void createFramebuffers();

    void recreateResources(vk::raii::SwapchainKHR* swapchain);

    void createUniformBuffer();
    void createSampler();
    void createDescriptorLayout();
    void createDescriptorPool();
    void createDescriptor();
    void updateDescriptor();

    vk::raii::ShaderModule createShader(const std::string& filename);
    void createPipeline();
    void createVertexData();
    void createVertexBuffer();
};