#include "SimpleEngine/Graphics.h"

#include <iostream>
#include <unordered_set>
#include <GLFW/glfw3.h>

#include "SimpleEngine/Window.h"

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

using namespace SEngine;

bool Graphics::QueueFamilies::valid() const {
    return graphics.has_value() && present.has_value() && transfer.has_value();
}

Graphics::Graphics(Window& window, const std::string& appName)
    : m_onSwapchainChanged(m_onSwapchainChangedSignal) {
    m_window = &window;

    createInstance(appName);
    createSurface();
    selectPhysicalDevice();
    createSwapchain();
    createImageViews();

    m_memoryManager = std::make_unique<MemoryManager>(**m_physicalDevice, **m_device);

    m_framebufferConnection = window.onFramebufferResized().connect<&Graphics::recreateSwapchain>(this);
}

void Graphics::createInstance(const std::string& appName) {
    m_context = std::make_unique<vk::raii::Context>();

    vk::ApplicationInfo appInfo = {};
    appInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 2, 0);
    appInfo.pApplicationName = appName.c_str();

    std::vector<const char*> extensions = {

    };

    uint32_t requiredCount;
    auto requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredCount);

    for (uint32_t i = 0; i < requiredCount; i++) {
        extensions.push_back(requiredExtensions[i]);
    }

    std::vector<const char*> layers = {
#ifndef NDEBUG
        "VK_LAYER_KHRONOS_validation"
#endif
    };

    vk::InstanceCreateInfo info = {};
    info.pApplicationInfo = &appInfo;
    info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    info.ppEnabledExtensionNames = extensions.data();
    info.enabledLayerCount = static_cast<uint32_t>(layers.size());
    info.ppEnabledLayerNames = layers.data();

    m_instance = std::make_unique<vk::raii::Instance>(*m_context, info);
}

void Graphics::createSurface() {
    VkSurfaceKHR surfaceVK;
    glfwCreateWindowSurface(**m_instance, m_window->handle(), nullptr, &surfaceVK);
    m_surface = std::make_unique<vk::raii::SurfaceKHR>(*m_instance, surfaceVK);
}

Graphics::QueueFamilies Graphics::evaluatePhysicalDevice(vk::raii::PhysicalDevice& device, bool requireDiscrete) {
    QueueFamilies families = {};

    //check if discrete
    vk::PhysicalDeviceProperties properties = device.getProperties();
    if (requireDiscrete && !(properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)) {
        return families;
    }

    //check if required extensions are available
    auto requiredExtensions = std::unordered_set<std::string>(deviceExtensions.begin(), deviceExtensions.end());

    for (auto& extension : device.enumerateDeviceExtensionProperties()) {
        requiredExtensions.erase(std::string(extension.extensionName.data()));
    }

    if (requiredExtensions.size() > 0) {
        return families;
    }

    //check if the required queue families are available
    std::vector<vk::QueueFamilyProperties> queueProperties = device.getQueueFamilyProperties();

    for (uint32_t i = 0; i < queueProperties.size(); i++) {
        auto& prop = queueProperties[i];

        if (!families.graphics.has_value() && prop.queueFlags & vk::QueueFlagBits::eGraphics) {
            families.graphics = i;
        }

        if (!families.present.has_value() && device.getSurfaceSupportKHR(i, **m_surface)) {
            families.present = i;
        }

        if (!families.transfer.has_value() && (prop.queueFlags & vk::QueueFlagBits::eTransfer) && !(prop.queueFlags & vk::QueueFlagBits::eGraphics)) {
            families.transfer = i;
        }
    }

    //if dedicated transfer queue not found, use graphics queue
    if (!families.transfer.has_value() && families.graphics.has_value()) {
        families.transfer = *families.graphics;
    }

    return families;
}

void Graphics::selectPhysicalDevice() {
    vk::raii::PhysicalDevices physicalDevices(*m_instance);

    //check for dedicated GPUs
    for (auto& device : physicalDevices) {
        QueueFamilies families = evaluatePhysicalDevice(device, true);

        if (families.valid()) {
            createDevice(device, families);
            return;
        }
    }

    //check for dedicated or integrated GPUs
    for (auto& device : physicalDevices) {
        QueueFamilies families = evaluatePhysicalDevice(device, false);

        if (families.valid()) {
            createDevice(device, families);
            return;
        }
    }

    throw std::runtime_error("Could not find usable Physical Device");
}

