#include "SimpleEngine/Clock.h"


using namespace SEngine;

Clock::Clock() {
    m_time = 0;
    m_delta = 0;
}

void Clock::update(float currentTime) {
    m_delta = currentTime - m_time;
    m_time = currentTime;
}

float Clock::currentTime() {
    return m_time;
}

float Clock::deltaTime() {
    return m_delta;
}