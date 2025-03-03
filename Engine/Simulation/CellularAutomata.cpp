#include "CellularAutomata.h"
#include "Material.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace Engine {

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

void CellularAutomata::UpdateParticle(Chunk& chunk, int x, int y, float dt) {
    Particle& particle = chunk.GetParticle(x, y);
    
    if (particle.IsEmpty())
        return; // No need to update empty cells
    
    // Apply material-specific update rules based on material ID
    switch (particle.materialID) {
        case 1: // Sand
        case 3: // Stone
        case 5: // Wood
            UpdateSand(chunk, x, y, dt);
            break;
        case 2: // Water
            UpdateWater(chunk, x, y, dt);
            break;
        case 4: // Fire
            UpdateFire(chunk, x, y, dt);
            break;
        case 6: // Gunpowder
            UpdateGunpowder(chunk, x, y, dt);
            break;
        case 7: // Acid
            UpdateAcid(chunk, x, y, dt);
            break;
        case 8: // Oil
            UpdateOil(chunk, x, y, dt);
            break;
        case 9: // Smoke
            UpdateSmoke(chunk, x, y, dt);
            break;
        case 10: // Salt
            UpdateSalt(chunk, x, y, dt);
            break;
        default:
            // For unknown materials, fall back to category-based behavior
            const Material& material = MaterialDatabase::Get().GetMaterial(particle.materialID);
            if (material.isSolid && !material.isLiquid && !material.isGas) {
                UpdateSand(chunk, x, y, dt);
            } else if (material.isLiquid) {
                UpdateWater(chunk, x, y, dt);
            } else if (material.isGas) {
                UpdateGas(chunk, x, y, dt);
            } else if (material.flammability > 0.0f) {
                UpdateFire(chunk, x, y, dt);
            }
            break;
    }
    
    // Apply general rules if the particle hasn't moved
    UpdateVelocity(particle, dt);
}

bool CellularAutomata::IsEmpty(const Chunk& chunk, int x, int y) {
    if (!IsInBounds(chunk, x, y))
        return false; // Treat out-of-bounds as non-empty
    
    return chunk.GetParticle(x, y).IsEmpty();
}

bool CellularAutomata::IsInBounds(const Chunk& chunk, int x, int y) {
    return chunk.IsInBounds(x, y);
}

bool CellularAutomata::MoveParticle(Chunk& chunk, int srcX, int srcY, int destX, int destY) {
    if (!IsInBounds(chunk, srcX, srcY) || !IsInBounds(chunk, destX, destY))
        return false;
    
    Particle& srcParticle = chunk.GetParticle(srcX, srcY);
    Particle& destParticle = chunk.GetParticle(destX, destY);
    
    if (!destParticle.IsEmpty())
        return false; // Can't move to a non-empty cell
    
    // Swap particles
    destParticle = srcParticle;
    srcParticle = Particle(); // Set source to empty
    
    // Mark both cells as dirty
    chunk.MarkDirty(srcX, srcY);
    chunk.MarkDirty(destX, destY);
    
    return true;
}

bool CellularAutomata::SwapParticles(Chunk& chunk, int x1, int y1, int x2, int y2) {
    if (!IsInBounds(chunk, x1, y1) || !IsInBounds(chunk, x2, y2))
        return false;
    
    // Get references to both particles
    Particle& p1 = chunk.GetParticle(x1, y1);
    Particle& p2 = chunk.GetParticle(x2, y2);
    
    // Swap the particles
    Particle temp = p1;
    p1 = p2;
    p2 = temp;
    
    // Mark both cells as dirty
    chunk.MarkDirty(x1, y1);
    chunk.MarkDirty(x2, y2);
    
    return true;
}

bool CellularAutomata::CanDissolveIn(uint8_t materialID, uint8_t solventID) {
    // Salt dissolves in water
    if (materialID == 10 && solventID == 2) return true;
    
    return false;
}

bool CellularAutomata::CanFloat(const Particle& floater, const Particle& liquid) {
    if (floater.IsEmpty() || liquid.IsEmpty())
        return false;
        
    const Material& floaterMaterial = MaterialDatabase::Get().GetMaterial(floater.materialID);
    const Material& liquidMaterial = MaterialDatabase::Get().GetMaterial(liquid.materialID);
    
    // Ensure second particle is a liquid
    if (!liquidMaterial.isLiquid)
        return false;
        
    // Check density - lower density materials float
    return floaterMaterial.density < liquidMaterial.density;
}

