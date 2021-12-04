#include "RenderNode.h"
#include <SimpleEngine/SimpleEngine.h>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
};

const std::vector<Vertex> vertexData = {
    {{  0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }},
    {{  1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }},
    {{ -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }}
};

RenderNode::RenderNode(SEngine::Engine& engine, SEngine::RenderGraph& graph, SEngine::AcquireNode& acquireNode, SEngine::TransferNode& transferNode)
    : SEngine::RenderGraph::Node(graph, engine.getGraphics().graphicsQueue()) {
    m_engine = &engine;
    m_graphics = &engine.getGraphics();
    m_acquireNode = &acquireNode;
    m_transferNode = &transferNode;

    m_bufferUsage = std::make_unique<SEngine::RenderGraph::BufferUsage>(*this, vk::AccessFlagBits::eVertexAttributeRead, vk::PipelineStageFlagBits::eVertexInput);

    m_imageUsage = std::make_unique<SEngine::RenderGraph::ImageUsage>(
        *this, vk::ImageLayout::eColorAttachmentOptimal, vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput
    );

    createRenderPass();
    createFramebuffers();
    createDescriptorLayout();
    createDescriptorPool();
    createDescriptor();
    createUniformBuffer();
    updateUniformBuffer();
    createPipeline();
    createVertexData();

    m_swapchainConnection = engine.getGraphics().onSwapchainChanged().connect<&RenderNode::recreateResources>(this);
    m_uniform = {};
}

void RenderNode::setCamera(SEngine::Camera& camera) {
    m_camera = &camera;
}

void RenderNode::preRender(uint32_t currentFrame) {
    if (m_camera != nullptr) {
        m_uniform.projectionMatrix = m_camera->projection();
    }

    m_transferNode->transfer(*m_uniformBuffer, sizeof(UniformData), 0, &m_uniform);
}

void RenderNode::render(uint32_t currentFrame, vk::raii::CommandBuffer& commandBuffer) {
    uint32_t imageIndex = m_acquireNode->swapchainIndex();
    vk::ClearValue clear = {};

    vk::RenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.renderPass = **m_renderPass;
    renderPassInfo.framebuffer = *m_framebuffers[imageIndex];
    renderPassInfo.renderArea = vk::Rect2D{ {}, m_graphics->swapchainExtent() };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clear;

    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, **m_pipeline);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, **m_pipelineLayout, 0, *m_descriptor, nullptr);
    commandBuffer.bindVertexBuffers(0, m_vertexBuffer->buffer(), { 0 });

    vk::Extent2D extent = m_graphics->swapchainExtent();

    vk::Viewport viewport = {};
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);

    vk::Rect2D scissor = {};
    scissor.extent = extent;

    commandBuffer.setViewport(0, viewport);
    commandBuffer.setScissor(0, scissor);

    commandBuffer.draw(3, 1, 0, 0);

    commandBuffer.endRenderPass();
}

void RenderNode::recreateResources(vk::raii::SwapchainKHR* swapchain) {
    m_renderPass.reset();

    if (swapchain == nullptr) return;

    createRenderPass();
    createFramebuffers();
}

void RenderNode::createRenderPass() {
    vk::AttachmentDescription colorAttachment = {};
    colorAttachment.format = m_graphics->swapchainFormat();
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass = {};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    vk::RenderPassCreateInfo info = {};
    info.attachmentCount = 1;
    info.pAttachments = &colorAttachment;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;

    m_renderPass = std::make_unique<vk::raii::RenderPass>(m_graphics->device(), info);
}

void RenderNode::createFramebuffers() {
    auto& imageViews = m_graphics->swapchainImageViews();
    m_framebuffers.clear();

    for (uint32_t i = 0; i < imageViews.size(); i++) {
        vk::Format format = m_graphics->swapchainFormat();
        vk::Extent2D extent = m_graphics->swapchainExtent();

        vk::FramebufferCreateInfo info = {};
        info.renderPass = **m_renderPass;
        info.width = extent.width;
        info.height = extent.height;
        info.layers = 1;
        info.attachmentCount = 1;
        info.pAttachments = &*imageViews[i];

        m_framebuffers.emplace_back(m_graphics->device(), info);
    }
}

void RenderNode::createDescriptorLayout() {
    vk::DescriptorSetLayoutBinding binding = {};
    binding.descriptorType = vk::DescriptorType::eUniformBuffer;
    binding.descriptorCount = 1;
    binding.stageFlags = vk::ShaderStageFlagBits::eVertex;

    vk::DescriptorSetLayoutCreateInfo info = {};
    info.bindingCount = 1;
    info.pBindings = &binding;

    m_descriptorLayout = std::make_unique<vk::raii::DescriptorSetLayout>(m_graphics->device(), info);
}

