#pragma once

namespace SEngine {
class Window;
class Graphics;
class RenderGraph;

class Engine {
public:
    Engine();
    Engine(const Engine& other) = delete;
    Engine& operator = (const Engine& other) = delete;
    Engine(Engine&& other) = default;
    Engine& operator = (Engine&& other) = default;

    ~Engine();

    void setWindow(Window& window);
    Window& getWindow();

    void setGraphics(Graphics& graphics);
    Graphics& getGraphics();

    void setRenderGraph(RenderGraph& renderGraph);
    RenderGraph& getRenderGraph();

    void run();

private:
    Window* m_window;
    Graphics* m_graphics;
    RenderGraph* m_renderGraph;
    bool m_shouldExit;
};
}