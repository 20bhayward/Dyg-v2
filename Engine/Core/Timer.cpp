#include "Timer.h"

namespace Engine {

Timer::Timer() {
    Reset();
}

void Timer::Reset() {
    m_StartTime = std::chrono::high_resolution_clock::now();
}

float Timer::GetElapsedTime() const {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime - m_StartTime).count();
    return elapsedTime * 1e-9f; // Convert nanoseconds to seconds
}

float Timer::GetElapsedTimeMs() const {
    return GetElapsedTime() * 1000.0f;
}

} // namespace Engine