bool CellularAutomata::CanBurn(uint8_t materialID) {
    if (materialID == 0) return false; // Empty can't burn
    
    const Material& material = MaterialDatabase::Get().GetMaterial(materialID);
    return material.flammability > 0.0f;
}

void CellularAutomata::ApplyGravity(Particle& particle, float dt) {
    const float GRAVITY = 9.8f;
    particle.velocityY += GRAVITY * dt;
}

void CellularAutomata::UpdateVelocity(Particle& particle, float dt) {
    const Material& material = MaterialDatabase::Get().GetMaterial(particle.materialID);
    
    // Apply damping based on material viscosity
    float dampingFactor = 1.0f - material.viscosity * 0.5f;
    particle.velocityX *= dampingFactor;
    particle.velocityY *= dampingFactor;
    
    // Apply gravity
    if (!material.isGas) {
        ApplyGravity(particle, dt);
    } else {
        // Gases rise (negative gravity)
        particle.velocityY -= 3.0f * dt;
    }
}

void CellularAutomata::UpdateSand(Chunk& chunk, int x, int y, float dt) {
    // Basic falling behavior
    if (IsEmpty(chunk, x, y + 1)) {
        MoveParticle(chunk, x, y, x, y + 1);
    } else {
        // Try to fall diagonally
        bool movedLeft = false;
        bool movedRight = false;
        
        // Add a bit of randomness to make it more natural
        if (dist(gen) > 0.0f) {
            movedLeft = IsEmpty(chunk, x - 1, y + 1) && MoveParticle(chunk, x, y, x - 1, y + 1);
            if (!movedLeft) {
                movedRight = IsEmpty(chunk, x + 1, y + 1) && MoveParticle(chunk, x, y, x + 1, y + 1);
            }
        } else {
            movedRight = IsEmpty(chunk, x + 1, y + 1) && MoveParticle(chunk, x, y, x + 1, y + 1);
            if (!movedRight) {
                movedLeft = IsEmpty(chunk, x - 1, y + 1) && MoveParticle(chunk, x, y, x - 1, y + 1);
            }
        }
    }
}

void CellularAutomata::UpdateWater(Chunk& chunk, int x, int y, float dt) {
    // Try to fall first
    if (IsEmpty(chunk, x, y + 1)) {
        MoveParticle(chunk, x, y, x, y + 1);
        return;
    }
    
    // Try to spread horizontally
    const Material& material = MaterialDatabase::Get().GetMaterial(chunk.GetParticle(x, y).materialID);
    int spreadDistance = static_cast<int>(material.spreadFactor);
    
    bool moved = false;
    
    // Add some randomness to make it more natural
    if (dist(gen) > 0.0f) {
        // Try left first, then right
        for (int offset = 1; offset <= spreadDistance && !moved; offset++) {
            if (IsEmpty(chunk, x - offset, y)) {
                moved = MoveParticle(chunk, x, y, x - offset, y);
            }
        }
        
        if (!moved) {
            for (int offset = 1; offset <= spreadDistance && !moved; offset++) {
                if (IsEmpty(chunk, x + offset, y)) {
                    moved = MoveParticle(chunk, x, y, x + offset, y);
                }
            }
        }
    } else {
        // Try right first, then left
        for (int offset = 1; offset <= spreadDistance && !moved; offset++) {
            if (IsEmpty(chunk, x + offset, y)) {
                moved = MoveParticle(chunk, x, y, x + offset, y);
            }
        }
        
        if (!moved) {
            for (int offset = 1; offset <= spreadDistance && !moved; offset++) {
                if (IsEmpty(chunk, x - offset, y)) {
                    moved = MoveParticle(chunk, x, y, x - offset, y);
                }
            }
        }
    }
    
    // If it couldn't move, try to flow diagonally downward
    if (!moved) {
        if (IsEmpty(chunk, x - 1, y + 1)) {
            MoveParticle(chunk, x, y, x - 1, y + 1);
        } else if (IsEmpty(chunk, x + 1, y + 1)) {
            MoveParticle(chunk, x, y, x + 1, y + 1);
        }
    }
}

