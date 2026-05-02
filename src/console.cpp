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
#include "platform.h"
#include "console.h"
#include "cvar.h"
#include "concmd.h"
#include <thread>
#include <queue>
#include <mutex>
#include <atomic>
#include <vector>
#include <chrono>

#ifdef PLATFORM_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <shellapi.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET socket_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
typedef int socket_t;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

namespace Console
{
    CVar sv_cheats("sv_cheats", "0", "Allow cheat commands and variables.", CVAR_SAVE);

    static std::queue<std::string> s_commandQueue;
    static std::mutex s_consoleMutex;
    static std::thread s_networkThread;
    static std::atomic<bool> s_running{ false };

    static socket_t s_clientSocket = INVALID_SOCKET;
    static std::atomic<bool> s_connected{ false };

    static std::vector<std::string> SplitArgs(const std::string& command)
    {
        std::vector<std::string> args;
        std::string current;
        bool inQuotes = false;

        for (char c : command)
        {
            if (c == '\"') 
                inQuotes = !inQuotes;
            else if (c == ' ' && !inQuotes)
            {
                if (!current.empty()) 
                    args.push_back(current);
                current.clear();
            }
            else 
                current += c;
        }
        if (!current.empty()) 
            args.push_back(current);
        return args;
    }

    void SendRemote(const std::string& text)
    {
        if (s_connected && s_clientSocket != INVALID_SOCKET)
        {
            std::string msg = text + "\n";
            send(s_clientSocket, msg.c_str(), (int)msg.length(), 0);
        }
    }

    static void NetworkLoop()
    {
#ifdef PLATFORM_WINDOWS
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

        while (s_running)
        {
            if (!s_connected)
            {
                s_clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

                sockaddr_in serverAddr;
                serverAddr.sin_family = AF_INET;
                serverAddr.sin_port = htons(28016);
                inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

                if (connect(s_clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) != SOCKET_ERROR)
                {
                    s_connected = true;

                    char handshake[3];
                    recv(s_clientSocket, handshake, 2, 0);

                    // Sync CVars
                    for (auto const& [name, cvar] : CVar::GetRegistry())
                    {
                        SendRemote("register_cvar \"" + name + "\" \"" + cvar->GetString() + "\" \"" + cvar->GetDescription() + "\"");
                    }

                    // Sync ConCmds
                    for (auto const& [name, cmd] : ConCmd::GetRegistry())
                    {
                        SendRemote("register_cmd \"" + name + "\" \"" + cmd->GetDescription() + "\"");
                    }
                }
                else
                {
                    closesocket(s_clientSocket);
                    s_clientSocket = INVALID_SOCKET;
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    continue;
                }
            }

            char buffer[2048];
            int bytesReceived = recv(s_clientSocket, buffer, sizeof(buffer) - 1, 0);

            if (bytesReceived > 0)
            {
                buffer[bytesReceived] = '\0';
                std::string incoming(buffer);
                size_t pos = 0;
                while ((pos = incoming.find('\n')) != std::string::npos)
                {
                    std::string line = incoming.substr(0, pos);
                    if (!line.empty())
                    {
                        std::lock_guard<std::mutex> lock(s_consoleMutex);
                        s_commandQueue.push(line);
                    }
                    incoming.erase(0, pos + 1);
                }
            }
            else
            {
                s_connected = false;
                closesocket(s_clientSocket);
                s_clientSocket = INVALID_SOCKET;
            }
        }

#ifdef PLATFORM_WINDOWS
        WSACleanup();
#endif
    }

    void Init()
    {
        s_running = true;
        s_networkThread = std::thread(NetworkLoop);
    }

    void Execute(const std::string& fullLine)
    {
        std::vector<std::string> args = SplitArgs(fullLine);
        if (args.empty()) 
            return;

        std::string name = args[0];
        if (ConCmd* cmd = ConCmd::Find(name))
        {
            cmd->Execute(args);
            return;
        }

        if (CVar* var = CVar::Find(name))
        {
            if (args.size() > 1)
            {
                if (var->IsCheat() && sv_cheats.GetInt() == 0)
                {
                    Warn("CVar '" + name + "' is cheat protected (sv_cheats must be 1)");
                    return;
                }

                CVar::Set(name, args[1]);
                Log(name + " = " + args[1]);
            }
            else
            {
                Log(name + " is \"" + var->GetString() + "\"");
            }
            return;
        }
        Warn("Unknown command or variable: " + name);
    }

    void ToggleExternal()
    {
        if (!s_connected)
        {
#ifdef PLATFORM_WINDOWS
            ShellExecuteA(NULL, "open", "TConsole.exe", NULL, NULL, SW_SHOWNORMAL);
#else
            (void)system("./TConsole &");
#endif
        }
        else
        {
            SendRemote("_toggle_window");
        }
    }

    void Update()
    {
        std::lock_guard<std::mutex> lock(s_consoleMutex);
        while (!s_commandQueue.empty())
        {
            std::string fullLine = s_commandQueue.front();
            s_commandQueue.pop();

            std::vector<std::string> args = SplitArgs(fullLine);
            if (args.empty()) 
                continue;

            Execute(fullLine);
        }
    }

    void Shutdown()
    {
        s_running = false;
        if (s_clientSocket != INVALID_SOCKET) 
            closesocket(s_clientSocket);
        if (s_networkThread.joinable()) 
            s_networkThread.detach();
    }

    void Log(const std::string& message)
    {
        SendRemote("[LOG] " + message);
    }

    void Warn(const std::string& message)
    {
        SendRemote("[WARNING] " + message);
    }

    void Error(const std::string& message)
    {
        SendRemote("[ERROR] " + message);
    }
}