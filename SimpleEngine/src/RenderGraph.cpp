#include "SimpleEngine/RenderGraph/RenderGraph.h"
#include "SimpleEngine/DirectedAcyclicGraph.h"
#include "SimpleEngine/Buffer.h"
#include "SimpleEngine/Image.h"
#include "SimpleEngine/Graphics.h"

using namespace SEngine;

RenderGraph::BufferUsage::BufferUsage(Node& node, vk::AccessFlagBits accessMask, vk::PipelineStageFlagBits stageFlags) {
    m_node = &node;
    m_node->addUsage(*this);
    m_accessMask = accessMask;
    m_stageFlags = stageFlags;
    m_buffers.resize(node.graph().framesInFlight());
}

void RenderGraph::BufferUsage::sync(Buffer& buffer, vk::DeviceSize size, vk::DeviceSize offset) {
    const vk::Buffer* vkBuffer = &buffer.buffer();

    auto& buffers = getSyncs(m_node->currentFrame());
    auto it = buffers.find(vkBuffer);

    if (it == buffers.end()) {
        buffers.insert({ vkBuffer, { { size, offset } } });
    } else {
        buffers[vkBuffer].push_back({ size, offset });
    }
}

void RenderGraph::BufferUsage::clear(uint32_t currentFrame) {
    m_buffers[currentFrame].clear();
}

RenderGraph::ImageUsage::ImageUsage(Node& node, vk::ImageLayout imageLayout, vk::AccessFlagBits accessMask, vk::PipelineStageFlagBits stageFlags) {
    m_node = &node;
    m_node->addUsage(*this);
    m_imageLayout = imageLayout;
    m_accessMask = accessMask;
    m_stageFlags = stageFlags;
    m_images.resize(node.graph().framesInFlight());
}

void RenderGraph::ImageUsage::sync(Image& image, vk::ImageSubresourceRange subresource) {
    const vk::Image* vkImage = &image.image();

    auto& images = getSyncs(m_node->currentFrame());
    auto it = images.find(vkImage);

    if (it == images.end()) {
        images.insert({ vkImage, { { subresource } } });
    } else {
        images[vkImage].push_back({ subresource });
    }
}

void RenderGraph::ImageUsage::clear(uint32_t currentFrame) {
    m_images[currentFrame].clear();
}

RenderGraph::Edge::Edge(Node& source, Node& dest) {
    m_sourceNode = &source;
    m_destNode = &dest;
}

RenderGraph::BufferEdge::BufferEdge(BufferUsage& sourceUsage, BufferUsage& destUsage) : Edge(sourceUsage.node(), destUsage.node()) {
    m_sourceUsage = &sourceUsage;
    m_destUsage = &destUsage;
}

vk::PipelineStageFlagBits RenderGraph::BufferEdge::sourceStage() const {
    return m_sourceUsage->stageFlags();
}

vk::PipelineStageFlagBits RenderGraph::BufferEdge::destStage() const {
    return m_destUsage->stageFlags();
}

void RenderGraph::BufferEdge::recordSourceBarriers(uint32_t currentFrame, vk::raii::CommandBuffer& commandBuffer) {
    m_barriers.clear();
    vk::PipelineStageFlags sourceStageFlags = m_sourceUsage->stageFlags();
    vk::PipelineStageFlags destStageFlags = m_destUsage->stageFlags();

    if (source().queue().familyIndex != dest().queue().familyIndex) {
        destStageFlags = vk::PipelineStageFlagBits::eBottomOfPipe;  //override dest stage flags when transfering queue ownership. recordDestBarriers will handle the other side
    }

    for (auto& sourcePair : m_sourceUsage->getSyncs(currentFrame)) {
        auto& destSyncs = m_destUsage->getSyncs(currentFrame);
        auto it = destSyncs.find(sourcePair.first);

        if (it != destSyncs.end()) {
            for (auto& sourceSegment : sourcePair.second) {
                vk::BufferMemoryBarrier barrier = {};
                barrier.buffer = *sourcePair.first;
                barrier.offset = sourceSegment.offset;
                barrier.size = sourceSegment.size;
                barrier.srcAccessMask = m_sourceUsage->accessMask();

                if (source().queue().familyIndex == dest().queue().familyIndex) {
                    barrier.dstAccessMask = m_destUsage->accessMask();
                }

                barrier.srcQueueFamilyIndex = source().queue().familyIndex;
                barrier.dstQueueFamilyIndex = dest().queue().familyIndex;

                m_barriers.push_back(barrier);
            }
        }
    }

    if (m_barriers.size() > 0) {
        commandBuffer.pipelineBarrier(m_sourceUsage->stageFlags(), m_destUsage->stageFlags(), {},
            nullptr,
            m_barriers,
            nullptr
        );
    }
}

