#pragma once

namespace SEngine {
class Clock;

class ISystem {
public:
    ISystem();
    void setPriority(size_t priority);
    size_t getPriority() const;

    virtual void update(Clock& clock) = 0;

private:
    size_t m_priority;
};
}