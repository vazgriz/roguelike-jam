#pragma once

namespace SEngine {
class Clock {
public:
    Clock();
    Clock(const Clock& other) = delete;
    Clock& operator = (const Clock& other) = delete;
    Clock(Clock&& other) = delete;
    Clock& operator = (Clock&& other) = delete;

    void update(float currentTime);

    float deltaTime();
    float currentTime();

private:
    float m_time;
    float m_delta;
};
}