#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include "texture.h"

class ResourceManager
{
public:
    static std::shared_ptr<Texture> LoadTexture(const std::string& path);
    static void UnloadUnused();
    static void Clear();

private:
    static std::unordered_map<std::string, std::shared_ptr<Texture>> s_textures;
};