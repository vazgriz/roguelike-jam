#pragma once
#include <glm/glm.hpp>

namespace SEngine {
class Camera {
public:
    Camera(float width, float height);
    Camera(const Camera& other) = delete;
    Camera& operator = (const Camera& other) = delete;
    Camera(Camera&& other) = delete;
    Camera& operator = (Camera&& other) = delete;

    float width() const { return m_width; }
    float height() const { return m_height; }
    glm::vec2 position() const { return m_position; }
    glm::mat4 projectionMatrix() const { return m_projectionMatrix; }
    glm::mat4 viewMatrix() const { return m_viewMatrix; }

    void setPosition(glm::vec2 pos);
    void setSize(glm::vec2 size);
    void setZoom(float zoom);

private:
    float m_width;
    float m_height;
    float m_zoom;
    glm::vec2 m_position;
    glm::mat4 m_projectionMatrix;
    glm::mat4 m_viewMatrix;

    void createProjectionMatrix();
};
}