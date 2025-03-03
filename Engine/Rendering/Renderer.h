#pragma once

#include <memory>
#include <string>
#include <SDL2/SDL.h>

namespace Engine {

// Forward declarations
class World;
class VulkanRenderer;

enum class RendererType {
    Vulkan
};

// A clean, high-level interface to our rendering system
class Renderer {
public:
    Renderer(int screenWidth, int screenHeight, RendererType type = RendererType::Vulkan);
    ~Renderer();

    // Initialization and shutdown
    bool initialize(SDL_Window* window);
    void cleanup();
    
    // Core rendering methods
    void beginFrame();
    void endFrame();
    void renderWorld(const World& world, int cameraX = 0, int cameraY = 0, float zoomLevel = 1.0f);
    
    // Window event handling
    void handleResize(int width, int height);
    
    // Rendering settings
    void setClearColor(float r, float g, float b, float a = 1.0f);
    void setViewport(int x, int y, int width, int height);
    
    // Information methods
    std::string getRendererInfo() const;
    bool supportsFeature(const std::string& featureName) const;
    
    // Static helper to check for rendering system availability
    static bool isRendererAvailable(RendererType type);

private:
    int m_screenWidth;
    int m_screenHeight;
    RendererType m_rendererType;
    
    // Backend renderer implementation (Vulkan only for now)
    std::unique_ptr<VulkanRenderer> m_vulkanRenderer;
};

} // namespace Engine