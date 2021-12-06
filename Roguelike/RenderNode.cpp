#include "RenderNode.h"
#include <SimpleEngine/SimpleEngine.h>

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

    m_textureUsage = std::make_unique<SEngine::RenderGraph::ImageUsage>(
        *this, vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader
    );

    createRenderPass();
    createFramebuffers();
    createUniformBuffer();
    createSampler();
    createDescriptorLayout();
    createDescriptorPool();
    createDescriptor();
    createPipeline();

    m_swapchainConnection = engine.getGraphics().onSwapchainChanged().connect<&RenderNode::recreateResources>(this);
    m_uniform = {};
}

void RenderNode::setCamera(SEngine::Camera& camera) {
    m_camera = &camera;
}

void RenderNode::loadMap(Tiled::Tileset& tileset, Tiled::Map& map) {
    m_map = &map;

    SEngine::ImageAsset asset = SEngine::readImage("data/" + tileset.image);
    int32_t extendedImageWidth = tileset.imageWidth - (2 * tileset.margin) + tileset.spacing;
    int32_t extendedImageHeight = tileset.imageHeight - (2 * tileset.margin) + tileset.spacing;
    int32_t tileStepX = tileset.tileWidth + tileset.spacing;
    int32_t tileStepY = tileset.tileHeight + tileset.spacing;

    int32_t tileCountX = extendedImageWidth / tileStepX;
    int32_t tileCountY = extendedImageHeight / tileStepY;

    std::vector<vk::BufferImageCopy> copies;
    uint32_t tileIndex = 0;

    vk::Extent3D tileExtent = {
        static_cast<uint32_t>(tileset.tileWidth),
        static_cast<uint32_t>(tileset.tileHeight),
        1
    };

    for (int32_t y = 0; y < tileCountY; y++) {
        for (int32_t x = 0; x < tileCountX; x++) {
            glm::ivec2 tileOffset = {
                tileset.margin + x * tileStepX,
                tileset.margin + y * tileStepY
            };

            vk::BufferImageCopy copy = {};
            copy.imageExtent = tileExtent;
            copy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            copy.imageSubresource.baseArrayLayer = tileIndex;
            copy.imageSubresource.layerCount = 1;
            copy.imageSubresource.mipLevel = 0;
            copy.bufferOffset = (((size_t)tileset.imageWidth * tileOffset.y) + (tileOffset.x)) * SEngine::getFormatSize(vk::Format::eR8G8B8A8Srgb);

            copies.push_back(copy);
            tileIndex++;
        }
    }

    vk::Format format = vk::Format::eR8G8B8A8Srgb;

    vk::ImageCreateInfo info = {};
    info.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
    info.imageType = vk::ImageType::e2D;
    info.format = format;
    info.extent = tileExtent;
    info.arrayLayers = tileIndex;
    info.mipLevels = 1;
    info.samples = vk::SampleCountFlagBits::e1;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    auto& image = m_spritesheets.emplace_back(*m_engine, info, allocInfo);

    vk::Extent3D totalExtent = {
        static_cast<uint32_t>(tileset.imageWidth),
        static_cast<uint32_t>(tileset.imageHeight),
        1
    };

    m_transferNode->transfer(image, format, copies, totalExtent, asset.data());

    vk::ImageViewCreateInfo viewInfo = {};
    viewInfo.image = image.image();
    viewInfo.format = format;
    viewInfo.viewType = vk::ImageViewType::e2DArray;
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = tileIndex;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;

    m_spritesheetViews.emplace_back(m_graphics->device(), viewInfo);

    updateDescriptor();
    createVertexData();
    createVertexBuffer();
}

void RenderNode::preRender(uint32_t currentFrame) {
    if (m_camera != nullptr) {
        m_uniform.projectionMatrix = m_camera->projectionMatrix();
        m_uniform.viewMatrix = m_camera->viewMatrix();
    }

    m_transferNode->transfer(*m_uniformBuffer, sizeof(UniformData), 0, &m_uniform);

    for (auto& spritesheet : m_spritesheets) {
        vk::ImageSubresourceRange subresource = {};
        subresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        subresource.layerCount = spritesheet.arrayLayers();
        subresource.levelCount = 1;

        m_textureUsage->sync(spritesheet, subresource);
    }
}