void RenderGraph::BufferEdge::recordDestBarriers(uint32_t currentFrame, vk::raii::CommandBuffer& commandBuffer) {
    if (source().queue().familyIndex == dest().queue().familyIndex) return;
    m_barriers.clear();
    vk::PipelineStageFlags sourceStageFlags = m_sourceUsage->stageFlags();
    vk::PipelineStageFlags destStageFlags = m_destUsage->stageFlags();

    if (source().queue().familyIndex != dest().queue().familyIndex) {
        sourceStageFlags = vk::PipelineStageFlagBits::eTopOfPipe;   //override source stage flags when transfering queue ownership. recordDestBarriers will handle the other side
    }

    auto& sourceSyncs = m_sourceUsage->getSyncs(currentFrame);
    auto& destSyncs = m_destUsage->getSyncs(currentFrame);

    for (auto& destPair : destSyncs) {
        auto it = sourceSyncs.find(destPair.first);

        if (it != sourceSyncs.end()) {
            for (auto& destSegment : it->second) {
                vk::BufferMemoryBarrier barrier = {};
                barrier.buffer = *destPair.first;
                barrier.offset = destSegment.offset;
                barrier.size = destSegment.size;

                if (source().queue().familyIndex == dest().queue().familyIndex) {
                    barrier.srcAccessMask = m_sourceUsage->accessMask();
                }

                barrier.dstAccessMask = m_destUsage->accessMask();
                barrier.srcQueueFamilyIndex = source().queue().familyIndex;
                barrier.dstQueueFamilyIndex = dest().queue().familyIndex;

                m_barriers.push_back(barrier);
            }
        }
    }

    if (m_barriers.size() > 0) {
        commandBuffer.pipelineBarrier(m_sourceUsage->stageFlags(), m_destUsage->stageFlags(), {},
            nullptr,
            m_barriers,
            nullptr
        );
    }
}

RenderGraph::ImageEdge::ImageEdge(ImageUsage& sourceUsage, ImageUsage& destUsage) : Edge(sourceUsage.node(), destUsage.node()) {
    m_sourceUsage = &sourceUsage;
    m_destUsage = &destUsage;
}

vk::PipelineStageFlagBits RenderGraph::ImageEdge::sourceStage() const {
    return m_sourceUsage->stageFlags();
}

vk::PipelineStageFlagBits RenderGraph::ImageEdge::destStage() const {
    return m_destUsage->stageFlags();
}

void RenderGraph::ImageEdge::recordSourceBarriers(uint32_t currentFrame, vk::raii::CommandBuffer& commandBuffer) {
    m_barriers.clear();
    vk::PipelineStageFlags sourceStageFlags = m_sourceUsage->stageFlags();
    vk::PipelineStageFlags destStageFlags = m_destUsage->stageFlags();

    if (source().queue().familyIndex != dest().queue().familyIndex) {
        destStageFlags = vk::PipelineStageFlagBits::eBottomOfPipe;  //override dest stage flags when transfering queue ownership. recordDestBarriers will handle the other side
    }

    auto& sourceSyncs = m_sourceUsage->getSyncs(currentFrame);
    auto& destSyncs = m_destUsage->getSyncs(currentFrame);

    for (auto& sourcePair : sourceSyncs) {
        auto it = destSyncs.find(sourcePair.first);

        if (it != destSyncs.end()) {
            for (auto& sourceSegment : sourcePair.second) {
                vk::ImageMemoryBarrier barrier = {};
                barrier.image = *sourcePair.first;
                barrier.oldLayout = m_sourceUsage->imageLayout();
                barrier.newLayout = m_destUsage->imageLayout();
                barrier.subresourceRange = sourceSegment.subresource;
                barrier.srcAccessMask = m_sourceUsage->accessMask();

                if (source().queue().familyIndex == dest().queue().familyIndex) {
                    barrier.dstAccessMask = m_destUsage->accessMask();
                }

                barrier.srcQueueFamilyIndex = source().queue().familyIndex;
                barrier.dstQueueFamilyIndex = dest().queue().familyIndex;

                m_barriers.push_back(barrier);
            }
        }
    }

    if (m_barriers.size() > 0) {
        commandBuffer.pipelineBarrier(m_sourceUsage->stageFlags(), m_destUsage->stageFlags(), {},
            nullptr,
            nullptr,
            m_barriers
        );
    }
}

