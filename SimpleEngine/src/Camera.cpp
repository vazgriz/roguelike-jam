#include "SimpleEngine/Camera.h"
#include <glm/gtc/matrix_transform.hpp>

using namespace SEngine;

Camera::Camera(float width, float height) {
    setSize({ width, height });
}

void Camera::setPosition(glm::vec2 pos) {
    m_position = pos;
}

void Camera::setSize(glm::vec2 size) {
    m_width = size.x;
    m_height = size.y;
    m_projection = glm::orthoRH_ZO(-m_width / 2, m_width / 2, m_height / 2, -m_height / 2, -10.0f, 10.0f);
    m_projection[1][1] *= -1.0f;
}