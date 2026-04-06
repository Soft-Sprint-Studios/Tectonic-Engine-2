#pragma once
#include <string>
#include <unordered_map>

enum CVarFlags
{
    CVAR_NONE = 0,
    CVAR_SAVE = (1 << 0)
};

class CVar
{
public:
    CVar(const std::string& name, const std::string& defaultValue, int flags = CVAR_NONE);

    static void Set(const std::string& name, const std::string& value);
    static CVar* Find(const std::string& name);
    static std::unordered_map<std::string, CVar*>& GetRegistry();

    static void Init();
    static void Save();

    std::string GetString() const
    {
        return m_value;
    }

    int GetInt() const
    {
        return std::stoi(m_value);
    }

    float GetFloat() const
    {
        return std::stof(m_value);
    }

private:
    std::string m_name;
    std::string m_value;
    int m_flags;
};