#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <glm/glm.hpp>

namespace Engine {

struct Material {
    std::string name;
    uint8_t id;
    float density;
    float viscosity;
    float flammability;
    glm::vec4 color;         // RGBA for rendering
    
    bool isLiquid = false;   // If true, will spread horizontally
    bool isGas = false;      // If true, will rise and spread
    bool isSolid = false;    // If true, will not move unless disturbed
    
    // Additional parameters
    float spreadFactor = 1.0f;  // How quickly it spreads horizontally
    float corrosiveness = 0.0f; // How quickly it corrodes other materials
    
    // Default constructor required for std::unordered_map
    Material() 
        : name("Undefined"), id(0), density(1.0f), viscosity(0.0f), flammability(0.0f), color(1.0f, 1.0f, 1.0f, 1.0f) {}
    
    Material(uint8_t _id, const std::string& _name) 
        : name(_name), id(_id), density(1.0f), viscosity(0.0f), flammability(0.0f), color(1.0f, 1.0f, 1.0f, 1.0f) {}
};

class MaterialDatabase {
public:
    static MaterialDatabase& Get() {
        static MaterialDatabase instance;
        return instance;
    }
    
    void AddMaterial(const Material& material) {
        m_Materials[material.id] = material;
    }
    
    const Material& GetMaterial(uint8_t id) const {
        return m_Materials.at(id);
    }
    
    static void Initialize();
    static void LoadMaterials(const std::string& configPath);
    
private:
    MaterialDatabase() = default;
    std::unordered_map<uint8_t, Material> m_Materials;
};

} // namespace Engine