#pragma once
#include <string>
#include <vector>

namespace Networking
{
    void Init();
    void Update();
    void Shutdown();

    void Ping(const std::string& host);
    void Download(const std::string& url, const std::string& savePath);
}