#pragma once

#include "Particle.h"
#include "../Procedural/Chunk.h"

namespace Engine {

class CellularAutomata {
public:
    static void UpdateParticle(Chunk& chunk, int x, int y, float dt);
    
    static bool IsEmpty(const Chunk& chunk, int x, int y);
    static bool IsInBounds(const Chunk& chunk, int x, int y);
    static bool MoveParticle(Chunk& chunk, int srcX, int srcY, int destX, int destY);
    static bool SwapParticles(Chunk& chunk, int x1, int y1, int x2, int y2);
    
    static void ApplyGravity(Particle& particle, float dt);
    static void UpdateVelocity(Particle& particle, float dt);
    
    // Special update rules for different material types
    static void UpdateSand(Chunk& chunk, int x, int y, float dt);
    static void UpdateWater(Chunk& chunk, int x, int y, float dt);
    static void UpdateFire(Chunk& chunk, int x, int y, float dt);
    static void UpdateGas(Chunk& chunk, int x, int y, float dt);
    
    // New material update functions
    static void UpdateGunpowder(Chunk& chunk, int x, int y, float dt);
    static void UpdateAcid(Chunk& chunk, int x, int y, float dt);
    static void UpdateOil(Chunk& chunk, int x, int y, float dt);
    static void UpdateSmoke(Chunk& chunk, int x, int y, float dt);
    static void UpdateSalt(Chunk& chunk, int x, int y, float dt);
    
    // Helper functions for material interactions
    static bool CanDissolveIn(uint8_t materialID, uint8_t solventID);
    static bool CanFloat(const Particle& floater, const Particle& liquid);
    static bool CanBurn(uint8_t materialID);
};

} // namespace Engine