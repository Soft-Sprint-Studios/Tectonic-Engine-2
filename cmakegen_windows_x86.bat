@echo off
if not exist buildx86 (
    mkdir buildx86
)

cd buildx86
cmake -G "Visual Studio 18 2026" -A Win32 ..