#include "Chunk.h"
#include "../Simulation/CellularAutomata.h"
#include <fstream>
#include <iostream>

namespace Engine {

// Initialize static member
const int Chunk::CHUNK_SIZE = 64;

Chunk::Chunk(const glm::ivec2& coord)
    : m_ChunkCoord(coord), m_Updated(false) {
    // Initialize the grid with empty particles
    m_Grid.resize(CHUNK_SIZE * CHUNK_SIZE);
    
    // Clear the dirty rect
    ClearDirty();
}

Chunk::~Chunk() {
    // Nothing to destroy yet
}

Particle& Chunk::GetParticle(int x, int y) {
    if (!IsInBounds(x, y)) {
        static Particle emptyParticle; // Return a default particle for out-of-bounds access
        return emptyParticle;
    }
    
    return m_Grid[FlattenIndex(x, y)];
}

const Particle& Chunk::GetParticle(int x, int y) const {
    if (!IsInBounds(x, y)) {
        static const Particle emptyParticle; // Return a default particle for out-of-bounds access
        return emptyParticle;
    }
    
    return m_Grid[FlattenIndex(x, y)];
}

void Chunk::SetParticle(int x, int y, const Particle& particle) {
    if (!IsInBounds(x, y))
        return;
    
    m_Grid[FlattenIndex(x, y)] = particle;
    MarkDirty(x, y);
}

bool Chunk::IsInBounds(int x, int y) const {
    return x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_SIZE;
}

void Chunk::Update(float dt) {
    if (m_DirtyRect.IsEmpty())
        return; // No need to update if nothing has changed
    
    // Use the dirty rect to optimize updates
    int startX = m_DirtyRect.x;
    int startY = m_DirtyRect.y;
    int endX = startX + m_DirtyRect.width;
    int endY = startY + m_DirtyRect.height;
    
    // Clamp to chunk bounds
    startX = std::max(0, startX);
    startY = std::max(0, startY);
    endX = std::min(CHUNK_SIZE, endX);
    endY = std::min(CHUNK_SIZE, endY);
    
    // Update particles in the dirty rect
    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            CellularAutomata::UpdateParticle(*this, x, y, dt);
        }
    }
    
    // Clear the dirty rect for the next frame
    ClearDirty();
}

void Chunk::Render() {
    // Rendering will be implemented in the rendering module
}

void Chunk::MarkDirty(int x, int y) {
    if (!IsInBounds(x, y))
        return;
    
    if (m_DirtyRect.IsEmpty()) {
        m_DirtyRect = Rect(x, y, 1, 1);
    } else {
        m_DirtyRect.Expand(x, y);
    }
}

void Chunk::ClearDirty() {
    m_DirtyRect = Rect(); // Reset to empty rect
}

void Chunk::Save(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for saving chunk: " << filename << std::endl;
        return;
    }
    
    // Write chunk coordinates
    file.write(reinterpret_cast<const char*>(&m_ChunkCoord.x), sizeof(int));
    file.write(reinterpret_cast<const char*>(&m_ChunkCoord.y), sizeof(int));
    
    // Write particle data
    for (const auto& particle : m_Grid) {
        file.write(reinterpret_cast<const char*>(&particle.materialID), sizeof(uint8_t));
        file.write(reinterpret_cast<const char*>(&particle.velocityX), sizeof(float));
        file.write(reinterpret_cast<const char*>(&particle.velocityY), sizeof(float));
        file.write(reinterpret_cast<const char*>(&particle.lifetime), sizeof(uint32_t));
        file.write(reinterpret_cast<const char*>(&particle.flags), sizeof(uint32_t));
    }
    
    file.close();
}

bool Chunk::Load(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for loading chunk: " << filename << std::endl;
        return false;
    }
    
    // Read chunk coordinates
    file.read(reinterpret_cast<char*>(&m_ChunkCoord.x), sizeof(int));
    file.read(reinterpret_cast<char*>(&m_ChunkCoord.y), sizeof(int));
    
    // Read particle data
    m_Grid.resize(CHUNK_SIZE * CHUNK_SIZE);
    for (auto& particle : m_Grid) {
        file.read(reinterpret_cast<char*>(&particle.materialID), sizeof(uint8_t));
        file.read(reinterpret_cast<char*>(&particle.velocityX), sizeof(float));
        file.read(reinterpret_cast<char*>(&particle.velocityY), sizeof(float));
        file.read(reinterpret_cast<char*>(&particle.lifetime), sizeof(uint32_t));
        file.read(reinterpret_cast<char*>(&particle.flags), sizeof(uint32_t));
    }
    
    // Mark the entire chunk as dirty
    m_DirtyRect = Rect(0, 0, CHUNK_SIZE, CHUNK_SIZE);
    
    file.close();
    return true;
}

} // namespace Engine