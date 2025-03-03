#pragma once

#include <memory>
#include <string>

namespace Engine {

class Application {
public:
    Application(const std::string& name = "Sand Simulation Engine");
    ~Application();
    
    void Run();
    void Stop();
    
    bool IsRunning() const { return m_Running; }
    
private:
    std::string m_Name;
    bool m_Running = false;
};

} // namespace Engine