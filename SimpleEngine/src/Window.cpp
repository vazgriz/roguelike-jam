#include "SimpleEngine/Window.h"

using namespace SEngine;

Window::Window(int32_t width, int32_t height, const std::string& title)
    : m_onResized(m_onResizedSignal), m_onFramebufferResized(m_onFramebufferResizedSignal) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = std::unique_ptr<GLFWwindow, WindowDeleter>(glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr));

    m_input = std::make_unique<Input>(m_window.get());
    m_resized = false;

    getSize(width, height);

    glfwSetWindowUserPointer(m_window.get(), this);
    glfwSetWindowSizeCallback(m_window.get(), &handleWindowResize);
    glfwSetKeyCallback(m_window.get(), &handleKeyInput);
    glfwSetMouseButtonCallback(m_window.get(), &handleMouseButtonInput);
    glfwSetCursorPosCallback(m_window.get(), &handleMousePosition);
}

void Window::handleKeyInput(GLFWwindow* windowptr, int key, int scancode, int action, int mods) {
    Window& window = *static_cast<Window*>(glfwGetWindowUserPointer(windowptr));
    window.m_input->handleKeyInput(key, scancode, action, mods);
}

void Window::handleMouseButtonInput(GLFWwindow* windowptr, int mouseButton, int action, int mods) {
    Window& window = *static_cast<Window*>(glfwGetWindowUserPointer(windowptr));
    window.m_input->handleMouseButtonInput(mouseButton, action, mods);
}

void Window::handleMousePosition(GLFWwindow* windowptr, double x, double y) {
    Window& window = *static_cast<Window*>(glfwGetWindowUserPointer(windowptr));
    window.m_input->handleMousePosition(x, y);
}

void Window::handleWindowResize(GLFWwindow* windowptr, int width, int height) {
    Window& window = *static_cast<Window*>(glfwGetWindowUserPointer(windowptr));
    window.getSize(width, height);
}

void Window::update() {
    m_input->preUpdate();

    if (minimized()) {
        glfwWaitEvents();
    } else {
        glfwPollEvents();
    }

    if (m_resized) {
        m_resized = false;

        m_onResizedSignal.publish(m_width, m_height);
        m_onFramebufferResizedSignal.publish(m_framebufferWidth, m_framebufferHeight);
    }
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window.get());
}

bool Window::minimized() const {
    return m_framebufferWidth == 0 || m_framebufferHeight == 0;
}

void Window::getSize(int width, int height) {
    m_width = width;
    m_height = height;
    m_resized = true;

    int framebufferwidth;
    int framebufferheight;
    glfwGetFramebufferSize(m_window.get(), &framebufferwidth, &framebufferheight);
    m_framebufferWidth = framebufferwidth;
    m_framebufferHeight = framebufferheight;
}

void Window::setTitle(const std::string& title) {
    glfwSetWindowTitle(m_window.get(), title.c_str());
}