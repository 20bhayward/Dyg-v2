#!/bin/bash

# Exit on error
set -e

# Check if the executable exists
if [ ! -f "./build/DygEndless" ]; then
    echo "Executable not found. Running build script first..."
    ./build.sh
fi

# Run the simulation
echo "Starting Dyg-Endless Sand Simulation Engine..."
cd build
./DygEndless

echo "Simulation closed."