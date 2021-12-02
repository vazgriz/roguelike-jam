#include "SimpleEngine/Engine.h"

#include <sstream>
#include <GLFW/glfw3.h>

#include "SimpleEngine/ErrorDialog.h"
#include "SimpleEngine/RenderGraph/RenderGraph.h"
#include "SimpleEngine/Window.h"
#include "SimpleEngine/Graphics.h"

using namespace SEngine;

static void handleGLFWError(int error_code, const char* description) {
    std::stringstream stream;
    stream << "GLFW ERROR [" << error_code << "]: " << description;

    ShowError(stream.str());

    exit(EXIT_FAILURE);
}

Engine::Engine() {
    m_window = nullptr;
    m_graphics = nullptr;
    m_renderGraph = nullptr;
    m_shouldExit = false;

    glfwSetErrorCallback(&handleGLFWError);

    glfwInit();
}

Engine::~Engine() {
    glfwTerminate();
}

void Engine::setWindow(Window& window) {
    m_window = &window;
}

Window& Engine::getWindow() {
    return *m_window;
}

void Engine::setGraphics(Graphics& graphics) {
    m_graphics = &graphics;
}

Graphics& Engine::getGraphics() {
    return *m_graphics;
}

void Engine::setRenderGraph(RenderGraph& renderGraph) {
    m_renderGraph = &renderGraph;
}

RenderGraph& Engine::getRenderGraph() {
    return *m_renderGraph;
}

void Engine::run() {
    if (m_window == nullptr) throw std::runtime_error("Window not set");
    if (m_graphics == nullptr) throw std::runtime_error("Graphics context not set");
    if (m_renderGraph == nullptr) throw std::runtime_error("Render graph not set");
    if (!m_renderGraph->isBaked()) throw std::runtime_error("Render graph not baked");

    bool shouldRender = true;

    while (true) {
        if (shouldRender) {
            glfwPollEvents();
        } else {
            glfwWaitEvents();
        }

        if (m_window->shouldClose()) {
            break;
        }

        if (m_graphics->swapchain() != nullptr) {
            shouldRender = true;
            m_renderGraph->execute();
        } else {
            shouldRender = false;
        }
    }

    m_graphics->device().waitIdle();
}