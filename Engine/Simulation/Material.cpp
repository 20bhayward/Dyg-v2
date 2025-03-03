#include "Material.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace Engine {

void MaterialDatabase::Initialize() {
    MaterialDatabase& db = Get();
    
    // Add default empty material (ID 0)
    Material emptyMaterial(0, "Empty");
    emptyMaterial.density = 0.0f;
    emptyMaterial.color = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f); // Transparent
    db.AddMaterial(emptyMaterial);
    
    // Add sand (ID 1)
    Material sand(1, "Sand");
    sand.density = 1.5f;
    sand.isSolid = true;
    sand.color = glm::vec4(0.76f, 0.7f, 0.5f, 1.0f); // Sandy color
    db.AddMaterial(sand);
    
    // Add water (ID 2)
    Material water(2, "Water");
    water.density = 1.0f;
    water.viscosity = 0.7f;
    water.isLiquid = true;
    water.spreadFactor = 4.0f;
    water.color = glm::vec4(0.0f, 0.3f, 0.8f, 0.8f); // Blue, semi-transparent
    db.AddMaterial(water);
    
    // Add stone (ID 3)
    Material stone(3, "Stone");
    stone.density = 2.5f;
    stone.isSolid = true;
    stone.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray
    db.AddMaterial(stone);
    
    // Add fire (ID 4)
    Material fire(4, "Fire");
    fire.density = 0.2f;
    fire.flammability = 1.0f;
    fire.color = glm::vec4(1.0f, 0.3f, 0.0f, 0.9f); // Orange-red
    db.AddMaterial(fire);
    
    // Add wood (ID 5)
    Material wood(5, "Wood");
    wood.density = 0.8f;
    wood.isSolid = true;
    wood.flammability = 0.7f;
    wood.color = glm::vec4(0.6f, 0.4f, 0.2f, 1.0f); // Brown
    db.AddMaterial(wood);
    
    // Add gunpowder (ID 6)
    Material gunpowder(6, "Gunpowder");
    gunpowder.density = 1.3f;
    gunpowder.isSolid = true;
    gunpowder.flammability = 0.95f; // Highly flammable
    gunpowder.color = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f); // Dark gray
    db.AddMaterial(gunpowder);
    
    // Add acid (ID 7)
    Material acid(7, "Acid");
    acid.density = 1.1f;
    acid.viscosity = 0.6f;
    acid.isLiquid = true;
    acid.spreadFactor = 3.5f;
    acid.corrosiveness = 0.8f; // Highly corrosive
    acid.color = glm::vec4(0.8f, 1.0f, 0.2f, 0.9f); // Bright green
    db.AddMaterial(acid);
    
    // Add oil (ID 8)
    Material oil(8, "Oil");
    oil.density = 0.85f; // Less dense than water, will float
    oil.viscosity = 0.8f; // More viscous than water
    oil.isLiquid = true;
    oil.spreadFactor = 3.0f;
    oil.flammability = 0.85f;
    oil.color = glm::vec4(0.1f, 0.1f, 0.1f, 0.8f); // Dark with some transparency
    db.AddMaterial(oil);
    
    // Add smoke (ID 9)
    Material smoke(9, "Smoke");
    smoke.density = 0.1f;
    smoke.isGas = true;
    smoke.color = glm::vec4(0.7f, 0.7f, 0.7f, 0.4f); // Light gray, transparent
    db.AddMaterial(smoke);
    
    // Add salt (ID 10)
    Material salt(10, "Salt");
    salt.density = 1.4f;
    salt.isSolid = true;
    salt.color = glm::vec4(0.9f, 0.9f, 0.9f, 1.0f); // White
    db.AddMaterial(salt);
}

void MaterialDatabase::LoadMaterials(const std::string& configPath) {
    try {
        std::ifstream file(configPath);
        if (!file.is_open()) {
            std::cerr << "Failed to open material config file: " << configPath << std::endl;
            return;
        }
        
        nlohmann::json json;
        file >> json;
        
        MaterialDatabase& db = Get();
        
        for (const auto& materialJson : json["materials"]) {
            uint8_t id = materialJson["id"].get<uint8_t>();
            std::string name = materialJson["name"].get<std::string>();
            
            Material material(id, name);
            
            // Load properties
            if (materialJson.contains("density"))
                material.density = materialJson["density"].get<float>();
                
            if (materialJson.contains("viscosity"))
                material.viscosity = materialJson["viscosity"].get<float>();
                
            if (materialJson.contains("flammability"))
                material.flammability = materialJson["flammability"].get<float>();
                
            if (materialJson.contains("isLiquid"))
                material.isLiquid = materialJson["isLiquid"].get<bool>();
                
            if (materialJson.contains("isGas"))
                material.isGas = materialJson["isGas"].get<bool>();
                
            if (materialJson.contains("isSolid"))
                material.isSolid = materialJson["isSolid"].get<bool>();
                
            if (materialJson.contains("spreadFactor"))
                material.spreadFactor = materialJson["spreadFactor"].get<float>();
                
            if (materialJson.contains("corrosiveness"))
                material.corrosiveness = materialJson["corrosiveness"].get<float>();
                
            // Load color
            if (materialJson.contains("color")) {
                auto& colorJson = materialJson["color"];
                material.color = glm::vec4(
                    colorJson["r"].get<float>(),
                    colorJson["g"].get<float>(),
                    colorJson["b"].get<float>(),
                    colorJson["a"].get<float>()
                );
            }
            
            db.AddMaterial(material);
        }
        
        std::cout << "Successfully loaded materials from: " << configPath << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading materials: " << e.what() << std::endl;
    }
}

} // namespace Engine