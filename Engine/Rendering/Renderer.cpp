#include "Renderer.h"
#include "VulkanRenderer.h"
#include "../Procedural/World.h"
#include <iostream>
#include <stdexcept>

namespace Engine {

Renderer::Renderer(int screenWidth, int screenHeight, RendererType type)
    : m_screenWidth(screenWidth)
    , m_screenHeight(screenHeight)
    , m_rendererType(type)
    , m_vulkanRenderer(nullptr) {
    
    std::cout << "Creating " << (type == RendererType::Vulkan ? "Vulkan" : "Unknown") 
              << " renderer with dimensions " << screenWidth << "x" << screenHeight << std::endl;
}

Renderer::~Renderer() {
    cleanup();
}

bool Renderer::initialize(SDL_Window* window) {
    try {
        // Select the appropriate rendering backend
        switch (m_rendererType) {
            case RendererType::Vulkan:
                if (!isRendererAvailable(RendererType::Vulkan)) {
                    std::cerr << "Vulkan is not available on this system" << std::endl;
                    return false;
                }
                
                std::cout << "Initializing Vulkan renderer..." << std::endl;
                m_vulkanRenderer = std::make_unique<VulkanRenderer>(m_screenWidth, m_screenHeight);
                
                if (!m_vulkanRenderer->initialize(window)) {
                    std::cerr << "Failed to initialize Vulkan renderer" << std::endl;
                    return false;
                }
                
                std::cout << "Vulkan renderer initialized successfully" << std::endl;
                return true;
                
            default:
                std::cerr << "Unsupported renderer type" << std::endl;
                return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error initializing renderer: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "Unknown error initializing renderer" << std::endl;
        return false;
    }
}

void Renderer::cleanup() {
    std::cout << "Cleaning up renderer..." << std::endl;
    
    try {
        if (m_vulkanRenderer) {
            std::cout << "Cleaning up Vulkan renderer..." << std::endl;
            m_vulkanRenderer->cleanup();
            m_vulkanRenderer.reset();
            std::cout << "Vulkan renderer cleanup complete" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error during renderer cleanup: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error during renderer cleanup" << std::endl;
    }
    
    std::cout << "Renderer cleanup finished" << std::endl;
}

void Renderer::beginFrame() {
    try {
        if (m_vulkanRenderer) {
            m_vulkanRenderer->beginFrame();
        } else {
            std::cerr << "Cannot begin frame - no renderer initialized" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error beginning frame: " << e.what() << std::endl;
    }
}

void Renderer::endFrame() {
    try {
        if (m_vulkanRenderer) {
            m_vulkanRenderer->endFrame();
        } else {
            std::cerr << "Cannot end frame - no renderer initialized" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error ending frame: " << e.what() << std::endl;
    }
}

void Renderer::renderWorld(const World& world, int cameraX, int cameraY, float zoomLevel) {
    try {
        if (m_vulkanRenderer) {
            m_vulkanRenderer->renderWorld(world, cameraX, cameraY, zoomLevel);
        } else {
            std::cerr << "Cannot render world - no renderer initialized" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error rendering world: " << e.what() << std::endl;
    }
}

void Renderer::handleResize(int width, int height) {
    m_screenWidth = width;
    m_screenHeight = height;
    
    std::cout << "Handling resize: " << width << "x" << height << std::endl;
    
    if (m_vulkanRenderer) {
        m_vulkanRenderer->handleResize(width, height);
    }
}

void Renderer::setClearColor(float r, float g, float b, float a) {
    if (m_vulkanRenderer) {
        m_vulkanRenderer->setClearColor(r, g, b, a);
    }
}

void Renderer::setViewport(int x, int y, int width, int height) {
    if (m_vulkanRenderer) {
        m_vulkanRenderer->setViewport(x, y, width, height);
    }
}

std::string Renderer::getRendererInfo() const {
    switch (m_rendererType) {
        case RendererType::Vulkan:
            return "Vulkan Renderer";
        default:
            return "Unknown Renderer";
    }
}

bool Renderer::supportsFeature(const std::string& featureName) const {
    // This is a simplified implementation - in a full implementation,
    // we would forward this to the specific renderer backend
    if (featureName == "vulkan" && m_rendererType == RendererType::Vulkan) {
        return true;
    }
    return false;
}

bool Renderer::isRendererAvailable(RendererType type) {
    switch (type) {
        case RendererType::Vulkan:
            return VulkanRenderer::isVulkanAvailable();
        default:
            return false;
    }
}

} // namespace Engine