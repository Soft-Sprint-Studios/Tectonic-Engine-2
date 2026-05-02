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
#include "cvar.h"
#include "filesystem.h"
#include <fstream>
#include <sstream>

std::unordered_map<std::string, CVar*>& CVar::GetRegistry()
{
    static std::unordered_map<std::string, CVar*> s_registry;
    return s_registry;
}

CVar::CVar(const std::string& name, const std::string& defaultValue, const std::string& description, int flags)
    : m_name(name), m_value(defaultValue), m_description(description), m_flags(flags)
{
    GetRegistry()[name] = this;
}

CVar* CVar::Find(const std::string& name)
{
    auto& registry = GetRegistry();
    auto it = registry.find(name);
    return (it != registry.end()) ? it->second : nullptr;
}

void CVar::Set(const std::string& name, const std::string& value)
{
    if (CVar* var = Find(name))
    {
        var->m_value = value;
    }
}

int CVar::GetInt(const std::string& name, int defaultVal)
{
    CVar* var = Find(name);
    return var ? var->GetInt() : defaultVal;
}

float CVar::GetFloat(const std::string& name, float defaultVal)
{
    CVar* var = Find(name);
    return var ? var->GetFloat() : defaultVal;
}

std::string CVar::GetString(const std::string& name, const std::string& defaultVal)
{
    CVar* var = Find(name);
    return var ? var->GetString() : defaultVal;
}

void CVar::Init()
{
    std::string content = Filesystem::ReadText("cvars.txt");
    if (content.empty())
    {
        return;
    }

    std::stringstream ss(content);
    std::string line;
    while (std::getline(ss, line))
    {
        if (line.empty())
            continue;

        std::stringstream lineStream(line);
        std::string name, value;

        if (lineStream >> name >> value)
        {
            Set(name, value);
        }
    }
}

void CVar::Save()
{
    std::ofstream file(Filesystem::GetFullPath("cvars.txt"));
    if (!file.is_open())
    {
        return;
    }

    for (auto const& [name, cvar] : GetRegistry())
    {
        if (cvar->m_flags & CVAR_SAVE)
        {
            file << name << " " << cvar->m_value << "\n";
        }
    }
}