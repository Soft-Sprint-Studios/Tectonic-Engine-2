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
    libxft-dev \
    libxrender-dev \
    libxfixes-dev \
    libxcursor-dev \
    libfontconfig1-dev \
    libopenal-dev

if [ ! -d "buildlinux" ]; then
    mkdir buildlinux
fi

cd buildlinux

echo "Configuring CMake..."
cmake -S .. -B . -DCMAKE_BUILD_TYPE=Release

echo "Building..."
cmake --build . -- -j$(nproc)

echo "Build complete."