void Graphics::createDevice(vk::raii::PhysicalDevice& physicalDevice, QueueFamilies& queueFamilies) {
    m_physicalDevice = std::make_unique<vk::raii::PhysicalDevice>(std::move(physicalDevice));

    std::cout << "Device selected: " << m_physicalDevice->getProperties().deviceName.data() << "\n";

    std::unordered_set<uint32_t> uniqueFamilies = {
        *queueFamilies.graphics,
        *queueFamilies.present,
        *queueFamilies.transfer,
    };

    float priority = 1.0f;

    std::vector<vk::DeviceQueueCreateInfo> queueInfos = {};
    for (uint32_t index : uniqueFamilies) {
        vk::DeviceQueueCreateInfo queueInfo = {};
        queueInfo.queueFamilyIndex = index;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &priority;

        queueInfos.push_back(queueInfo);
    }

    vk::PhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures = {};
    timelineSemaphoreFeatures.timelineSemaphore = true;

    vk::PhysicalDeviceFeatures2 features = {};
    features.pNext = &timelineSemaphoreFeatures;

    vk::DeviceCreateInfo info = {};
    info.pNext = &features;
    info.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    info.ppEnabledExtensionNames = deviceExtensions.data();
    info.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
    info.pQueueCreateInfos = queueInfos.data();

    m_device = std::make_unique<vk::raii::Device>(*m_physicalDevice, info);

    m_graphicsQueue = std::make_unique<QueueInfo>(*m_device, *queueFamilies.graphics);
    m_presentQueue = std::make_unique<QueueInfo>(*m_device, *queueFamilies.present);
    m_transferQueue = std::make_unique<QueueInfo>(*m_device, *queueFamilies.transfer);
}

vk::Format Graphics::chooseFormat(std::vector<vk::SurfaceFormatKHR>& formats) {
    for (const auto& format : formats) {
        if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return format.format;
        }
    }

    return formats[0].format;
}

vk::PresentModeKHR Graphics::choosePresentMode(std::vector<vk::PresentModeKHR>& mdoes) {
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Graphics::chooseExtent(vk::SurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    else {
        int32_t width = m_window->width();
        int32_t height = m_window->height();

        vk::Extent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void Graphics::recreateSwapchain(int32_t width, int32_t height) {
    m_device->waitIdle();
    createSwapchain();
    createImageViews();
    m_onSwapchainChangedSignal.publish(m_swapchain.get());
}

void Graphics::createSwapchain() {
    auto formats = m_physicalDevice->getSurfaceFormatsKHR(**m_surface);
    auto presentModes = m_physicalDevice->getSurfacePresentModesKHR(**m_surface);
    auto capabilities = m_physicalDevice->getSurfaceCapabilitiesKHR(**m_surface);

    if (capabilities.currentExtent.width == 0 || capabilities.currentExtent.height == 0) {
        //cannot create a swapchain
        m_swapchain = nullptr;
        m_swapchainFormat = vk::Format::eUndefined;
        m_swapchainExtent = vk::Extent2D{};
        return;
    }

    m_swapchainFormat = chooseFormat(formats);
    auto presentMode = choosePresentMode(presentModes);
    m_swapchainExtent = chooseExtent(capabilities);
    uint32_t max = capabilities.maxImageCount > 0 ? capabilities.maxImageCount : std::numeric_limits<uint32_t>::max();
    uint32_t imageCount = std::clamp<uint32_t>(2, capabilities.minImageCount, max);

    vk::SwapchainCreateInfoKHR info = {};
    info.surface = **m_surface;
    info.minImageCount = imageCount;
    info.imageFormat = m_swapchainFormat;
    info.presentMode = presentMode;
    info.imageExtent = m_swapchainExtent;
    info.imageArrayLayers = 1;
    info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    uint32_t indices[] = { m_graphicsQueue->familyIndex,  m_presentQueue->familyIndex };
    if (m_graphicsQueue->familyIndex == m_presentQueue->familyIndex) {
        info.imageSharingMode = vk::SharingMode::eExclusive;
    } else {
        info.imageSharingMode = vk::SharingMode::eConcurrent;
        info.queueFamilyIndexCount = 2;
        info.pQueueFamilyIndices = indices;
    }

    info.preTransform = capabilities.currentTransform;
    info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    info.clipped = true;
    info.oldSwapchain = m_swapchain != nullptr ? **m_swapchain : VK_NULL_HANDLE;

    m_swapchain = std::make_unique<vk::raii::SwapchainKHR>(*m_device, info);

    auto images = m_swapchain->getImages();
    m_swapchainImages.clear();

    for (auto image : images) {
        m_swapchainImages.push_back(image);
    }
}

void Graphics::createImageViews() {
    m_swapchainImageViews.clear();

    if (m_swapchain == nullptr) return;

    for (auto image : m_swapchainImages) {
        vk::ImageViewCreateInfo info = {};
        info.image = image;
        info.format = m_swapchainFormat;
        info.viewType = vk::ImageViewType::e2D;
        info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        info.subresourceRange.layerCount = 1;
        info.subresourceRange.levelCount = 1;

        m_swapchainImageViews.emplace_back(*m_device, info);
    }
}