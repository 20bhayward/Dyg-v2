#include "World.h"
#include <filesystem>
#include <iostream>
#include <thread>
#include <future>

namespace Engine {

World::World()
    : m_PlayerPosition(0.0f, 0.0f), m_Running(true) {
    const unsigned int threadCount = std::thread::hardware_concurrency();
    std::cout << "Initializing world with " << threadCount << " worker threads" << std::endl;
}

World::~World() {
    m_Running = false;
    
    // Wait for all worker threads to finish
    for (auto& thread : m_WorkerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    // Clear all chunks
    m_Chunks.clear();
}

void World::Update(float dt) {
    // Stream chunks based on player position
    StreamChunks();
    
    // Update chunks in parallel
    UpdateChunksMultiThreaded(dt);
}

void World::Render() {
    // Rendering will be implemented in the rendering module
    // This would iterate through visible chunks and render them
}

Chunk* World::GetChunk(const glm::ivec2& coord) {
    std::lock_guard<std::mutex> lock(m_ChunkMutex);
    
    auto it = m_Chunks.find(coord);
    if (it != m_Chunks.end()) {
        return it->second.get();
    }
    
    return nullptr;
}

Chunk* World::CreateChunk(const glm::ivec2& coord) {
    std::lock_guard<std::mutex> lock(m_ChunkMutex);
    
    // Check if chunk already exists
    auto it = m_Chunks.find(coord);
    if (it != m_Chunks.end()) {
        return it->second.get();
    }
    
    // Create a new chunk
    auto chunk = std::make_unique<Chunk>(coord);
    Chunk* result = chunk.get();
    m_Chunks[coord] = std::move(chunk);
    
    return result;
}

void World::DestroyChunk(const glm::ivec2& coord) {
    std::lock_guard<std::mutex> lock(m_ChunkMutex);
    
    auto it = m_Chunks.find(coord);
    if (it != m_Chunks.end()) {
        m_Chunks.erase(it);
    }
}

Particle& World::GetParticle(int worldX, int worldY) {
    glm::ivec2 chunkCoord = WorldToChunkCoord(worldX, worldY);
    glm::ivec2 localCoord = WorldToLocalCoord(worldX, worldY);
    
    Chunk* chunk = GetChunk(chunkCoord);
    if (!chunk) {
        // Create the chunk if it doesn't exist
        chunk = CreateChunk(chunkCoord);
    }
    
    return chunk->GetParticle(localCoord.x, localCoord.y);
}

const Particle World::GetParticle(int worldX, int worldY) const {
    glm::ivec2 chunkCoord = WorldToChunkCoord(worldX, worldY);
    glm::ivec2 localCoord = WorldToLocalCoord(worldX, worldY);
    
    // Since this is a const method, we can't create chunks if they don't exist
    // Instead, return an empty particle if the chunk doesn't exist
    
    // Check if chunk exists
    auto it = m_Chunks.find(chunkCoord);
    if (it != m_Chunks.end()) {
        // Get the particle from the chunk
        return it->second->GetParticle(localCoord.x, localCoord.y);
    }
    
    // Return empty particle if chunk doesn't exist
    return Particle();
}

void World::SetParticle(int worldX, int worldY, const Particle& particle) {
    glm::ivec2 chunkCoord = WorldToChunkCoord(worldX, worldY);
    glm::ivec2 localCoord = WorldToLocalCoord(worldX, worldY);
    
    Chunk* chunk = GetChunk(chunkCoord);
    if (!chunk) {
        // Create the chunk if it doesn't exist
        chunk = CreateChunk(chunkCoord);
    }
    
    chunk->SetParticle(localCoord.x, localCoord.y, particle);
}

void World::SetPlayerPosition(const glm::vec2& position) {
    m_PlayerPosition = position;
}

void World::Save(const std::string& directory) {
    std::filesystem::create_directories(directory);
    
    std::lock_guard<std::mutex> lock(m_ChunkMutex);
    
    for (const auto& [coord, chunk] : m_Chunks) {
        std::string filename = directory + "/chunk_" + 
            std::to_string(coord.x) + "_" + 
            std::to_string(coord.y) + ".bin";
        
        chunk->Save(filename);
    }
    
    std::cout << "Saved " << m_Chunks.size() << " chunks to " << directory << std::endl;
}

void World::Load(const std::string& directory) {
    if (!std::filesystem::exists(directory)) {
        std::cerr << "Directory does not exist: " << directory << std::endl;
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_ChunkMutex);
    
    // Clear existing chunks
    m_Chunks.clear();
    
    // Load all chunk files
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".bin") {
            std::string filename = entry.path().string();
            
            // Extract coordinates from filename
            // Assuming format: chunk_X_Y.bin
            std::string basename = entry.path().stem().string();
            size_t firstUnderscore = basename.find('_');
            size_t secondUnderscore = basename.find('_', firstUnderscore + 1);
            
            if (firstUnderscore != std::string::npos && secondUnderscore != std::string::npos) {
                std::string xStr = basename.substr(firstUnderscore + 1, secondUnderscore - firstUnderscore - 1);
                std::string yStr = basename.substr(secondUnderscore + 1);
                
                try {
                    int x = std::stoi(xStr);
                    int y = std::stoi(yStr);
                    
                    glm::ivec2 coord(x, y);
                    auto chunk = std::make_unique<Chunk>(coord);
                    
                    if (chunk->Load(filename)) {
                        m_Chunks[coord] = std::move(chunk);
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "Error parsing chunk filename: " << filename << " - " << e.what() << std::endl;
                }
            }
        }
    }
    
    std::cout << "Loaded " << m_Chunks.size() << " chunks from " << directory << std::endl;
}

void World::UpdateChunksAroundPlayer() {
    glm::ivec2 playerChunkCoord = WorldToChunkCoord(
        static_cast<int>(m_PlayerPosition.x),
        static_cast<int>(m_PlayerPosition.y)
    );
    
    // Create chunks in a square around the player
    for (int y = -m_ChunkLoadRadius; y <= m_ChunkLoadRadius; y++) {
        for (int x = -m_ChunkLoadRadius; x <= m_ChunkLoadRadius; x++) {
            glm::ivec2 chunkCoord = playerChunkCoord + glm::ivec2(x, y);
            
            // Check if chunk exists, create if it doesn't
            if (!GetChunk(chunkCoord)) {
                CreateChunk(chunkCoord);
            }
        }
    }
}

void World::StreamChunks() {
    glm::ivec2 playerChunkCoord = WorldToChunkCoord(
        static_cast<int>(m_PlayerPosition.x),
        static_cast<int>(m_PlayerPosition.y)
    );
    
    // First, create chunks around the player
    UpdateChunksAroundPlayer();
    
    // Then, remove chunks that are too far away
    std::vector<glm::ivec2> chunksToRemove;
    
    {
        std::lock_guard<std::mutex> lock(m_ChunkMutex);
        
        for (const auto& [coord, chunk] : m_Chunks) {
            int xDist = std::abs(coord.x - playerChunkCoord.x);
            int yDist = std::abs(coord.y - playerChunkCoord.y);
            
            if (xDist > m_ChunkLoadRadius + 2 || yDist > m_ChunkLoadRadius + 2) {
                chunksToRemove.push_back(coord);
            }
        }
    }
    
    // Remove chunks outside the radius
    for (const auto& coord : chunksToRemove) {
        DestroyChunk(coord);
    }
}

glm::ivec2 World::WorldToChunkCoord(int worldX, int worldY) const {
    const int chunkSize = Chunk::CHUNK_SIZE;
    
    // Floor division to get chunk coordinates
    int chunkX = worldX >= 0 ? worldX / chunkSize : (worldX - chunkSize + 1) / chunkSize;
    int chunkY = worldY >= 0 ? worldY / chunkSize : (worldY - chunkSize + 1) / chunkSize;
    
    return glm::ivec2(chunkX, chunkY);
}

glm::ivec2 World::WorldToLocalCoord(int worldX, int worldY) const {
    const int chunkSize = Chunk::CHUNK_SIZE;
    
    // Get local coordinates within the chunk
    int localX = worldX >= 0 ? worldX % chunkSize : (worldX % chunkSize + chunkSize) % chunkSize;
    int localY = worldY >= 0 ? worldY % chunkSize : (worldY % chunkSize + chunkSize) % chunkSize;
    
    return glm::ivec2(localX, localY);
}

void World::UpdateChunksMultiThreaded(float dt) {
    // Use a checkerboard pattern for updating to avoid race conditions
    // This divides the chunks into 4 groups that can be updated in parallel
    
    for (int phase = 0; phase < 4; phase++) {
        std::vector<std::future<void>> futures;
        
        std::lock_guard<std::mutex> lock(m_ChunkMutex);
        
        for (auto& [coord, chunk] : m_Chunks) {
            // Only update chunks that have changed
            if (!chunk->IsDirty())
                continue;
            
            // Check if this chunk belongs to the current phase
            bool evenX = (coord.x % 2) == 0;
            bool evenY = (coord.y % 2) == 0;
            
            int chunkPhase = 0;
            if (evenX && evenY) chunkPhase = 0;
            else if (!evenX && evenY) chunkPhase = 1;
            else if (evenX && !evenY) chunkPhase = 2;
            else if (!evenX && !evenY) chunkPhase = 3;
            
            if (chunkPhase == phase) {
                futures.push_back(std::async(std::launch::async, [&chunk, dt]() {
                    chunk->Update(dt);
                }));
            }
        }
        
        // Wait for all updates in this phase to complete
        for (auto& future : futures) {
            future.wait();
        }
    }
}

} // namespace Engine