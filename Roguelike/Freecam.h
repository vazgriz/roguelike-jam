#pragma once
#include <SimpleEngine/SimpleEngine.h>

class Freecam : public SEngine::ISystem {
public:
    Freecam(SEngine::Window& window, SEngine::Camera& camera);
    Freecam(const Freecam& other) = delete;
    Freecam& operator = (const Freecam& other) = delete;
    Freecam(Freecam&& other) = default;
    Freecam& operator = (Freecam&& other) = default;

    void update(SEngine::Clock& clock);

    void setSpeed(float speed);

private:
    SEngine::Input* m_input;
    SEngine::Camera* m_camera;
    glm::vec2 m_position;
    float m_speed;
};