void RenderNode::createDescriptorPool() {
    vk::DescriptorPoolSize poolSize = {};
    poolSize.descriptorCount = 1;
    poolSize.type = vk::DescriptorType::eUniformBuffer;

    vk::DescriptorPoolCreateInfo info = {};
    info.maxSets = 1;
    info.poolSizeCount = 1;
    info.pPoolSizes = &poolSize;

    m_descriptorPool = std::make_unique<vk::raii::DescriptorPool>(m_graphics->device(), info);
}

void RenderNode::createDescriptor() {
    vk::DescriptorSetAllocateInfo info = {};
    info.descriptorPool = **m_descriptorPool;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &**m_descriptorLayout;

    auto descriptors = (*m_graphics->device()).allocateDescriptorSets(info);
    m_descriptor = std::make_unique<vk::DescriptorSet>(std::move(descriptors[0]));
}

void RenderNode::createUniformBuffer() {
    vk::BufferCreateInfo info = {};
    info.size = sizeof(UniformData);
    info.usage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    m_uniformBuffer = std::make_unique<SEngine::Buffer>(*m_engine, info, allocInfo);
}

void RenderNode::updateUniformBuffer() {
    vk::DescriptorBufferInfo info = {};
    info.buffer = m_uniformBuffer->buffer();
    info.range = m_uniformBuffer->size();

    vk::WriteDescriptorSet write = {};
    write.descriptorCount = 1;
    write.descriptorType = vk::DescriptorType::eUniformBuffer;
    write.dstSet = *m_descriptor;
    write.pBufferInfo = &info;

    m_graphics->device().updateDescriptorSets(write, nullptr);
}

vk::raii::ShaderModule RenderNode::createShader(const std::string& filename) {
    auto data = SEngine::readFile(filename);

    vk::ShaderModuleCreateInfo info = {};
    info.codeSize = data.size();
    info.pCode = reinterpret_cast<const uint32_t*>(data.data());

    return vk::raii::ShaderModule(m_graphics->device(), info);
}

void RenderNode::createPipeline() {
    auto vertexShader = createShader("shaders/triangle.vert.spv");
    auto fragmentShader = createShader("shaders/triangle.frag.spv");

    vk::PipelineShaderStageCreateInfo vertexStage = {};
    vertexStage.stage = vk::ShaderStageFlagBits::eVertex;
    vertexStage.module = *vertexShader;
    vertexStage.pName = "main";

    vk::PipelineShaderStageCreateInfo fragmentStage = {};
    fragmentStage.stage = vk::ShaderStageFlagBits::eFragment;
    fragmentStage.module = *fragmentShader;
    fragmentStage.pName = "main";

    std::array<vk::PipelineShaderStageCreateInfo, 2> stages = { vertexStage, fragmentStage };

    vk::VertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = vk::VertexInputRate::eVertex;

    std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions = {};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    vk::PipelineVertexInputStateCreateInfo vertexInput = {};
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &bindingDescription;
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInput.pVertexAttributeDescriptions = attributeDescriptions.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

    vk::PipelineViewportStateCreateInfo viewportState = {};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.lineWidth = 1.0f;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR
        | vk::ColorComponentFlagBits::eG
        | vk::ColorComponentFlagBits::eB
        | vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    vk::DynamicState dynamicStates[] = {
        vk::DynamicState::eScissor,
        vk::DynamicState::eViewport
    };

    vk::PipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    vk::PipelineMultisampleStateCreateInfo multisample = {};
    multisample.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &**m_descriptorLayout;

    m_pipelineLayout = std::make_unique<vk::raii::PipelineLayout>(m_graphics->device(), pipelineLayoutInfo);

    vk::GraphicsPipelineCreateInfo info = {};
    info.stageCount = 2;
    info.pStages = stages.data();
    info.pVertexInputState = &vertexInput;
    info.pInputAssemblyState = &inputAssembly;
    info.pViewportState = &viewportState;
    info.pRasterizationState = &rasterizer;
    info.pColorBlendState = &colorBlending;
    info.pDynamicState = &dynamicState;
    info.pMultisampleState = &multisample;
    info.layout = **m_pipelineLayout;
    info.renderPass = **m_renderPass;

    m_pipeline = std::make_unique<vk::raii::Pipeline>(m_graphics->device(), nullptr, info);
}

void RenderNode::createVertexData() {
    vk::BufferCreateInfo info = {};
    info.size = vertexData.size() * sizeof(Vertex);
    info.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    m_vertexBuffer = std::make_unique<SEngine::Buffer>(*m_engine, info, allocInfo);

    m_transferNode->transfer(*m_vertexBuffer, vertexData.size() * sizeof(Vertex), 0, vertexData.data());
}