#pragma once
#include "SimpleEngine/Engine.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <iostream>

#include <vulkan/vulkan_raii.hpp>

namespace SEngine {
    struct BufferState;
    class Buffer;
    struct ImageState;
    class Image;

    struct QueueInfo;

    class RenderGraph {
    public:
        class Node;
        class Edge;
        class BufferEdge;
        class ImageEdge;

        struct BufferSegment {
            vk::DeviceSize size;
            vk::DeviceSize offset;
        };

        struct ImageSegment {
            vk::ImageSubresourceRange subresource;
        };

        class BufferUsage {
            friend class Node;
            friend class BufferEdge;
        public:
            BufferUsage(Node& node, vk::AccessFlagBits accessMask, vk::PipelineStageFlagBits stageFlags);

            Node& node() const { return *m_node; }
            vk::AccessFlagBits accessMask() const { return m_accessMask; }
            vk::PipelineStageFlagBits stageFlags() const { return m_stageFlags; }

            void sync(Buffer& buffer, vk::DeviceSize size, vk::DeviceSize offset);

        private:
            Node* m_node;
            vk::AccessFlagBits m_accessMask;
            vk::PipelineStageFlagBits m_stageFlags;
            std::vector<std::unordered_map<const vk::Buffer*, std::vector<BufferSegment>>> m_buffers;

            std::unordered_map<const vk::Buffer*, std::vector<BufferSegment>>& getSyncs(uint32_t currentFrame) { return m_buffers[currentFrame]; }

            void clear(uint32_t currentFrame);
        };

        class ImageUsage {
            friend class Node;
            friend class ImageEdge;
        public:
            ImageUsage(Node& node, vk::ImageLayout imageLayout, vk::AccessFlagBits accessMask, vk::PipelineStageFlagBits stageFlags);

            Node& node() const { return *m_node; }
            vk::ImageLayout imageLayout() const { return m_imageLayout; }
            vk::AccessFlagBits accessMask() const { return m_accessMask; }
            vk::PipelineStageFlagBits stageFlags() const { return m_stageFlags; }

            void sync(Image& image, vk::ImageSubresourceRange subresource);

        private:
            Node* m_node;
            vk::ImageLayout m_imageLayout;
            vk::AccessFlagBits m_accessMask;
            vk::PipelineStageFlagBits m_stageFlags;
            std::vector<std::unordered_map<const vk::Image*, std::vector<ImageSegment>>> m_images;

            std::unordered_map<const vk::Image*, std::vector<ImageSegment>>& getSyncs(uint32_t currentFrame) { return m_images[currentFrame]; }

            void clear(uint32_t currentFrame);
        };

        class Edge {
            friend class RenderGraph;
        public:
            Edge(Node& source, Node& dest);

            Node& source() const { return *m_sourceNode; }
            Node& dest() const { return *m_destNode; }

            virtual vk::PipelineStageFlagBits sourceStage() const = 0;
            virtual vk::PipelineStageFlagBits destStage() const = 0;

        private:
            Node* m_sourceNode;
            Node* m_destNode;

            virtual void recordSourceBarriers(uint32_t currentFrame, vk::raii::CommandBuffer& commandBuffer) = 0;
            virtual void recordDestBarriers(uint32_t currentFrame, vk::raii::CommandBuffer& commandBuffer) = 0;
        };

        class BufferEdge : public Edge {
            friend class RenderGraph;
        public:
            BufferEdge(BufferUsage& sourceUsage, BufferUsage& destUsage);
            vk::PipelineStageFlagBits sourceStage() const;
            vk::PipelineStageFlagBits destStage() const;

        private:
            BufferUsage* m_sourceUsage;
            BufferUsage* m_destUsage;
            std::vector<vk::BufferMemoryBarrier> m_barriers;

            void recordSourceBarriers(uint32_t currentFrame, vk::raii::CommandBuffer& commandBuffer);
            void recordDestBarriers(uint32_t currentFrame, vk::raii::CommandBuffer& commandBuffer);
        };

        class ImageEdge : public Edge {
            friend class RenderGraph;
        public:
            ImageEdge(ImageUsage& sourceUsage, ImageUsage& destUsage);
            vk::PipelineStageFlagBits sourceStage() const;
            vk::PipelineStageFlagBits destStage() const;

        private:
            ImageUsage* m_sourceUsage;
            ImageUsage* m_destUsage;
            std::vector<vk::ImageMemoryBarrier> m_barriers;

