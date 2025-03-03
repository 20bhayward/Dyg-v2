#!/bin/bash

# Color setup for messages
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Exit on error
set -e

# Check for --clean flag
CLEAN_BUILD=0
for arg in "$@"; do
  if [ "$arg" == "--clean" ]; then
    CLEAN_BUILD=1
  fi
done

# Print header
echo -e "${BLUE}=========================================${NC}"
echo -e "${BLUE}   Dyg-Endless Engine Build Script      ${NC}"
echo -e "${BLUE}=========================================${NC}"

# Clean build directory if requested
if [ $CLEAN_BUILD -eq 1 ]; then
  echo -e "${YELLOW}Cleaning build directory...${NC}"
  rm -rf build
fi

# Make output directories
mkdir -p Engine/Assets/Shaders/spirv

# Check if glslc is available
if command -v glslc &> /dev/null; then
  echo -e "${GREEN}Found glslc, compiling shaders...${NC}"
  
  # Compile shaders
  echo -e "${BLUE}Compiling shaders...${NC}"
  
  # Vertex shaders
  for shader in Engine/Assets/Shaders/*.vert; do
    if [ -f "$shader" ]; then
      base=$(basename "$shader" .vert)
      echo -e "${BLUE}Compiling ${shader}...${NC}"
      glslc -o "Engine/Assets/Shaders/spirv/${base}.vert.spv" "$shader"
    fi
  done
  
  # Fragment shaders
  for shader in Engine/Assets/Shaders/*.frag; do
    if [ -f "$shader" ]; then
      base=$(basename "$shader" .frag)
      echo -e "${BLUE}Compiling ${shader}...${NC}"
      glslc -o "Engine/Assets/Shaders/spirv/${base}.frag.spv" "$shader"
    fi
  done
  
  echo -e "${GREEN}Shader compilation complete!${NC}"
else
  echo -e "${YELLOW}Warning: Could not find glslc shader compiler. Skipping shader compilation.${NC}"
  echo -e "${YELLOW}Install the Vulkan SDK to compile shaders.${NC}"
fi

# Build the project
echo -e "${BLUE}Building Dyg-Endless Engine...${NC}"

# Create build directory if it doesn't exist
mkdir -p build

# Create shader directories in build
mkdir -p build/Engine/Assets/Shaders/spirv

# Copy shaders to build directory
echo -e "${BLUE}Copying shaders to build directory...${NC}"
cp -f Engine/Assets/Shaders/spirv/*.spv build/Engine/Assets/Shaders/spirv/ 2>/dev/null || echo -e "${YELLOW}No shader files to copy${NC}"

# Configure and build
echo -e "${BLUE}Configuring with CMake...${NC}"
cd build
cmake .. || { echo -e "${RED}CMake configuration failed!${NC}"; exit 1; }

echo -e "${BLUE}Building project...${NC}"
cmake --build . -j $(nproc) || { echo -e "${RED}Build failed!${NC}"; exit 1; }

# Done!
echo -e "${GREEN}Build completed successfully!${NC}"
echo -e "${GREEN}Run ./build/DygEndless to start the simulation.${NC}"
echo -e "${BLUE}=========================================${NC}"