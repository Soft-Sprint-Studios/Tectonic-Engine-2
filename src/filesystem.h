#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace Filesystem
{
    void Init();

    std::string GetFullPath(const std::string& relativePath);
    std::vector<uint8_t> ReadBinary(const std::string& path);
    std::string ReadText(const std::string& path);

    bool Exists(const std::string& relativePath);
    std::vector<std::string> ListFiles(const std::string& relativePath, const std::string& extension);
}