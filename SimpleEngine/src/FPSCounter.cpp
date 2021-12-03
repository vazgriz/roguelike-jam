#include "SimpleEngine/FPSCounter.h"
#include "SimpleEngine/SimpleEngine.h"
#include <sstream>

#define FPS_INTERVAL 0.25f

using namespace SEngine;

FPSCounter::FPSCounter(Window& window, const std::string& title) {
    m_window = &window;
    m_title = title;
    m_lastTime = 0;
    m_frameCount = 0;
}

void FPSCounter::update(Clock& clock) {
    m_frameCount++;
    float now = clock.currentTime();
    float elapsed = now - m_lastTime;

    if (elapsed > FPS_INTERVAL) {
        std::stringstream stream;
        stream << m_title << " (" << std::round((float)m_frameCount / elapsed) << " fps)";
        m_window->setTitle(stream.str());
        m_lastTime = now;
        m_frameCount = 0;
    }
}