void RenderGraph::ImageEdge::recordDestBarriers(uint32_t currentFrame, vk::raii::CommandBuffer& commandBuffer) {
    if (source().queue().familyIndex == dest().queue().familyIndex) return;
    m_barriers.clear();
    vk::PipelineStageFlags sourceStageFlags = m_sourceUsage->stageFlags();
    vk::PipelineStageFlags destStageFlags = m_destUsage->stageFlags();

    if (source().queue().familyIndex != dest().queue().familyIndex) {
        sourceStageFlags = vk::PipelineStageFlagBits::eTopOfPipe;    //override source stage flags when transfering queue ownership. recordDestBarriers will handle the other side
    }

    auto& sourceSyncs = m_sourceUsage->getSyncs(currentFrame);
    auto& destSyncs = m_destUsage->getSyncs(currentFrame);

    for (auto& destPair : destSyncs) {
        auto it = sourceSyncs.find(destPair.first);

        if (it != sourceSyncs.end()) {
            for (auto& destSegment : it->second) {
                vk::ImageMemoryBarrier barrier = {};
                barrier.image = *destPair.first;
                barrier.oldLayout = m_sourceUsage->imageLayout();
                barrier.newLayout = m_destUsage->imageLayout();
                barrier.subresourceRange = destSegment.subresource;

                if (source().queue().familyIndex == dest().queue().familyIndex) {
                    barrier.srcAccessMask = m_sourceUsage->accessMask();
                }

                barrier.dstAccessMask = m_destUsage->accessMask();
                barrier.srcQueueFamilyIndex = source().queue().familyIndex;
                barrier.dstQueueFamilyIndex = dest().queue().familyIndex;

                m_barriers.push_back(barrier);
            }
        }
    }

    if (m_barriers.size() > 0) {
        commandBuffer.pipelineBarrier(m_sourceUsage->stageFlags(), m_destUsage->stageFlags(), {},
            nullptr,
            nullptr,
            m_barriers
        );
    }
}

RenderGraph::Node::Node(RenderGraph& graph, QueueInfo& queue) {
    m_graph = &graph;
    m_queue = &queue;

    createCommandBuffers(graph.device());
    createSemaphore();
}

void RenderGraph::Node::createCommandBuffers(vk::raii::Device& device) {
    vk::CommandPoolCreateInfo info = {};
    info.queueFamilyIndex = m_queue->familyIndex;
    info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

    m_commandPool = std::make_unique<vk::raii::CommandPool>(m_queue->device, info);

    vk::CommandBufferAllocateInfo allocInfo = {};
    allocInfo.commandPool = **m_commandPool;
    allocInfo.commandBufferCount = m_graph->framesInFlight();
    allocInfo.level = vk::CommandBufferLevel::ePrimary;

    vk::raii::CommandBuffers commandBuffers(device, allocInfo);

    m_commandBuffers = std::move(commandBuffers);
}

void RenderGraph::Node::createSemaphore() {
    vk::SemaphoreTypeCreateInfo timelineInfo = {};
    timelineInfo.semaphoreType = vk::SemaphoreType::eTimeline;

    vk::SemaphoreCreateInfo info = {};
    info.pNext = &timelineInfo;

    m_semaphore = std::make_unique<vk::raii::Semaphore>(m_graph->device(), info);
}

