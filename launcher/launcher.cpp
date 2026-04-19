/*
 * MIT License
 *
 * Copyright (c) 2025-2026 Soft Sprint Studios
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <iostream>
#include <stdexcept>
#include <string>

using EngineMainFunc = int(*)(int, char**);

#ifdef _WIN32
// Hint Windows to prefer discrete AMD/NVIDIA GPUs over integrated ones (mostly for laptops).
extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 0x00000001;
}
#endif

class DynamicLibrary {
public:
    DynamicLibrary(const std::string& path) 
    {
#ifdef _WIN32
        handle = LoadLibraryA(path.c_str());
        if (!handle)
            throw std::runtime_error("Failed to load library: " + path);
#else
        handle = dlopen(path.c_str(), RTLD_NOW);
        if (!handle)
            throw std::runtime_error(std::string("Failed to load library: ") + dlerror());
#endif
    }

    ~DynamicLibrary() 
    {
#ifdef _WIN32
        if (handle) 
            FreeLibrary((HMODULE)handle);
#else
        if (handle) 
            dlclose(handle);
#endif
    }

    void* getSymbol(const std::string& name) 
    {
#ifdef _WIN32
        void* sym = (void*)GetProcAddress((HMODULE)handle, name.c_str());
        if (!sym)
            throw std::runtime_error("Failed to find symbol: " + name);
        return sym;
#else
        dlerror(); // clear
        void* sym = dlsym(handle, name.c_str());
        if (const char* err = dlerror())
            throw std::runtime_error(std::string("Failed to find symbol: ") + err);
        return sym;
#endif
    }

private:
    void* handle{};
};

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    DynamicLibrary lib("Engine.dll");
    auto Engine_Main = reinterpret_cast<EngineMainFunc>(lib.getSymbol("Engine_Main"));

    return Engine_Main(__argc, __argv);
}
#else
int main(int argc, char* argv[])
{
    DynamicLibrary lib("./Engine.so");
    auto Engine_Main = reinterpret_cast<EngineMainFunc>(lib.getSymbol("Engine_Main"));

    return Engine_Main(argc, argv);
}
#endif