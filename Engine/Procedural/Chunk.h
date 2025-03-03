#pragma once

#include <glm/glm.hpp>
#include "../Simulation/Particle.h"
#include <vector>
#include <memory>

namespace Engine {

struct Rect {
    int x, y, width, height;
    
    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(int _x, int _y, int _width, int _height) 
        : x(_x), y(_y), width(_width), height(_height) {}
        
    bool IsEmpty() const { return width <= 0 || height <= 0; }
    void Expand(int _x, int _y) {
        if (_x < x) {
            width += (x - _x);
            x = _x;
        } else if (_x >= x + width) {
            width = _x - x + 1;
        }
        
        if (_y < y) {
            height += (y - _y);
            y = _y;
        } else if (_y >= y + height) {
            height = _y - y + 1;
        }
    }
};

class Chunk {
public:
    static const int CHUNK_SIZE; // Size of one dimension of the chunk
    
    Chunk(const glm::ivec2& coord);
    ~Chunk();
    
    Particle& GetParticle(int x, int y);
    const Particle& GetParticle(int x, int y) const;
    void SetParticle(int x, int y, const Particle& particle);
    
    bool IsInBounds(int x, int y) const;
    
    void Update(float dt);
    void Render();
    
    void MarkDirty(int x, int y);
    void ClearDirty();
    bool IsDirty() const { return !m_DirtyRect.IsEmpty(); }
    const Rect& GetDirtyRect() const { return m_DirtyRect; }
    
    const glm::ivec2& GetCoord() const { return m_ChunkCoord; }
    
    void Save(const std::string& filename);
    bool Load(const std::string& filename);
    
private:
    glm::ivec2 m_ChunkCoord;          // Coordinates in world space
    std::vector<Particle> m_Grid;      // Flat array of particles
    Rect m_DirtyRect;                  // Bounding box of cells that changed
    bool m_Updated;                    // Flag to track if chunk was updated this frame
    
    // Helper methods for converting between 2D and 1D indices
    int FlattenIndex(int x, int y) const { return y * CHUNK_SIZE + x; }
};

} // namespace Engine