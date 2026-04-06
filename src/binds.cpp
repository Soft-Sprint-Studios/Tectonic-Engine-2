#include "binds.h"
#include "console.h"
#include "concmd.h"
#include "input.h"
#include "filesystem.h"
#include <fstream>
#include <sstream>

namespace Binds
{
    static std::unordered_map<SDL_Scancode, std::string> s_bindMap;

    void Init()
    {
        std::string content = Filesystem::ReadText("binds.txt");
        if (content.empty()) 
            return;

        std::stringstream ss(content);
        std::string line;
        while (std::getline(ss, line))
        {
            if (line.empty()) 
                continue;

            size_t firstQuote = line.find('\"');
            size_t secondQuote = line.find('\"', firstQuote + 1);
            size_t thirdQuote = line.find('\"', secondQuote + 1);
            size_t fourthQuote = line.find('\"', thirdQuote + 1);

            if (firstQuote != std::string::npos && fourthQuote != std::string::npos)
            {
                std::string key = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
                std::string cmd = line.substr(thirdQuote + 1, fourthQuote - thirdQuote - 1);
                SetBind(key, cmd);
            }
        }
    }

    void Save()
    {
        std::ofstream file(Filesystem::GetFullPath("binds.txt"));
        for (auto const& [code, cmd] : s_bindMap)
        {
            std::string keyName = SDL_GetScancodeName(code);
            file << "bind \"" << keyName << "\" \"" << cmd << "\"\n";
        }
    }

    void Update(const Input& input)
    {
        for (auto const& [code, cmd] : s_bindMap)
        {
            if (input.GetKeyDown(code))
            {
                Console::Execute(cmd);
            }
        }
    }

    void SetBind(const std::string& keyName, const std::string& command)
    {
        SDL_Scancode code = SDL_GetScancodeFromName(keyName.c_str());
        if (code == SDL_SCANCODE_UNKNOWN)
        {
            Console::Warn("Unknown key: " + keyName);
            return;
        }
        s_bindMap[code] = command;
    }

    void Unbind(const std::string& keyName)
    {
        SDL_Scancode code = SDL_GetScancodeFromName(keyName.c_str());
        if (code != SDL_SCANCODE_UNKNOWN)
        {
            s_bindMap.erase(code);
        }
    }

    CON_COMMAND(bind, "Binds a key to a command: bind <key> <command>")
    {
        if (args.size() < 3)
        {
            Console::Log("Usage: bind <key> <command>");
            return;
        }
        SetBind(args[1], args[2]);
        Save();
    }

    CON_COMMAND(unbind, "Unbinds a key: unbind <key>")
    {
        if (args.size() < 2)
        {
            Console::Log("Usage: unbind <key>");
            return;
        }
        Unbind(args[1]);
        Save();
    }
}