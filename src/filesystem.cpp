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

    std::vector<std::string> ListFiles(const std::string& relativePath, const std::string& extension)
    {
        std::vector<std::string> results;
        std::string fullPath = GetFullPath(relativePath);
        if (!std::filesystem::exists(fullPath)) return results;

        for (const auto& entry : std::filesystem::directory_iterator(fullPath))
        {
            if (entry.path().extension() == extension)
            {
                results.push_back(entry.path().stem().string());
            }
        }
        return results;
    }
}