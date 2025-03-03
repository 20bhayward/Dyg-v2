#include "Application.h"
#include <iostream>

namespace Engine {

Application::Application(const std::string& name)
    : m_Name(name), m_Running(false) {
    std::cout << "Initializing " << m_Name << "..." << std::endl;
    // Initialize subsystems here
}

Application::~Application() {
    std::cout << "Shutting down " << m_Name << "..." << std::endl;
    // Cleanup subsystems here
}

void Application::Run() {
    m_Running = true;
    
    std::cout << m_Name << " is running..." << std::endl;
    
    // Main loop
    while (m_Running) {
        // Process input
        
        // Update simulation
        
        // Render frame
        
        // Cap frame rate if needed
    }
}

void Application::Stop() {
    m_Running = false;
}

} // namespace Engine