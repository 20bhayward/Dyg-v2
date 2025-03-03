#pragma once

#include <chrono>

namespace Engine {

class Timer {
public:
    Timer();
    
    void Reset();
    
    // Returns elapsed time in seconds
    float GetElapsedTime() const;
    
    // Returns elapsed time in milliseconds
    float GetElapsedTimeMs() const;
    
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
};

} // namespace Engine