void RenderNode::render(uint32_t currentFrame, vk::raii::CommandBuffer& commandBuffer) {
    if (m_vertexBuffer == nullptr) return;
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

    commandBuffer.draw(static_cast<uint32_t>(m_vertexData.size()), 1, 0, 0);

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

void RenderNode::createUniformBuffer() {
    vk::BufferCreateInfo info = {};
    info.size = sizeof(UniformData);
    info.usage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    m_uniformBuffer = std::make_unique<SEngine::Buffer>(*m_engine, info, allocInfo);
}

void RenderNode::createSampler() {
    vk::SamplerCreateInfo info = {};
    info.addressModeU = vk::SamplerAddressMode::eRepeat;
    info.addressModeV = vk::SamplerAddressMode::eRepeat;
    info.magFilter = vk::Filter::eNearest;
    info.minFilter = vk::Filter::eNearest;

    m_sampler = std::make_unique<vk::raii::Sampler>(m_graphics->device(), info);
}

void RenderNode::createDescriptorLayout() {
    vk::DescriptorSetLayoutBinding binding0 = {};
    binding0.descriptorType = vk::DescriptorType::eUniformBuffer;
    binding0.descriptorCount = 1;
    binding0.stageFlags = vk::ShaderStageFlagBits::eVertex;
    binding0.binding = 0;

    vk::DescriptorSetLayoutBinding binding1 = {};
    binding1.descriptorType = vk::DescriptorType::eSampler;
    binding1.descriptorCount = 1;
    binding1.stageFlags = vk::ShaderStageFlagBits::eFragment;
    binding1.binding = 1;

    vk::DescriptorSetLayoutBinding binding2 = {};
    binding2.descriptorType = vk::DescriptorType::eSampledImage;
    binding2.descriptorCount = 1;
    binding2.stageFlags = vk::ShaderStageFlagBits::eFragment;
    binding2.binding = 2;

    vk::DescriptorSetLayoutBinding bindings[] = { binding0, binding1, binding2 };

    vk::DescriptorSetLayoutCreateInfo info = {};
    info.bindingCount = 3;
    info.pBindings = bindings;

    m_descriptorLayout = std::make_unique<vk::raii::DescriptorSetLayout>(m_graphics->device(), info);
}

void RenderNode::createDescriptorPool() {
    vk::DescriptorPoolSize poolSize0 = {};
    poolSize0.descriptorCount = 1;
    poolSize0.type = vk::DescriptorType::eUniformBuffer;

    vk::DescriptorPoolSize poolSize1 = {};
    poolSize1.descriptorCount = 1;
    poolSize1.type = vk::DescriptorType::eSampler;

    vk::DescriptorPoolSize poolSize2 = {};
    poolSize2.descriptorCount = 1;
    poolSize2.type = vk::DescriptorType::eSampledImage;

    vk::DescriptorPoolSize poolSizes[] = { poolSize0, poolSize1, poolSize2 };

    vk::DescriptorPoolCreateInfo info = {};
    info.maxSets = 1;
    info.poolSizeCount = 3;
    info.pPoolSizes = poolSizes;

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

void RenderNode::updateDescriptor() {
    vk::DescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = m_uniformBuffer->buffer();
    bufferInfo.range = m_uniformBuffer->size();

    vk::WriteDescriptorSet write0 = {};
    write0.descriptorCount = 1;
    write0.descriptorType = vk::DescriptorType::eUniformBuffer;
    write0.dstSet = *m_descriptor;
    write0.dstBinding = 0;
    write0.pBufferInfo = &bufferInfo;

    vk::DescriptorImageInfo samplerInfo = {};
    samplerInfo.sampler = **m_sampler;

    vk::WriteDescriptorSet write1 = {};
    write1.descriptorCount = 1;
    write1.descriptorType = vk::DescriptorType::eSampler;
    write1.dstSet = *m_descriptor;
    write1.dstBinding = 1;
    write1.pImageInfo = &samplerInfo;

    vk::DescriptorImageInfo imageInfo = {};
    imageInfo.imageView = *m_spritesheetViews[0];
    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    vk::WriteDescriptorSet write2 = {};
    write2.descriptorCount = 1;
    write2.descriptorType = vk::DescriptorType::eSampledImage;
    write2.dstSet = *m_descriptor;
    write2.dstBinding = 2;
    write2.pImageInfo = &imageInfo;

    m_graphics->device().updateDescriptorSets({ write0, write1, write2 }, nullptr);
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
    attributeDescriptions[1].offset = offsetof(Vertex, uv);

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
    colorBlendAttachment.blendEnable = true;
    colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
    colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

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
    for (auto& layer : m_map->layers) {
        for (int32_t y = 0; y < layer.height; y++) {
            for (int32_t x = 0; x < layer.width; x++) {
                glm::vec3 offset = { x, y, 0 };

                int32_t index = (y * layer.width) + x;
                int32_t id = layer.data[index].id - 1;
                int32_t depth = layer.id;

                if (id < 0) {
                    continue;
                }

                Vertex v1 = {
                    glm::vec3{ 0, 0, depth } + offset,
                    glm::vec3{ 0, 0, id }
                };

                Vertex v2 = {
                    glm::vec3{ 1, 0, depth } + offset,
                    glm::vec3{ 1, 0, id }
                };

                Vertex v3 = {
                    glm::vec3{ 0, 1, depth } + offset,
                    glm::vec3{ 0, 1, id }
                };

                Vertex v4 = {
                    glm::vec3{ 1, 1, depth } + offset,
                    glm::vec3{ 1, 1, id }
                };

                m_vertexData.push_back(v1);
                m_vertexData.push_back(v2);
                m_vertexData.push_back(v3);

                m_vertexData.push_back(v2);
                m_vertexData.push_back(v4);
                m_vertexData.push_back(v3);
            }
        }
    }
}

void RenderNode::createVertexBuffer() {
    vk::BufferCreateInfo info = {};
    info.size = m_vertexData.size() * sizeof(Vertex);
    info.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    m_vertexBuffer = std::make_unique<SEngine::Buffer>(*m_engine, info, allocInfo);

    m_transferNode->transfer(*m_vertexBuffer, m_vertexData.size() * sizeof(Vertex), 0, m_vertexData.data());
}