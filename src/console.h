#pragma once
#include <string>

namespace Console
{
    void Init();
    void Update();
    void Shutdown();

    void Log(const std::string& message);
    void Warn(const std::string& message);
    void Error(const std::string& message);

    void SendRemote(const std::string& text);
    void Execute(const std::string& command);
}