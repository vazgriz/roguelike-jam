#pragma once
#include <entt/entt.hpp>

namespace SEngine {
class Scene {
public:
    Scene();
    Scene(const Scene& other) = delete;
    Scene& operator = (const Scene& other) = delete;
    Scene(Scene&& other) = default;
    Scene& operator = (Scene&& other) = default;

private:
    entt::registry m_registry;
};
}