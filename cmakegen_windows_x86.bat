@echo off
if not exist build_windows_x86 (
    mkdir build_windows_x86
)

cd build_windows_x86
cmake -G "Visual Studio 18 2026" -A Win32 ..