#include "concmd.h"

std::unordered_map<std::string, ConCmd*>& ConCmd::GetRegistry()
{
    static std::unordered_map<std::string, ConCmd*> s_cmdRegistry;
    return s_cmdRegistry;
}

ConCmd::ConCmd(const std::string& name, ConCmdCallback callback, const std::string& description)
    : m_name(name), m_callback(callback), m_description(description)
{
    GetRegistry()[name] = this;
}

ConCmd* ConCmd::Find(const std::string& name)
{
    auto& registry = GetRegistry();
    auto it = registry.find(name);
    return (it != registry.end()) ? it->second : nullptr;
}

void ConCmd::Execute(const std::vector<std::string>& args)
{
    if (m_callback)
    {
        m_callback(args);
    }
}