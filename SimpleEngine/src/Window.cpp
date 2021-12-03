#include "SimpleEngine/Window.h"

using namespace SEngine;

Window::Window(int32_t width, int32_t height, const std::string& title)
    : m_onResized(m_onResizedSignal), m_onFramebufferResized(m_onFramebufferResizedSignal) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = std::unique_ptr<GLFWwindow, WindowDeleter>(glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr));

    getSize(width, height);

    glfwSetWindowUserPointer(m_window.get(), this);
    glfwSetWindowSizeCallback(m_window.get(), &handleWindowResize);
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window.get());
}

void Window::handleWindowResize(GLFWwindow* windowptr, int width, int height) {
    Window& window = *static_cast<Window*>(glfwGetWindowUserPointer(windowptr));
    window.getSize(width, height);

    window.m_onResizedSignal.publish(window.m_width, window.m_height);
    window.m_onFramebufferResizedSignal.publish(window.m_framebufferWidth, window.m_framebufferHeight);
}

void Window::getSize(int width, int height) {
    m_width = width;
    m_height = height;

    int framebufferwidth;
    int framebufferheight;
    glfwGetFramebufferSize(m_window.get(), &framebufferwidth, &framebufferheight);
    m_framebufferWidth = framebufferwidth;
    m_framebufferHeight = framebufferheight;
}

void Window::setTitle(const std::string& title) {
    glfwSetWindowTitle(m_window.get(), title.c_str());
}