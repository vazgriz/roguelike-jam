#pragma once
#include <memory>
#include <vector>

namespace SEngine {
class Window;
class Graphics;
class RenderGraph;
class Clock;
class ISystem;

class Engine {
public:
    Engine();
    Engine(const Engine& other) = delete;
    Engine& operator = (const Engine& other) = delete;
    Engine(Engine&& other) = default;
    Engine& operator = (Engine&& other) = default;

    ~Engine();

    Clock& renderClock() const { return *m_renderClock; }

    void setWindow(Window& window);
    Window& getWindow();

    void setGraphics(Graphics& graphics);
    Graphics& getGraphics();

    void setRenderGraph(RenderGraph& renderGraph);
    RenderGraph& getRenderGraph();

    void addSystem(ISystem& system);

    void run();

private:
    Window* m_window;
    Graphics* m_graphics;
    RenderGraph* m_renderGraph;
    bool m_shouldExit;
    std::vector<ISystem*> m_renderSystems;

    std::unique_ptr<Clock> m_renderClock;
};
}