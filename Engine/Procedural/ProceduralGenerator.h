#pragma once

#include "Chunk.h"
#include <random>
#include <glm/glm.hpp>

namespace Engine {

class ProceduralGenerator {
public:
    ProceduralGenerator(uint32_t seed = 12345);
    ~ProceduralGenerator();
    
    void GenerateChunk(Chunk* chunk);
    
    // Different types of terrain generation
    void GenerateFlat(Chunk* chunk);
    void GenerateTerrain(Chunk* chunk);
    void GenerateCaves(Chunk* chunk);
    
private:
    std::mt19937 m_Random;
    uint32_t m_Seed;
    
    // Noise generation helpers
    float Noise(float x, float y);
    float SmoothNoise(float x, float y);
    float Perlin(float x, float y, float persistence, int octaves);
};

} // namespace Engine