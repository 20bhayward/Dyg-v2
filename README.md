# Dyg-Endless: Dynamic Falling-Sand Simulation Engine

A robust simulation engine inspired by games like Noita, utilizing cellular automata to create a dynamic, physics-based world with high-quality rendering.

## Features

- **Cellular Automata Simulation**: Particles interact based on material properties using rule-based physics
- **Material System**: Define custom materials with properties like density, viscosity, flammability, and more
- **Procedural World Generation**: Infinite world streaming with procedural terrain generation
- **Multi-Threaded Performance**: Optimized for modern CPUs with multi-threaded chunk updates
- **Vulkan Rendering**: Hardware-accelerated graphics with shader-based effects
- **Save/Load System**: Chunks can be saved and loaded from disk
- **Interactive Editor**: Place materials and explore the world with intuitive controls

## Requirements

- C++17 compatible compiler
- CMake 3.12 or higher
- SDL2 (Simple DirectMedia Layer)
- Vulkan SDK 1.3+ (for shader compilation and rendering)
- GLM (OpenGL Mathematics)
- nlohmann_json (JSON for Modern C++)

## Building

### Quick Build

Use the provided build script that handles shader compilation and the build process:

```bash
chmod +x build_with_shaders.sh
./build_with_shaders.sh
```

### Manual Build

1. Compile the shaders (requires Vulkan SDK with glslc):
```bash
chmod +x compile_shaders.sh
./compile_shaders.sh
```

2. Create a build directory and build the project:
```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

## Running

After building, run the executable from the build directory:

```bash
cd build
./DygEndless
```

## Controls

- **WASD/Arrow Keys**: Move camera
- **Mouse Wheel**: Zoom in/out
- **Middle Mouse**: Pan camera
- **Left Mouse**: Place selected material
- **1-5 Keys**: Select material (1=Sand, 2=Water, 3=Stone, 4=Fire, 5=Wood)
- **+/- Keys**: Increase/decrease brush size
- **ESC**: Exit the application

## Project Structure

- `/Engine/Core`: Core engine functionality
- `/Engine/Simulation`: Cellular automata and material systems
- `/Engine/Procedural`: Chunk and world management with procedural generation
- `/Engine/Rendering`: Graphics rendering system with Vulkan backend
- `/Engine/Assets`: Resources, shaders, and configuration files

## Rendering System

The engine uses a modern Vulkan rendering pipeline for high-performance graphics:

- **Shader-Based Effects**: Custom shaders for material rendering and post-processing
- **Material Visualization**: Special visual effects for different materials (glowing fire, flowing water)
- **Hardware Acceleration**: Utilizes GPU for particle rendering
- **Extensible Pipeline**: Easy to add new visual effects and rendering techniques

## Material System

Materials are defined in `/Engine/Assets/Configs/materials.json`. You can add your own materials by editing this file, following the existing pattern.

Each material has properties such as:
- **Density**: Affects how it falls
- **Viscosity**: How much it flows
- **Flammability**: How easily it catches fire
- **Visual Properties**: Color, transparency, and special effects

## Extending

The engine is designed to be modular and extensible. You can:

1. Add new materials by editing the `materials.json` file
2. Implement new cellular automata rules in `CellularAutomata.cpp`
3. Create custom procedural generation algorithms in `ProceduralGenerator.cpp`
4. Add visual effects by modifying the shaders in `Engine/Assets/Shaders/`

## License

This project is released under the MIT License. See [LICENSE](LICENSE) for details.

## Acknowledgements

- Inspired by games like Noita, Oxygen Not Included, and Terraria
- Thanks to the falling-sand genre community for ideas and inspiration
- Vulkan rendering techniques inspired by modern graphics programming practices