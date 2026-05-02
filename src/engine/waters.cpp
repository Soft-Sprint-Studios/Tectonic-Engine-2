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
#include "waters.h"
#include "resources.h"
#include "filesystem.h"
#include <sstream>
#include <algorithm>

std::unordered_map<std::string, WaterDef> Waters::m_defs;
WaterDef Waters::m_fallback;

void Waters::Init()
{
    m_fallback.normalMap = Resources::LoadTexture("textures/water_normal.png", false);
    m_fallback.dudvMap = Resources::LoadTexture("textures/water_dudv.png", false);
    m_fallback.flowMap = nullptr;
    m_fallback.flowSpeed = 0.05f;
}

void Waters::LoadDefinitions(const std::string& path)
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

            WaterDef def = m_fallback;
            ss >> token; // {

            while (ss >> token && token != "}")
            {
                if (token == "normal")
                {
                    ss >> token;
                    ss >> token;
                    def.normalMap = Resources::LoadTexture("textures/" + token.substr(1, token.size() - 2), false);
                }
                else if (token == "dudv")
                {
                    ss >> token;
                    ss >> token;
                    def.dudvMap = Resources::LoadTexture("textures/" + token.substr(1, token.size() - 2), false);
                }
                else if (token == "flow")
                {
                    ss >> token;
                    ss >> token;
                    def.flowMap = Resources::LoadTexture("textures/" + token.substr(1, token.size() - 2), false);
                }
                else if (token == "flowspeed")
                {
                    ss >> token;
                    ss >> def.flowSpeed;
                }
            }
            m_defs[matName] = def;
        }
    }
}

WaterDef* Waters::GetDefinition(const std::string& name)
{
    auto it = m_defs.find(name);
    if (it != m_defs.end())
    {
        return &it->second;
    }
    return &m_fallback;
}