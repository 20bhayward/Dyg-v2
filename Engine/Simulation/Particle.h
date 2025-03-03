#pragma once

#include <cstdint>

namespace Engine {

struct Particle {
    uint8_t materialID;    // Enum/index for material type
    float velocityX;       // Horizontal velocity
    float velocityY;       // Vertical velocity
    uint32_t lifetime;     // Optional lifetime counter for transient effects
    uint32_t flags;        // E.g., updated-this-frame flag, etc.
    
    Particle()
        : materialID(0), velocityX(0.0f), velocityY(0.0f), lifetime(0), flags(0) {}
    
    Particle(uint8_t material)
        : materialID(material), velocityX(0.0f), velocityY(0.0f), lifetime(0), flags(0) {}
    
    Particle(uint8_t material, float velX, float velY)
        : materialID(material), velocityX(velX), velocityY(velY), lifetime(0), flags(0) {}
        
    bool IsEmpty() const { return materialID == 0; }
};

} // namespace Engine