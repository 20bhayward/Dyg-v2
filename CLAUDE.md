# CLAUDE.md - Instructions for Claude Code

## Build Commands
- Build with shaders: `./build_with_shaders.sh`
- Compile shaders only: `./compile_shaders.sh`
- Manual build:
```
mkdir -p build && cd build
cmake ..
cmake --build . -j $(nproc)
```
- Run: `./run.sh` or `./build/DygEndless`

## Code Style Guidelines
- **Naming**: 
  - Classes/Types: PascalCase
  - Methods/Functions: camelCase
  - Member Variables: m_camelCase
  - Constants: ALL_CAPS
- **Structure**:
  - Each class has matching .h/.cpp files
  - Public methods first, then private
  - Use forward declarations to reduce dependencies
- **Memory**: Smart pointers and RAII pattern
- **Error Handling**: Exceptions for critical errors, return values for operations
- **Documentation**: Comment classes, methods, and complex logic
- **Dependencies**: SDL2, Vulkan, GLM, nlohmann_json

## Project Structure
The codebase is a procedural simulation engine with cellular automata, chunked world generation, and Vulkan rendering.

## File List
### Core Files
- CMakeLists.txt - Main build configuration
- main.cpp - Application entry point
- build.sh, build_with_shaders.sh, compile_shaders.sh, run.sh - Build and run scripts

### Engine
- Engine/Core/Application.{h,cpp} - Main application class
- Engine/Core/Timer.{h,cpp} - Timing utilities

### Simulation
- Engine/Simulation/CellularAutomata.{h,cpp} - Simulation logic
- Engine/Simulation/Material.{h,cpp} - Material system
- Engine/Simulation/Particle.h - Particle definitions

### Procedural Generation
- Engine/Procedural/Chunk.{h,cpp} - World chunk management
- Engine/Procedural/ProceduralGenerator.{h,cpp} - Content generation
- Engine/Procedural/World.{h,cpp} - World management

### Rendering
- Engine/Rendering/Renderer.{h,cpp} - Rendering interface
- Engine/Rendering/VulkanRenderer.{h,cpp} - Vulkan implementation

### Assets
- Engine/Assets/Configs/materials.json - Material definitions
- Engine/Assets/Shaders/* - Shader files (GLSL and compiled SPIR-V)

### World Data
- worlddata/*.bin - Serialized chunk data