void RenderGraph::Node::addExternalWait(vk::raii::Semaphore& semaphore, vk::PipelineStageFlagBits stages) {
    m_submitInfo.waitSemaphores.push_back(*semaphore);
    m_submitInfo.waitDstStageMask.push_back(stages);
    m_submitInfo.waitSemaphoreValues.push_back(0);
}

void RenderGraph::Node::addExternalSignal(vk::raii::Semaphore& semaphore) {
    m_submitInfo.signalSemaphores.push_back(*semaphore);
    m_submitInfo.signalSemaphoreValues.push_back(0);
}

void RenderGraph::Node::addUsage(BufferUsage& usage) {
    m_bufferUsages.push_back(&usage);
}

void RenderGraph::Node::addUsage(ImageUsage& usage) {
    m_imageUsages.push_back(&usage);
}

void RenderGraph::Node::addOutput(Node& output, Edge& edge) {
    m_outputNodes.push_back(&output);
    m_outputEdges.push_back(&edge);
    output.m_inputEdges.push_back(&edge);
}

void RenderGraph::Node::makeInputTransfers(uint32_t currentFrame, vk::raii::CommandBuffer& commandBuffer) {
    for (auto& edge : m_inputEdges) {
        edge->recordDestBarriers(currentFrame, commandBuffer);
    }
}

void RenderGraph::Node::makeOutputTransfers(uint32_t currentFrame, vk::raii::CommandBuffer& commandBuffer) {
    for (auto& edge : m_outputEdges) {
        edge->recordSourceBarriers(currentFrame, commandBuffer);
    }
}

void RenderGraph::Node::clearSync(uint32_t currentFrame) {
    for (auto usage : m_bufferUsages) {
        usage->clear(currentFrame);
    }

    for (auto usage : m_imageUsages) {
        usage->clear(currentFrame);
    }
}

void RenderGraph::Node::internalRender(uint32_t currentFrame) {
    vk::raii::CommandBuffer& commandBuffer = m_commandBuffers[currentFrame];

    commandBuffer.reset((vk::CommandBufferResetFlagBits)0);

    vk::CommandBufferBeginInfo info = {};
    info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    commandBuffer.begin(info);

    makeInputTransfers(currentFrame, commandBuffer);
    render(currentFrame, commandBuffer);
    makeOutputTransfers(currentFrame, commandBuffer);

    commandBuffer.end();
}

void RenderGraph::Node::submit(uint32_t currentFrame) {
    vk::TimelineSemaphoreSubmitInfo timelineInfo = {};
    timelineInfo.waitSemaphoreValueCount = static_cast<uint32_t>(m_submitInfo.waitSemaphoreValues.size());
    timelineInfo.pWaitSemaphoreValues = m_submitInfo.waitSemaphoreValues.data();
    timelineInfo.signalSemaphoreValueCount = static_cast<uint32_t>(m_submitInfo.signalSemaphoreValues.size());
    timelineInfo.pSignalSemaphoreValues = m_submitInfo.signalSemaphoreValues.data();

    vk::SubmitInfo info = {};
    info.pNext = &timelineInfo;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &*m_commandBuffers[currentFrame];
    info.waitSemaphoreCount = static_cast<uint32_t>(m_submitInfo.waitSemaphores.size());
    info.pWaitSemaphores = m_submitInfo.waitSemaphores.data();
    info.pWaitDstStageMask = m_submitInfo.waitDstStageMask.data();
    info.signalSemaphoreCount = static_cast<uint32_t>(m_submitInfo.signalSemaphores.size());
    info.pSignalSemaphores = m_submitInfo.signalSemaphores.data();
    
    for (auto& value : m_submitInfo.waitSemaphoreValues) {
        value = m_graph->frameCount();
    }

    for (auto& value : m_submitInfo.signalSemaphoreValues) {
        value = m_graph->frameCount();
    }

    m_queue->queue.submit(info, nullptr);
}

