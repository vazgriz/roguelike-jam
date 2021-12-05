#include "SimpleEngine/Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

using namespace SEngine;

Camera::Camera(float width, float height) {
    m_zoom = 1;
    setSize({ width, height });
}

void Camera::setPosition(glm::vec2 pos) {
    m_position = pos;

    m_viewMatrix = glm::translate(glm::vec3(m_position, 0));
}

void Camera::createProjectionMatrix() {
    m_projectionMatrix = glm::orthoRH_ZO(
        -m_width / 2 / m_zoom, m_width / 2 / m_zoom,
        m_height / 2 / m_zoom, -m_height / 2 / m_zoom,
        -10.0f, 10.0f);
    m_projectionMatrix[1][1] *= -1.0f;
}

void Camera::setSize(glm::vec2 size) {
    m_width = size.x;
    m_height = size.y;

    createProjectionMatrix();
}

void Camera::setZoom(float zoom) {
    m_zoom = zoom;

    createProjectionMatrix();
}