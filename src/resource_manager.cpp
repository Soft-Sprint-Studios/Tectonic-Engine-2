#include "resource_manager.h"
#include "console.h"

std::unordered_map<std::string, std::shared_ptr<Texture>> ResourceManager::s_textures;

std::shared_ptr<Texture> ResourceManager::LoadTexture(const std::string& path)
{
    auto it = s_textures.find(path);
    if (it != s_textures.end())
    {
        return it->second;
    }

    auto tex = std::make_shared<Texture>();
    if (tex->Load(path))
    {
        s_textures[path] = tex;
        return tex;
    }

    return nullptr;
}

void ResourceManager::UnloadUnused()
{
    for (auto it = s_textures.begin(); it != s_textures.end();)
    {
        if (it->second.use_count() <= 1)
        {
            it = s_textures.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void ResourceManager::Clear()
{
    s_textures.clear();
}