void CellularAutomata::UpdateFire(Chunk& chunk, int x, int y, float dt) {
    Particle& particle = chunk.GetParticle(x, y);
    
    // Decrease lifetime
    if (particle.lifetime > 0) {
        particle.lifetime--;
    } else {
        // Fire burns out - chance to create smoke
        if (dist(gen) < 0.6f) {
            Particle smoke(9); // Smoke material ID
            smoke.lifetime = 200 + static_cast<uint32_t>(dist(gen) * 150);
            chunk.SetParticle(x, y, smoke);
        } else {
            chunk.SetParticle(x, y, Particle()); // Just disappear
        }
        return;
    }
    
    // Fire changes color/intensity based on lifetime
    // This would be handled in the renderer
    
    // Random dancing of flames
    if (dist(gen) < 0.3f) {
        // Fire rises with some randomness
        if (IsEmpty(chunk, x, y - 1)) {
            MoveParticle(chunk, x, y, x, y - 1);
        } else if (IsEmpty(chunk, x - 1, y - 1) && dist(gen) > 0.0f) {
            MoveParticle(chunk, x, y, x - 1, y - 1);
        } else if (IsEmpty(chunk, x + 1, y - 1)) {
            MoveParticle(chunk, x, y, x + 1, y - 1);
        } else if (IsEmpty(chunk, x - 1, y)) {
            MoveParticle(chunk, x, y, x - 1, y);
        } else if (IsEmpty(chunk, x + 1, y)) {
            MoveParticle(chunk, x, y, x + 1, y);
        }
    }
    
    // Spread fire to nearby flammable materials
    int spreadAttempts = 0;
    for (int dy = -1; dy <= 1 && spreadAttempts < 2; dy++) {
        for (int dx = -1; dx <= 1 && spreadAttempts < 2; dx++) {
            if (dx == 0 && dy == 0)
                continue;
            
            if (!IsInBounds(chunk, x + dx, y + dy))
                continue;
            
            Particle& neighbor = chunk.GetParticle(x + dx, y + dy);
            if (neighbor.IsEmpty())
                continue;
            
            const Material& neighborMaterial = MaterialDatabase::Get().GetMaterial(neighbor.materialID);
            
            // Don't ignite fire or smoke
            if (neighbor.materialID == 4 || neighbor.materialID == 9)
                continue;
                
            // Check if this material is flammable
            if (neighborMaterial.flammability > 0.0f) {
                // Chance to ignite based on flammability and fire lifetime
                float igniteProbability = neighborMaterial.flammability * 0.15f;
                
                // Gunpowder has higher chance to ignite
                if (neighbor.materialID == 6) igniteProbability *= 2.0f;
                
                // Oil has higher chance to ignite
                if (neighbor.materialID == 8) igniteProbability *= 1.5f;
                
                if (dist(gen) < igniteProbability) {
                    // Determine fire lifetime based on material
                    uint32_t fireLifetime = 100 + static_cast<uint32_t>(dist(gen) * 50);
                    
                    // Wood burns longer
                    if (neighbor.materialID == 5) fireLifetime += 100;
                    
                    // Oil burns longer
                    if (neighbor.materialID == 8) fireLifetime += 50;
                    
                    // Create new fire particle
                    Particle fireParticle(4);
                    fireParticle.lifetime = fireLifetime;
                    chunk.SetParticle(x + dx, y + dy, fireParticle);
                    
                    spreadAttempts++; // Limit spread rate
                }
            }
        }
    }
    
    // Occasionally generate smoke above the fire
    if (dist(gen) < 0.05f && IsInBounds(chunk, x, y - 1)) {
        Particle& above = chunk.GetParticle(x, y - 1);
        if (above.IsEmpty()) {
            Particle smoke(9); // Smoke material ID
            smoke.lifetime = 250 + static_cast<uint32_t>(dist(gen) * 150);
            chunk.SetParticle(x, y - 1, smoke);
        }
    }
}

void CellularAutomata::UpdateGas(Chunk& chunk, int x, int y, float dt) {
    // Gas rises
    if (IsEmpty(chunk, x, y - 1)) {
        MoveParticle(chunk, x, y, x, y - 1);
    } else {
        // Spread horizontally and upward
        bool moved = false;
        
        if (dist(gen) > 0.0f) {
            // Try left-up, then right-up
            if (IsEmpty(chunk, x - 1, y - 1)) {
                moved = MoveParticle(chunk, x, y, x - 1, y - 1);
            } else if (IsEmpty(chunk, x + 1, y - 1)) {
                moved = MoveParticle(chunk, x, y, x + 1, y - 1);
            }
        } else {
            // Try right-up, then left-up
            if (IsEmpty(chunk, x + 1, y - 1)) {
                moved = MoveParticle(chunk, x, y, x + 1, y - 1);
            } else if (IsEmpty(chunk, x - 1, y - 1)) {
                moved = MoveParticle(chunk, x, y, x - 1, y - 1);
            }
        }
        
        // If didn't move diagonally up, try horizontal
        if (!moved) {
            if (dist(gen) > 0.0f) {
                if (IsEmpty(chunk, x - 1, y)) {
                    moved = MoveParticle(chunk, x, y, x - 1, y);
                } else if (IsEmpty(chunk, x + 1, y)) {
                    moved = MoveParticle(chunk, x, y, x + 1, y);
                }
            } else {
                if (IsEmpty(chunk, x + 1, y)) {
                    moved = MoveParticle(chunk, x, y, x + 1, y);
                } else if (IsEmpty(chunk, x - 1, y)) {
                    moved = MoveParticle(chunk, x, y, x - 1, y);
                }
            }
        }
    }
}

