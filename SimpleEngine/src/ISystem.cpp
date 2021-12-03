#include "SimpleEngine/ISystem.h"

using namespace SEngine;

ISystem::ISystem() {
    m_priority = 0;
}

void ISystem::setPriority(size_t priority) {
    m_priority = priority;
}

size_t ISystem::getPriority() const {
    return m_priority;
}