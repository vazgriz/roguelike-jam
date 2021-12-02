#pragma once
#include <optional>
#include <vulkan/vulkan_raii.hpp>
#include <entt/signal/sigh.hpp>

#include "MemoryManager.h"

namespace SEngine {
class Window;

struct QueueInfo {
    vk::raii::Device& device;
    vk::raii::Queue queue;
    uint32_t familyIndex;

    QueueInfo(vk::raii::Device& device, uint32_t familyIndex)
        : device(device), queue(device, familyIndex, 0) {
        this->familyIndex = familyIndex;
    }
};

class Graphics {
public:
    Graphics(Window& window, const std::string& appName);
    Graphics(const Graphics& other) = delete;
    Graphics& operator = (const Graphics& other) = delete;
    Graphics(Graphics&& other) = default;
    Graphics& operator = (Graphics&& other) = default;

    vk::raii::PhysicalDevice& physicalDevice() const { return *m_physicalDevice; }
    vk::raii::Device& device() const { return *m_device; }
    MemoryManager& memory() const { return *m_memoryManager; }

    vk::raii::SwapchainKHR* swapchain() const { return m_swapchain.get(); }
    const std::vector<vk::Image>& swapchainImages() const { return m_swapchainImages; }
    const std::vector<vk::raii::ImageView>& swapchainImageViews() const { return m_swapchainImageViews; }
    vk::Format swapchainFormat() const { return m_swapchainFormat; }
    vk::Extent2D swapchainExtent() const { return m_swapchainExtent; }

    QueueInfo& graphicsQueue() { return *m_graphicsQueue; }
    QueueInfo& presentQueue() { return *m_presentQueue; }
    QueueInfo& transferQueue() { return *m_transferQueue; }

    entt::sink<void(vk::raii::SwapchainKHR*)>& onSwapchainChanged() { return m_onSwapchainChanged; }

private:
    struct QueueFamilies {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;
        std::optional<uint32_t> transfer;

        bool valid() const;
    };

    Window* m_window;

    std::unique_ptr<vk::raii::Context> m_context;
    std::unique_ptr<vk::raii::Instance> m_instance;
    std::unique_ptr<vk::raii::SurfaceKHR> m_surface;
    std::unique_ptr<vk::raii::PhysicalDevice> m_physicalDevice;
    std::unique_ptr<vk::raii::Device> m_device;
    std::unique_ptr<vk::raii::SwapchainKHR> m_swapchain;

    std::unique_ptr<QueueInfo> m_graphicsQueue;
    std::unique_ptr<QueueInfo> m_presentQueue;
    std::unique_ptr<QueueInfo> m_transferQueue;

    std::unique_ptr<MemoryManager> m_memoryManager;
    vk::Format m_swapchainFormat;
    vk::Extent2D m_swapchainExtent;
    std::vector<vk::Image> m_swapchainImages;
    std::vector<vk::raii::ImageView> m_swapchainImageViews;

    entt::scoped_connection m_framebufferConnection;

    entt::sigh<void(vk::raii::SwapchainKHR*)> m_onSwapchainChangedSignal;
    entt::sink<void(vk::raii::SwapchainKHR*)> m_onSwapchainChanged;

    void createInstance(const std::string& appName);
    void createSurface();
    QueueFamilies evaluatePhysicalDevice(vk::raii::PhysicalDevice& device, bool requireDiscrete);
    void selectPhysicalDevice();
    void createDevice(vk::raii::PhysicalDevice& physicalDevice, QueueFamilies& queueFamilies);

    void recreateSwapchain(int32_t width, int32_t height);
    void createImageViews();
    void createSwapchain();

    vk::Format chooseFormat(std::vector<vk::SurfaceFormatKHR>& formats);
    vk::PresentModeKHR choosePresentMode(std::vector<vk::PresentModeKHR>& formats);
    vk::Extent2D chooseExtent(vk::SurfaceCapabilitiesKHR& capabilities);
};
}