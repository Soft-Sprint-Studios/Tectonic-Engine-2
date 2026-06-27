#!/bin/sh

set -e

echo "Installing FreeBSD dependencies..."
sudo pkg install -y cmake ninja sdl3 sdl3_ttf openal-soft bullet

if [ ! -d "build_freebsd_x64" ]; then
    mkdir build_freebsd_x64
fi

cd build_freebsd_x64

echo "Configuring CMake..."
cmake -S .. -B . -G "Ninja" -DCMAKE_BUILD_TYPE=Release

echo "Building..."
cmake --build .

echo "Build complete."