#include "Freecam.h"
#include <SimpleEngine/Camera.h>

Freecam::Freecam(SEngine::Window& window, SEngine::Camera& camera) {
    m_input = &window.input();
    m_camera = &camera;
    m_position = {};
    m_speed = 1;
}

void Freecam::update(SEngine::Clock& clock) {
    float x = 0;
    float y = 0;

    if (m_input->keyHold(SEngine::Key::A)) {
        x += 1;
    }

    if (m_input->keyHold(SEngine::Key::D)) {
        x -= 1;
    }

    if (m_input->keyHold(SEngine::Key::W)) {
        y += 1;
    }

    if (m_input->keyHold(SEngine::Key::S)) {
        y -= 1;
    }

    glm::vec2 dir = glm::vec2(x, y);
    if (dir.x != 0 || dir.y != 0) dir = glm::normalize(dir);
    m_position += dir * m_speed * clock.deltaTime();

    m_camera->setPosition(m_position);
}

void Freecam::setSpeed(float speed) {
    m_speed = speed;
}