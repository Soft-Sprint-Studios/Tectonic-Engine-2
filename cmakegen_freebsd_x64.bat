#!/bin/sh

set -e

echo "Installing FreeBSD dependencies..."
sudo pkg install -y cmake ninja sdl3 sdl3_ttf openal-soft bullet

if [ ! -d "buildfreebsd" ]; then
    mkdir buildfreebsd
fi

cd buildfreebsd

echo "Configuring CMake..."
cmake -S .. -B . -G "Ninja" -DCMAKE_BUILD_TYPE=Release

echo "Building..."
cmake --build .

echo "Build complete."