void CellularAutomata::UpdateGunpowder(Chunk& chunk, int x, int y, float dt) {
    // Gunpowder behaves like sand but has special explosion mechanics when near fire
    
    // Check for nearby fire first
    bool nearFire = false;
    for (int dy = -1; dy <= 1 && !nearFire; dy++) {
        for (int dx = -1; dx <= 1 && !nearFire; dx++) {
            if (dx == 0 && dy == 0) continue;
            
            if (!IsInBounds(chunk, x + dx, y + dy)) continue;
            
            Particle& neighbor = chunk.GetParticle(x + dx, y + dy);
            if (neighbor.materialID == 4) { // Fire
                nearFire = true;
            }
        }
    }
    
    if (nearFire) {
        // High chance to ignite
        if (dist(gen) < 0.8f) {
            // Turn into fire with a longer lifetime
            Particle fireParticle(4); // Fire material ID
            fireParticle.lifetime = 150 + static_cast<uint32_t>(dist(gen) * 50);
            chunk.SetParticle(x, y, fireParticle);
            
            // Create some additional fires or smoke in nearby cells
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    
                    if (!IsInBounds(chunk, x + dx, y + dy)) continue;
                    
                    Particle& neighbor = chunk.GetParticle(x + dx, y + dy);
                    if (neighbor.IsEmpty() && dist(gen) < 0.4f) {
                        // 50% chance for fire, 50% for smoke
                        if (dist(gen) > 0.0f) {
                            Particle smoke(9); // Smoke material ID
                            chunk.SetParticle(x + dx, y + dy, smoke);
                        } else {
                            Particle fire(4); // Fire material ID
                            fire.lifetime = 100 + static_cast<uint32_t>(dist(gen) * 50);
                            chunk.SetParticle(x + dx, y + dy, fire);
                        }
                    }
                }
            }
            
            return;
        }
    }
    
    // Otherwise, behave like sand
    UpdateSand(chunk, x, y, dt);
}

void CellularAutomata::UpdateAcid(Chunk& chunk, int x, int y, float dt) {
    // Acid behaves like water but has a chance to dissolve materials it contacts
    
    // Check neighboring cells for materials to dissolve
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            
            if (!IsInBounds(chunk, x + dx, y + dy)) continue;
            
            Particle& neighbor = chunk.GetParticle(x + dx, y + dy);
            if (neighbor.IsEmpty()) continue;
            
            // Check if this material can be corroded by acid
            const Material& acidMaterial = MaterialDatabase::Get().GetMaterial(7); // Acid
            const Material& targetMaterial = MaterialDatabase::Get().GetMaterial(neighbor.materialID);
            
            // Higher chance to dissolve less dense materials
            float dissolveProbability = acidMaterial.corrosiveness * (1.0f / targetMaterial.density) * 0.1f;
            
            // Stone is more resistant
            if (neighbor.materialID == 3) dissolveProbability *= 0.2f;
            
            // Fire and gases can't be dissolved
            if (neighbor.materialID == 4 || neighbor.materialID == 9) continue;
            
            if (dist(gen) < dissolveProbability) {
                // Dissolve the material
                neighbor = Particle(); // Set to empty
                chunk.MarkDirty(x + dx, y + dy);
            }
        }
    }
    
    // Otherwise, behave like water
    UpdateWater(chunk, x, y, dt);
}

