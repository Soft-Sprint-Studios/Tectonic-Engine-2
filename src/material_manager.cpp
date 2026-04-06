#include "material_manager.h"
#include "resource_manager.h"
#include "filesystem.h"
#include "console.h"
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>

std::unordered_map<std::string, std::shared_ptr<Texture>> MaterialManager::m_textures;
std::unordered_map<std::string, std::shared_ptr<Texture>> MaterialManager::m_normals;
std::unordered_map<std::string, std::shared_ptr<Texture>> MaterialManager::m_speculars;
std::shared_ptr<Texture> MaterialManager::m_fallback;

void MaterialManager::Init()
{
    CreateFallbackTexture();
}

void MaterialManager::CreateFallbackTexture()
{
    const int size = 64;
    const int checkSize = 8;
    std::vector<uint8_t> data(size * size * 4);

    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            bool isPink = ((x / checkSize) + (y / checkSize)) % 2 == 0;
            int idx = (y * size + x) * 4;

            data[idx + 0] = isPink ? 255 : 0; // R
            data[idx + 1] = isPink ? 0 : 0; // G
            data[idx + 2] = isPink ? 255 : 0; // B
            data[idx + 3] = 255; // A
        }
    }

    m_fallback = std::make_shared<Texture>();
    m_fallback->Create(size, size, data.data());
}

void MaterialManager::LoadDefinitions(const std::string& path)
{
    std::string content = Filesystem::ReadText(path);

    if (content.empty())
    {
        return;
    }

    std::stringstream ss(content);
    std::string token;

    while (ss >> token)
    {
        if (token.front() == '\"')
        {
            std::string matName = token.substr(1, token.size() - 2);
            std::transform(matName.begin(), matName.end(), matName.begin(), ::tolower);

            ss >> token;

            while (ss >> token && token != "}")
            {
                if (token == "diffuse")
                {
                    ss >> token;
                    ss >> token;

                    std::string fileName = "textures/" + token.substr(1, token.size() - 2);
                    auto tex = ResourceManager::LoadTexture(fileName);

                    if (tex)
                    {
                        m_textures[matName] = tex;
                    }
                }
                else if (token == "normal")
                {
                    ss >> token; 
                    ss >> token;

                    std::string fileName = "textures/" + token.substr(1, token.size() - 2);
                    auto tex = ResourceManager::LoadTexture(fileName);

                    if (tex)
                    {
                        m_normals[matName] = tex;
                    }
                }
                else if (token == "specular")
                {
                    ss >> token; 
                    ss >> token;

                    std::string fileName = "textures/" + token.substr(1, token.size() - 2);
                    auto tex = ResourceManager::LoadTexture(fileName);

                    if (tex) 
                        m_speculars[matName] = tex;
                }
            }
        }
    }
}

std::shared_ptr<Texture> MaterialManager::GetTexture(const std::string& name)
{
    std::string searchName = name;
    std::transform(searchName.begin(), searchName.end(), searchName.begin(), ::tolower);

    auto it = m_textures.find(searchName);

    if (it != m_textures.end())
    {
        return it->second;
    }

    if (!searchName.empty())
    {
        Console::Warn("Texture definition missing: [" + name + "] - using fallback.");
    }

    return m_fallback;
}

std::shared_ptr<Texture> MaterialManager::GetNormalMap(const std::string& name)
{
    std::string searchName = name;
    std::transform(searchName.begin(), searchName.end(), searchName.begin(), ::tolower);

    auto it = m_normals.find(searchName);
    if (it != m_normals.end())
    {
        return it->second;
    }

    if (!searchName.empty())
    {
        Console::Warn("Texture definition missing: [" + name + "] - using fallback.");
    }

    return m_fallback;
}

std::shared_ptr<Texture> MaterialManager::GetSpecularMap(const std::string& name)
{
    std::string searchName = name;
    std::transform(searchName.begin(), searchName.end(), searchName.begin(), ::tolower);

    auto it = m_speculars.find(searchName);
    if (it != m_speculars.end())
    {
        return it->second;
    }

    if (!searchName.empty())
    {
        Console::Warn("Texture definition missing: [" + name + "] - using fallback.");
    }

    return m_fallback;
}