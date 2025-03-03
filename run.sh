#!/bin/bash

# Color setup for messages
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Exit on error
set -e

echo -e "${BLUE}=========================================${NC}"
echo -e "${BLUE}   Dyg-Endless Engine Runner            ${NC}"
echo -e "${BLUE}=========================================${NC}"

# Check if the executable exists
if [ ! -f "./build/DygEndless" ]; then
    echo -e "${YELLOW}Executable not found. Running build script first...${NC}"
    ./build_with_shaders.sh
fi

# Run the simulation
echo -e "${GREEN}Starting Dyg-Endless Sand Simulation Engine...${NC}"
echo -e "${BLUE}Controls:${NC}"
echo -e "${BLUE}- Left-click to place particles${NC}"
echo -e "${BLUE}- Right-click to erase particles${NC}"
echo -e "${BLUE}- Keys 1-0 to select different materials${NC}"
echo -e "${BLUE}- Shift + / - to adjust brush size${NC}"
echo -e "${BLUE}- Arrow keys or WASD to move camera${NC}"
echo -e "${BLUE}- Mouse wheel to zoom in/out${NC}"
echo -e "${BLUE}- ESC to exit${NC}"
echo -e "${BLUE}=========================================${NC}"

cd build
./DygEndless

echo -e "${GREEN}Simulation closed.${NC}"