void CellularAutomata::UpdateOil(Chunk& chunk, int x, int y, float dt) {
    // Oil behaves like water but floats on water and has a high chance to ignite
    
    // Check for nearby fire
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            
            if (!IsInBounds(chunk, x + dx, y + dy)) continue;
            
            Particle& neighbor = chunk.GetParticle(x + dx, y + dy);
            if (neighbor.materialID == 4) { // Fire
                // High chance to ignite
                if (dist(gen) < 0.7f) {
                    // Turn into fire with a medium lifetime
                    Particle fireParticle(4); // Fire material ID
                    fireParticle.lifetime = 120 + static_cast<uint32_t>(dist(gen) * 40);
                    chunk.SetParticle(x, y, fireParticle);
                    return;
                }
            }
        }
    }
    
    // Try to fall first
    if (IsEmpty(chunk, x, y + 1)) {
        MoveParticle(chunk, x, y, x, y + 1);
        return;
    }
    
    // Check if we're touching water, and if so, try to float
    bool touchingWater = false;
    
    // Check below for water
    if (IsInBounds(chunk, x, y + 1)) {
        Particle& below = chunk.GetParticle(x, y + 1);
        if (below.materialID == 2) { // Water
            touchingWater = true;
            // Try to swap places (float on top of water)
            SwapParticles(chunk, x, y, x, y + 1);
            return;
        }
    }
    
    // Check diagonally below for water
    if (!touchingWater) {
        // Left diagonal
        if (IsInBounds(chunk, x - 1, y + 1)) {
            Particle& diagLeft = chunk.GetParticle(x - 1, y + 1);
            if (diagLeft.materialID == 2) { // Water
                SwapParticles(chunk, x, y, x - 1, y + 1);
                return;
            }
        }
        
        // Right diagonal
        if (IsInBounds(chunk, x + 1, y + 1)) {
            Particle& diagRight = chunk.GetParticle(x + 1, y + 1);
            if (diagRight.materialID == 2) { // Water
                SwapParticles(chunk, x, y, x + 1, y + 1);
                return;
            }
        }
    }
    
    // Otherwise, behave like water (but with possibly different spread factor)
    const Material& material = MaterialDatabase::Get().GetMaterial(chunk.GetParticle(x, y).materialID);
    int spreadDistance = static_cast<int>(material.spreadFactor);
    
    bool moved = false;
    
    // Add some randomness to make it more natural
    if (dist(gen) > 0.0f) {
        // Try left first, then right
        for (int offset = 1; offset <= spreadDistance && !moved; offset++) {
            if (IsEmpty(chunk, x - offset, y)) {
                moved = MoveParticle(chunk, x, y, x - offset, y);
            }
        }
        
        if (!moved) {
            for (int offset = 1; offset <= spreadDistance && !moved; offset++) {
                if (IsEmpty(chunk, x + offset, y)) {
                    moved = MoveParticle(chunk, x, y, x + offset, y);
                }
            }
        }
    } else {
        // Try right first, then left
        for (int offset = 1; offset <= spreadDistance && !moved; offset++) {
            if (IsEmpty(chunk, x + offset, y)) {
                moved = MoveParticle(chunk, x, y, x + offset, y);
            }
        }
        
        if (!moved) {
            for (int offset = 1; offset <= spreadDistance && !moved; offset++) {
                if (IsEmpty(chunk, x - offset, y)) {
                    moved = MoveParticle(chunk, x, y, x - offset, y);
                }
            }
        }
    }
    
    // If it couldn't move, try to flow diagonally downward
    if (!moved) {
        if (IsEmpty(chunk, x - 1, y + 1)) {
            MoveParticle(chunk, x, y, x - 1, y + 1);
        } else if (IsEmpty(chunk, x + 1, y + 1)) {
            MoveParticle(chunk, x, y, x + 1, y + 1);
        }
    }
}

void CellularAutomata::UpdateSmoke(Chunk& chunk, int x, int y, float dt) {
    // Smoke behaves like a gas but with a limited lifetime
    Particle& particle = chunk.GetParticle(x, y);
    
    // Decrease lifetime
    if (particle.lifetime > 0) {
        particle.lifetime--;
    } else {
        // Default lifetime if not set
        particle.lifetime = 300 + static_cast<uint32_t>(dist(gen) * 200);
    }
    
    // Fade out and disappear when lifetime is low
    if (particle.lifetime < 30 && dist(gen) < 0.1f) {
        chunk.SetParticle(x, y, Particle());
        return;
    }
    
    // Otherwise, behave like gas
    UpdateGas(chunk, x, y, dt);
}

void CellularAutomata::UpdateSalt(Chunk& chunk, int x, int y, float dt) {
    // Salt behaves like sand but has a chance to dissolve in water
    
    // Check for nearby water
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            
            if (!IsInBounds(chunk, x + dx, y + dy)) continue;
            
            Particle& neighbor = chunk.GetParticle(x + dx, y + dy);
            if (neighbor.materialID == 2) { // Water
                // Chance to dissolve in water
                if (dist(gen) < 0.05f) {
                    // Dissolve the salt
                    chunk.SetParticle(x, y, Particle());
                    return;
                }
            }
        }
    }
    
    // Otherwise, behave like sand
    UpdateSand(chunk, x, y, dt);
}

} // namespace Engine