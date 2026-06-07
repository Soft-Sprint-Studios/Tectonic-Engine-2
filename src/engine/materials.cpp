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
#include "materials.h"
#include "resources.h"
#include "filesystem.h"
#include "console.h"
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>

std::unordered_map<std::string, std::shared_ptr<R_Texture>> Materials::m_textures;
std::unordered_map<std::string, std::shared_ptr<R_Texture>> Materials::m_normals;
std::unordered_map<std::string, std::shared_ptr<R_Texture>> Materials::m_mraohs;
std::unordered_map<std::string, std::shared_ptr<R_Texture>> Materials::m_textures2;
std::unordered_map<std::string, std::shared_ptr<R_Texture>> Materials::m_normals2;
std::unordered_map<std::string, std::shared_ptr<R_Texture>> Materials::m_mraohs2;
std::unordered_map<std::string, float> Materials::m_heightScales;
std::unordered_map<std::string, float> Materials::m_heightScales2;
std::shared_ptr<R_Texture> Materials::m_fallback;
std::shared_ptr<R_Texture> Materials::m_defaultNormal;
std::shared_ptr<R_Texture> Materials::m_defaultMraoh;

void Materials::Init()
{
    CreateFallbackTexture();
}

void Materials::CreateFallbackTexture()
{
    const int size = 32;
    const int checkSize = 4;
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

    m_fallback = std::make_shared<R_Texture>();
    m_fallback->Create(size, size, data.data(), true);

    uint8_t normalData[] = { 128, 128, 255, 0 };
    m_defaultNormal = std::make_shared<R_Texture>();
    m_defaultNormal->Create(1, 1, normalData, false);

    uint8_t mrahData[] = { 0, 128, 255, 255 };
    m_defaultMraoh = std::make_shared<R_Texture>();
    m_defaultMraoh->Create(1, 1, mrahData, false);
}

void Materials::LoadDefinitions(const std::string& path)
{
    std::string content = Filesystem::ReadText(path);

    if (content.empty())
    {
        return;
    }

    std::stringstream ss(content);
    std::string token;

    while (ss >> token)  // {
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
                    auto tex = Resources::LoadTexture(fileName, true);

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
                    auto tex = Resources::LoadTexture(fileName, false);

                    if (tex)
                    {
                        m_normals[matName] = tex;
                    }
                }
                else if (token == "mraoh")
                {
                    ss >> token;
                    ss >> token;

                    auto tex = Resources::LoadTexture("textures/" + token.substr(1, token.size() - 2), false);

                    if (tex) 
                    { 
                        m_mraohs[matName] = tex;
                    }
                }
                else if (token == "diffuse2")
                {
                    ss >> token; 
                    ss >> token;

                    auto tex = Resources::LoadTexture("textures/" + token.substr(1, token.size() - 2), true);

                    if (tex) 
                    { 
                        m_textures2[matName] = tex; 
                    }
                }
                else if (token == "normal2")
                {
                    ss >> token; 
                    ss >> token;

                    auto tex = Resources::LoadTexture("textures/" + token.substr(1, token.size() - 2), false);

                    if (tex) 
                    { 
                        m_normals2[matName] = tex; 
                    }
                }
                else if (token == "mraoh2")
                {
                    ss >> token;
                    ss >> token;

                    auto tex = Resources::LoadTexture("textures/" + token.substr(1, token.size() - 2), false);

                    if (tex) 
                    { 
                        m_mraohs2[matName] = tex;
                    }
                }
                else if (token == "heightscale")
                {
                    float s;

                    ss >> token;
                    ss >> s;

                    m_heightScales[matName] = s;
                }
                else if (token == "heightscale2")
                {
                    float s;

                    ss >> token;
                    ss >> s;

                    m_heightScales2[matName] = s;
                }
            }
        }
    }
}

std::shared_ptr<R_Texture> Materials::GetTexture(const std::string& name)
{
    auto it = m_textures.find(name);

    if (it != m_textures.end())
    {
        return it->second;
    }

    if (!name.empty())
    {
        Console::Warn("Texture definition missing: [" + name + "] - using fallback.");
    }

    return m_fallback;
}

std::shared_ptr<R_Texture> Materials::GetNormalMap(const std::string& name)
{
    auto it = m_normals.find(name);
    if (it != m_normals.end())
    {
        return it->second;
    }

    if (!name.empty())
    {
        return m_defaultNormal;
    }

    return m_defaultNormal;
}

std::shared_ptr<R_Texture> Materials::GetMRAOMap(const std::string& name)
{
    auto it = m_mraohs.find(name);
    return (it != m_mraohs.end()) ? it->second : m_defaultMraoh;
}

std::shared_ptr<R_Texture> Materials::GetTexture2(const std::string& name) 
{
    auto it = m_textures2.find(name);
    return (it != m_textures2.end()) ? it->second : nullptr;
}

std::shared_ptr<R_Texture> Materials::GetNormalMap2(const std::string& name) 
{
    auto it = m_normals2.find(name);
    return (it != m_normals2.end()) ? it->second : m_defaultNormal;
}

std::shared_ptr<R_Texture> Materials::GetMRAOMap2(const std::string& name)
{
    auto it = m_mraohs2.find(name);
    return (it != m_mraohs2.end()) ? it->second : m_defaultMraoh;
}

float Materials::GetHeightScale(const std::string& name)
{
    auto it = m_heightScales.find(name);
    return (it != m_heightScales.end()) ? it->second : 0.00f;
}

float Materials::GetHeightScale2(const std::string& name)
{
    auto it = m_heightScales2.find(name);
    return (it != m_heightScales2.end()) ? it->second : 0.00f;
}