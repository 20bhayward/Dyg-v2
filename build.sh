#!/bin/bash

# Exit on error
set -e

echo "Building Dyg-Endless Sand Simulation Engine..."

# Create build directory if it doesn't exist
mkdir -p build

# Navigate to build directory
cd build

# Configure with CMake
echo "Running CMake..."
cmake ..

# Build the project
echo "Building project..."
make -j$(nproc)

echo "Build completed successfully!"
echo "Run ./build/DygEndless to start the simulation."