#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

// Command callback
typedef std::function<void(const std::vector<std::string>&)> ConCmdCallback;

class ConCmd
{
public:
    ConCmd(const std::string& name, ConCmdCallback callback, const std::string& description = "");

    static ConCmd* Find(const std::string& name);
    static std::unordered_map<std::string, ConCmd*>& GetRegistry();

    void Execute(const std::vector<std::string>& args);

    std::string GetDescription() const 
    { 
        return m_description; 
    }

private:
    std::string m_name;
    std::string m_description;
    ConCmdCallback m_callback;
};

#define CON_COMMAND(name, description) \
    void name##_callback(const std::vector<std::string>& args); \
    ConCmd name##_cmd(#name, name##_callback, description); \
    void name##_callback(const std::vector<std::string>& args)