#pragma once
#include <vk_mem_alloc.h>
#include <memory>
#include <vulkan/vulkan_raii.hpp>

namespace SEngine {
    class Engine;

    struct ImageState {
        Engine* engine;
        vk::raii::Image image;
        VmaAllocation allocation;
        vk::Extent3D extent;
        vk::Format format;

        ImageState(Engine* engine, const vk::ImageCreateInfo& info, vk::raii::Image&& image, VmaAllocation allocation);
        ImageState(const ImageState& other) = delete;
        ImageState& operator = (const ImageState& other) = delete;
        ImageState(ImageState&& other) noexcept;
        ImageState& operator = (ImageState&& other) noexcept;
        ~ImageState();
    };

    class Image {
    public:
        Image(Engine& engine, const vk::ImageCreateInfo& info, const VmaAllocationCreateInfo& allocInfo);
        ~Image();

        const vk::Image& image() const { return *m_imageState->image; }
        vk::Extent3D extent() const { return m_imageState->extent; }
        vk::Format format() const { return m_imageState->format; }

    private:
        Engine* m_engine;
        std::unique_ptr<ImageState> m_imageState;
        VmaAllocationInfo m_allocationInfo;
    };
}