            void recordSourceBarriers(uint32_t currentFrame, vk::raii::CommandBuffer& commandBuffer);
            void recordDestBarriers(uint32_t currentFrame, vk::raii::CommandBuffer& commandBuffer);
        };

        class Node {
            friend class RenderGraph;
            friend class BufferUsage;
            friend class ImageUsage;

        public:
            Node(RenderGraph& graph, QueueInfo& queue);
            virtual ~Node() = default;

            RenderGraph& graph() const { return *m_graph; }
            QueueInfo& queue() const { return *m_queue; }
            uint32_t currentFrame() const { return m_graph->currentFrame(); }

            void addExternalWait(vk::raii::Semaphore& semaphore, vk::PipelineStageFlagBits stages);
            void addExternalSignal(vk::raii::Semaphore& semaphore);

            virtual void preRender(uint32_t currentFrame) = 0;
            virtual void render(uint32_t currentFrame, vk::raii::CommandBuffer& commandBuffer) = 0;
            virtual void postRender(uint32_t currentFrame) = 0;

        protected:
            vk::raii::CommandPool& commandPool() const { return *m_commandPool; }

        private:
            struct SubmitInfo {
                std::vector<vk::Semaphore> waitSemaphores;
                std::vector<vk::PipelineStageFlags> waitDstStageMask;
                std::vector<uint64_t> waitSemaphoreValues;
                std::vector<vk::Semaphore> signalSemaphores;
                std::vector<uint64_t> signalSemaphoreValues;
            };

            std::unique_ptr<vk::raii::Semaphore> m_semaphore;

            void createCommandBuffers(vk::raii::Device& device);
            void createSemaphore();

            QueueInfo* m_queue;
            RenderGraph* m_graph;
            std::vector<Node*> m_outputNodes;
            std::vector<BufferUsage*> m_bufferUsages;
            std::vector<ImageUsage*> m_imageUsages;
            std::vector<Edge*> m_inputEdges;
            std::vector<Edge*> m_outputEdges;

            std::unique_ptr<vk::raii::CommandPool> m_commandPool;
            std::vector<vk::raii::CommandBuffer> m_commandBuffers;
            SubmitInfo m_submitInfo;

            void addOutput(Node& output, Edge& edge);
            void addUsage(BufferUsage& usage);
            void addUsage(ImageUsage& usage);

            void makeInputTransfers(uint32_t currentFrame, vk::raii::CommandBuffer& commandBuffer);
            void makeOutputTransfers(uint32_t currentFrame, vk::raii::CommandBuffer& commandBuffer);
            void clearSync(uint32_t currentFrame);
            void internalRender(uint32_t currentFrame);
            void submit(uint32_t currentFrame);
        };

        RenderGraph(vk::raii::Device& device, uint32_t framesInFlight);
        RenderGraph(const RenderGraph& other) = delete;
        RenderGraph& operator = (const RenderGraph& other) = delete;
        RenderGraph(RenderGraph&& other) = default;
        RenderGraph& operator = (RenderGraph&& other) = default;
        ~RenderGraph();

        vk::raii::Device& device() const { return *m_device; }
        uint32_t framesInFlight() const { return m_framesInFlight; }
        uint32_t currentFrame() const { return m_currentFrame; }
        uint32_t frameCount() const { return m_frameCount; }
        bool isBaked() const { return m_baked; }

        template<class T, class... Args>
        T& addNode(Args&&... args) {
            m_nodes.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
            return *static_cast<T*>(m_nodes.back().get());
        }

        void addEdge(BufferEdge&& edge);
        void addEdge(ImageEdge&& edge);
        void bake();
        void wait();

        void execute();

        void queueDestroy(BufferState&& state);
        void queueDestroy(ImageState&& state);

    private:
        struct SemaphoreWaitInfo {
            std::vector<vk::Semaphore> semaphores;
            std::vector<uint64_t> values;
        };

        vk::raii::Device* m_device;
        uint32_t m_framesInFlight;
        mutable uint32_t m_currentFrame;
        uint32_t m_frameCount;
        bool m_baked;
        std::vector<std::unique_ptr<Node>> m_nodes;
        std::vector<std::unique_ptr<Edge>> m_edges;
        std::vector<Node*> m_nodeList;
        SemaphoreWaitInfo m_semaphoreWaitInfo;

        std::queue<std::vector<BufferState>> m_bufferDestroyQueue;
        std::queue<std::vector<ImageState>> m_imageDestroyQueue;

        void makeSemaphores();
        void wait(uint32_t targetFrame);
    };
}