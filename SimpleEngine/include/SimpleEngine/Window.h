#pragma once

#include <string>
#include <memory>
#include <GLFW/glfw3.h>
#include <entt/signal/sigh.hpp>

namespace SEngine {
class Window {
public:
    Window(int32_t width, int32_t height, const std::string& title);
    Window(const Window& other) = delete;
    Window& operator = (const Window& other) = delete;
    Window(Window&& other) = default;
    Window& operator = (Window&& other) = default;

    GLFWwindow* handle() const { return m_window.get(); }

    int32_t width() const { return m_width; }
    int32_t height() const { return m_height; }
    bool shouldClose() const;

    entt::sink<void(int32_t, int32_t)>& onResized() { return m_onResized; }
    entt::sink<void(int32_t, int32_t)>& onFramebufferResized() { return m_onFramebufferResized; }

private:
    struct WindowDeleter {
        void operator () (GLFWwindow* window) {
            glfwDestroyWindow(window);
        }
    };

    std::unique_ptr<GLFWwindow, WindowDeleter> m_window;
    int32_t m_width;
    int32_t m_height;
    int32_t m_framebufferWidth;
    int32_t m_framebufferHeight;

    entt::sigh<void(int32_t, int32_t)> m_onResizedSignal;
    entt::sink<void(int32_t, int32_t)> m_onResized;

    entt::sigh<void(int32_t, int32_t)> m_onFramebufferResizedSignal;
    entt::sink<void(int32_t, int32_t)> m_onFramebufferResized;

    static void handleWindowResize(GLFWwindow* window, int width, int height);
    void getSize(int width, int height);
};
}