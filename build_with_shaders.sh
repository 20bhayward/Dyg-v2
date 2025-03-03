#!/bin/bash

# Exit on error
set -e

# Make output directories
mkdir -p Engine/Assets/Shaders/spirv

# Check if glslc is available
if command -v glslc &> /dev/null
then
    echo "Found glslc, compiling shaders..."
    
    # Compile shaders
    ./compile_shaders.sh
else
    echo "Warning: Could not find glslc shader compiler. Skipping shader compilation."
    echo "Install the Vulkan SDK to compile shaders."
fi

# Build the project
echo "Building Dyg-Endless Sand Simulation Engine..."

# Create build directory if it doesn't exist
mkdir -p build

# Navigate to build directory
cd build

# Configure with CMake
echo "Running CMake..."
cmake ..

# Build the project using all available cores
echo "Building project..."
cmake --build . -j $(nproc)

echo "Build completed successfully!"
echo "Run ./build/DygEndless to start the simulation."