#include "cvar.h"
#include "filesystem.h"
#include <fstream>
#include <sstream>

std::unordered_map<std::string, CVar*>& CVar::GetRegistry()
{
    static std::unordered_map<std::string, CVar*> s_registry;
    return s_registry;
}

CVar::CVar(const std::string& name, const std::string& defaultValue, int flags)
    : m_name(name), m_value(defaultValue), m_flags(flags)
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