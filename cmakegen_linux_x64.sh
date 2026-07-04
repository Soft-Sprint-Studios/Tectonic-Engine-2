#!/bin/bash

set -e

echo "Installing Linux dependencies..."
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libgl1-mesa-dev \
    libx11-dev \
    libxext-dev \
    libxinerama-dev \
    libxrandr-dev \
    libxi-dev \
    libxcursor-dev \
    libfontconfig1-dev \
    libopenal-dev \
    libvulkan-dev

if [ ! -d "build_linux_x64" ]; then
    mkdir build_linux_x64
fi

cd build_linux_x64

echo "Configuring CMake..."
cmake -S .. -B . -DCMAKE_BUILD_TYPE=Release

echo "Building..."
cmake --build . -- -j$(nproc)

echo "Build complete."