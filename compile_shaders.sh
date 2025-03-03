#!/bin/bash

# This script compiles GLSL shaders to SPIR-V format for Vulkan

# Make sure the output directory exists
mkdir -p Engine/Assets/Shaders/spirv

# Compile all shaders
echo "Compiling shaders..."

# Vertex shaders
for shader in Engine/Assets/Shaders/*.vert; do
    base=$(basename "$shader" .vert)
    echo "Compiling $shader..."
    glslc -o "Engine/Assets/Shaders/spirv/${base}.vert.spv" "$shader"
done

# Fragment shaders
for shader in Engine/Assets/Shaders/*.frag; do
    base=$(basename "$shader" .frag)
    echo "Compiling $shader..."
    glslc -o "Engine/Assets/Shaders/spirv/${base}.frag.spv" "$shader"
done

# Copy all compiled shaders to the build directory if it exists
if [ -d "build" ]; then
    echo "Copying shaders to build directory..."
    mkdir -p build/Engine/Assets/Shaders/spirv
    cp Engine/Assets/Shaders/spirv/*.spv build/Engine/Assets/Shaders/spirv/ 2>/dev/null || echo "No shader files to copy"
fi

echo "Shader compilation complete!"