RenderGraph::RenderGraph(vk::raii::Device& device, uint32_t framesInFlight) {
    m_device = &device;
    m_framesInFlight = framesInFlight;
    m_frameCount = framesInFlight;
    m_currentFrame = 0;
    m_baked = false;
    m_semaphoreWaitInfo = {};

    for (uint32_t i = 0; i < framesInFlight; i++) {
        m_bufferDestroyQueue.push({});
    }

    for (uint32_t i = 0; i < framesInFlight; i++) {
        m_imageDestroyQueue.push({});
    }
}

RenderGraph::~RenderGraph() {
    m_nodes.clear();

    while (m_bufferDestroyQueue.size() > 0) {
        m_bufferDestroyQueue.pop();
    }

    while (m_imageDestroyQueue.size() > 0) {
        m_imageDestroyQueue.pop();
    }
}

void RenderGraph::addEdge(BufferEdge&& edge) {
    m_edges.emplace_back(std::make_unique<BufferEdge>(std::move(edge)));
    auto& edge_ = m_edges.back();
    edge_->source().addOutput(edge_->dest(), *edge_);
}

void RenderGraph::addEdge(ImageEdge&& edge) {
    m_edges.emplace_back(std::make_unique<ImageEdge>(std::move(edge)));
    auto& edge_ = m_edges.back();
    edge_->source().addOutput(edge_->dest(), *edge_);
}

void RenderGraph::bake() {
    std::unordered_set<Node*> nodeSet;

    for (auto& ptr : m_nodes) {
        nodeSet.insert(ptr.get());
    }

    m_nodeList = topologicalSort<Node>(nodeSet, [](Node* node) {
        return node->m_outputNodes;
    });

    makeSemaphores();

    m_baked = true;
}

void RenderGraph::makeSemaphores() {
    for (Node* node : m_nodeList) {
        auto& semaphore = *node->m_semaphore;

        node->m_submitInfo.signalSemaphores.push_back(*semaphore);
        node->m_submitInfo.signalSemaphoreValues.push_back(0);

        m_semaphoreWaitInfo.semaphores.push_back(*semaphore);
        m_semaphoreWaitInfo.values.push_back(0);

        for(size_t i = 0; i < node->m_outputNodes.size(); i++) {
            auto outputNode = node->m_outputNodes[i];
            outputNode->m_submitInfo.waitSemaphores.push_back(*semaphore);
            outputNode->m_submitInfo.waitDstStageMask.push_back(node->m_outputEdges[i]->destStage());
            outputNode->m_submitInfo.waitSemaphoreValues.push_back(0);
        }
    }
}

void RenderGraph::wait() {
    wait(frameCount() - m_framesInFlight); //wait until previous frame finishes
}

void RenderGraph::wait(uint32_t targetFrame) {
    for (auto& value : m_semaphoreWaitInfo.values) {
        value = targetFrame;
    }

    vk::SemaphoreWaitInfo info = {};
    info.semaphoreCount = static_cast<uint32_t>(m_semaphoreWaitInfo.semaphores.size());
    info.pSemaphores = m_semaphoreWaitInfo.semaphores.data();
    info.pValues = m_semaphoreWaitInfo.values.data();

    m_device->waitSemaphores(info, std::numeric_limits<uint64_t>::max());
}

void RenderGraph::execute() {
    for (auto node : m_nodeList) {
        node->clearSync(m_currentFrame);
    }

    for (auto node : m_nodeList) {
        node->preRender(m_currentFrame);
    }

    wait(frameCount() - framesInFlight());

    m_bufferDestroyQueue.pop();
    m_bufferDestroyQueue.push({});

    m_imageDestroyQueue.pop();
    m_imageDestroyQueue.push({});

    for (auto node : m_nodeList) {
        node->internalRender(m_currentFrame);
    }

    for (auto node : m_nodeList) {
        node->submit(m_currentFrame);
    }

    for (auto node : m_nodeList) {
        node->postRender(m_currentFrame);
    }

    m_frameCount++;
    m_currentFrame = m_frameCount % m_framesInFlight;
}

void RenderGraph::queueDestroy(BufferState&& state) {
    m_bufferDestroyQueue.back().emplace_back(std::move(state));
}

void RenderGraph::queueDestroy(ImageState&& state) {
    m_imageDestroyQueue.back().emplace_back(std::move(state));
}