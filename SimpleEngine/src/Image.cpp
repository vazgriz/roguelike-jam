#include "SimpleEngine/Image.h"
#include "SimpleEngine/Engine.h"
#include "SimpleEngine/Graphics.h"
#include "SimpleEngine/MemoryManager.h"
#include "SimpleEngine/RenderGraph/RenderGraph.h"

using namespace SEngine;

ImageState::ImageState(Engine* engine, const vk::ImageCreateInfo& info, vk::raii::Image&& image, VmaAllocation allocation) : image(std::move(image)) {
    this->engine = engine;
    this->allocation = allocation;
    this->extent = info.extent;
    this->format = info.format;
}

ImageState::ImageState(ImageState&& other) noexcept : image(std::move(other.image)) {
    allocation = other.allocation;
    other.allocation = {};
    engine = other.engine;
    extent = other.extent;
}

ImageState& ImageState::operator = (ImageState&& other) noexcept {
    if (&other != this) {
        allocation = other.allocation;
        other.allocation = VK_NULL_HANDLE;
        engine = other.engine;
        extent = other.extent;
    }
    return *this;
}

ImageState::~ImageState() {
    vmaFreeMemory(engine->getGraphics().memory().allocator(), allocation);
}

Image::Image(Engine& engine, const vk::ImageCreateInfo& info, const VmaAllocationCreateInfo& allocInfo) {
    m_engine = &engine;

    VmaAllocator allocator = engine.getGraphics().memory().allocator();

    VkImage buffer;
    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;
    vmaCreateImage(allocator, &(VkImageCreateInfo)info, &allocInfo, &buffer, &allocation, &allocationInfo);

    m_imageState = std::make_unique<ImageState>(m_engine, info, vk::raii::Image(engine.getGraphics().device(), buffer), allocation);
    m_allocationInfo = allocationInfo;
}

Image::~Image() {
    m_engine->getRenderGraph().queueDestroy(std::move(*m_imageState));
}