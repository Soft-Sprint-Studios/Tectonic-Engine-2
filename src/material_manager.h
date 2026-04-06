#pragma once
#include "texture.h"
#include <unordered_map>
#include <string>
#include <memory>

class MaterialManager
{
public:
    static void Init();
    static void LoadDefinitions(const std::string& path);
    static std::shared_ptr<Texture> GetTexture(const std::string& name);
    static std::shared_ptr<Texture> GetNormalMap(const std::string& name);
    static std::shared_ptr<Texture> GetSpecularMap(const std::string& name);

private:
    static void CreateFallbackTexture();
    static std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;
    static std::unordered_map<std::string, std::shared_ptr<Texture>> m_normals;
    static std::unordered_map<std::string, std::shared_ptr<Texture>> m_speculars;
    static std::shared_ptr<Texture> m_fallback;
};