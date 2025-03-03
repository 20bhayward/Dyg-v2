#include "ProceduralGenerator.h"
#include "../Simulation/Material.h"
#include <cmath>
#include <algorithm>

namespace Engine {

ProceduralGenerator::ProceduralGenerator(uint32_t seed)
    : m_Seed(seed) {
    m_Random.seed(seed);
}

ProceduralGenerator::~ProceduralGenerator() {
}

void ProceduralGenerator::GenerateChunk(Chunk* chunk) {
    if (!chunk)
        return;
    
    // Choose a generation method based on chunk position
    glm::ivec2 coord = chunk->GetCoord();
    
    // Different areas can have different terrain types
    if (coord.x < -5 || coord.x > 5) {
        GenerateCaves(chunk);
    } else if (coord.y < -3) {
        GenerateFlat(chunk);
    } else {
        GenerateTerrain(chunk);
    }
}

void ProceduralGenerator::GenerateFlat(Chunk* chunk) {
    if (!chunk)
        return;

    const int GROUND_LEVEL = Chunk::CHUNK_SIZE / 2 + 10;
    
    for (int y = 0; y < Chunk::CHUNK_SIZE; y++) {
        for (int x = 0; x < Chunk::CHUNK_SIZE; x++) {
            if (y > GROUND_LEVEL) {
                chunk->SetParticle(x, y, Particle(3)); // Stone (ID 3)
            }
        }
    }
}

void ProceduralGenerator::GenerateTerrain(Chunk* chunk) {
    if (!chunk)
        return;
    
    glm::ivec2 chunkCoord = chunk->GetCoord();
    const float SCALE = 0.03f;
    const int BASE_HEIGHT = Chunk::CHUNK_SIZE / 2;
    
    for (int x = 0; x < Chunk::CHUNK_SIZE; x++) {
        // Generate terrain height using Perlin noise
        float worldX = x + chunkCoord.x * Chunk::CHUNK_SIZE;
        float noiseValue = Perlin(worldX * SCALE, chunkCoord.y * SCALE, 0.5f, 4);
        int height = BASE_HEIGHT + static_cast<int>(noiseValue * 20.0f);
        
        // Cap height to chunk size
        height = std::min(height, Chunk::CHUNK_SIZE - 1);
        
        // Fill everything below the height with terrain
        for (int y = height; y < Chunk::CHUNK_SIZE; y++) {
            if (y == height) {
                chunk->SetParticle(x, y, Particle(1)); // Sand (ID 1) on top
            } else if (y < height + 5) {
                // Add some randomness to make it look more natural
                if (m_Random() % 10 < 8) {
                    chunk->SetParticle(x, y, Particle(1)); // Sand (ID 1)
                } else {
                    chunk->SetParticle(x, y, Particle(3)); // Stone (ID 3)
                }
            } else {
                chunk->SetParticle(x, y, Particle(3)); // Stone (ID 3)
            }
        }
        
        // Add some water in depressions
        if (height > BASE_HEIGHT + 5) {
            int waterLevel = BASE_HEIGHT + 3;
            for (int y = waterLevel; y < height; y++) {
                if (chunk->GetParticle(x, y).IsEmpty()) {
                    chunk->SetParticle(x, y, Particle(2)); // Water (ID 2)
                }
            }
        }
    }
    
    // Add some random features
    for (int i = 0; i < 10; i++) {
        int x = m_Random() % Chunk::CHUNK_SIZE;
        int y = m_Random() % (Chunk::CHUNK_SIZE / 2);
        
        // Add a small deposit of wood
        if (chunk->GetParticle(x, y).IsEmpty()) {
            chunk->SetParticle(x, y, Particle(5)); // Wood (ID 5)
            
            // Add a few more wood blocks nearby
            for (int j = 0; j < 3; j++) {
                int offsetX = (m_Random() % 5) - 2;
                int offsetY = (m_Random() % 5) - 2;
                
                int nx = x + offsetX;
                int ny = y + offsetY;
                
                if (chunk->IsInBounds(nx, ny) && chunk->GetParticle(nx, ny).IsEmpty()) {
                    chunk->SetParticle(nx, ny, Particle(5)); // Wood (ID 5)
                }
            }
        }
    }
}

void ProceduralGenerator::GenerateCaves(Chunk* chunk) {
    if (!chunk)
        return;
    
    glm::ivec2 chunkCoord = chunk->GetCoord();
    const float SCALE = 0.05f;
    
    for (int y = 0; y < Chunk::CHUNK_SIZE; y++) {
        for (int x = 0; x < Chunk::CHUNK_SIZE; x++) {
            float worldX = x + chunkCoord.x * Chunk::CHUNK_SIZE;
            float worldY = y + chunkCoord.y * Chunk::CHUNK_SIZE;
            
            float noiseValue = Perlin(worldX * SCALE, worldY * SCALE, 0.5f, 4);
            
            if (noiseValue > 0.3f) {
                chunk->SetParticle(x, y, Particle(3)); // Stone (ID 3)
            } else if (noiseValue > 0.2f) {
                if (m_Random() % 10 < 8) {
                    chunk->SetParticle(x, y, Particle(3)); // Stone (ID 3)
                } else {
                    chunk->SetParticle(x, y, Particle(1)); // Sand (ID 1)
                }
            } else if (noiseValue > 0.0f && m_Random() % 20 == 0) {
                chunk->SetParticle(x, y, Particle(2)); // Some water pools (ID 2)
            }
        }
    }
}

float ProceduralGenerator::Noise(float x, float y) {
    int n = static_cast<int>(x) + static_cast<int>(y) * 57;
    n = (n << 13) ^ n;
    
    float noise = (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
    return 0.5f * (noise + 1.0f); // Map to range [0,1]
}

float ProceduralGenerator::SmoothNoise(float x, float y) {
    // Get the four corners and the center
    float corners = (Noise(x - 1, y - 1) + Noise(x + 1, y - 1) + Noise(x - 1, y + 1) + Noise(x + 1, y + 1)) / 16.0f;
    float sides = (Noise(x - 1, y) + Noise(x + 1, y) + Noise(x, y - 1) + Noise(x, y + 1)) / 8.0f;
    float center = Noise(x, y) / 4.0f;
    
    return corners + sides + center;
}

float ProceduralGenerator::Perlin(float x, float y, float persistence, int octaves) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;
    
    for (int i = 0; i < octaves; i++) {
        total += SmoothNoise(x * frequency, y * frequency) * amplitude;
        
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }
    
    return total / maxValue;
}

} // namespace Engine