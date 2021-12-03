#pragma once
#include <string>
#include "ISystem.h"

namespace SEngine {
class Clock;
class Window;

class FPSCounter : public ISystem {
public:
    FPSCounter(Window& window, const std::string& title);
    FPSCounter(const FPSCounter& other) = delete;
    FPSCounter& operator = (const FPSCounter& other) = delete;
    FPSCounter(FPSCounter&& other) = default;
    FPSCounter& operator = (FPSCounter&& other) = default;

    void update(Clock& clock) override;

private:
    Window* m_window;
    std::string m_title;
    size_t m_frameCount;
    float m_lastTime;
};
}