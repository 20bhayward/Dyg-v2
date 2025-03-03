#pragma once

#include "Chunk.h"
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <thread>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

namespace Engine {

class World {
public:
    World();
    ~World();
    
    void Update(float dt);
    void Render();
    
    Chunk* GetChunk(const glm::ivec2& coord);
    Chunk* CreateChunk(const glm::ivec2& coord);
    void DestroyChunk(const glm::ivec2& coord);
    
    Particle& GetParticle(int worldX, int worldY);
    const Particle GetParticle(int worldX, int worldY) const;
    void SetParticle(int worldX, int worldY, const Particle& particle);
    
    void SetPlayerPosition(const glm::vec2& position);
    
    void Save(const std::string& directory);
    void Load(const std::string& directory);
    
private:
    std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>> m_Chunks;
    std::mutex m_ChunkMutex;
    
    glm::vec2 m_PlayerPosition;
    const int m_ChunkLoadRadius = 3; // Number of chunks to load around player
    
    void UpdateChunksAroundPlayer();
    void StreamChunks();
    
    glm::ivec2 WorldToChunkCoord(int worldX, int worldY) const;
    glm::ivec2 WorldToLocalCoord(int worldX, int worldY) const;
    
    // Multi-threading related
    void UpdateChunksMultiThreaded(float dt);
    std::vector<std::thread> m_WorkerThreads;
    bool m_Running;
};

} // namespace Engine