@echo off
if not exist build_windows_arm64 (
    mkdir build_windows_arm64
)

cd build_windows_arm64
cmake -G "Visual Studio 18 2026" -A ARM64 ..