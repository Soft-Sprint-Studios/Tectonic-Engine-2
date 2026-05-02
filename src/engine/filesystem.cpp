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
#include "filesystem.h"
#include "console.h"
#include <SDL3/SDL.h>
#include <fstream>
#include <filesystem>

namespace Filesystem
{
    static std::string s_basePath;

    void Init()
    {
        if (const char* base = SDL_GetBasePath())
        {
            s_basePath = base;
            SDL_free(const_cast<char*>(base));
        }
    }

    std::string GetFullPath(const std::string& relativePath)
    {
        return s_basePath + relativePath;
    }

    std::vector<uint8_t> ReadBinary(const std::string& path)
    {
        std::string fullPath = GetFullPath(path);
        std::ifstream file(fullPath, std::ios::ate | std::ios::binary);

        if (!file.is_open()) 
        {
            Console::Error("Failed to open binary file: " + fullPath);
            return {};
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<uint8_t> buffer(fileSize);
        file.seekg(0);
        file.read((char*)buffer.data(), fileSize);
        
        return buffer;
    }

    std::string ReadText(const std::string& path)
    {
        std::string fullPath = GetFullPath(path);
        std::ifstream file(fullPath);

        if (!file.is_open()) 
        {
            Console::Error("Failed to open text file: " + fullPath);
            return "";
        }

        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    bool Exists(const std::string& relativePath)
    {
        return std::filesystem::exists(GetFullPath(relativePath));
    }

    void CreateDirectory(const std::string& relativePath)
    {
        std::filesystem::create_directory(GetFullPath(relativePath));
    }

    std::vector<std::string> ListFiles(const std::string& relativePath, const std::string& extension)
    {
        std::vector<std::string> results;
        std::string fullPath = GetFullPath(relativePath);
        if (!std::filesystem::exists(fullPath)) 
            return results;

        for (const auto& entry : std::filesystem::directory_iterator(fullPath))
        {
            if (entry.path().extension() == extension)
            {
                results.push_back(entry.path().stem().string());
            }
        }
        return results;
    }

    // For now here
    bool StringEqual(const std::string& a, const std::string& b)
    {
        if (a.length() != b.length())
            return false;
        for (size_t i = 0; i < a.length(); ++i)
        {
            if (tolower(a[i]) != tolower(b[i]))
                return false;
        }
        return true;
    }
}