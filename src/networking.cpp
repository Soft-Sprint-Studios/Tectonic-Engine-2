#include "networking.h"
#include "console.h"
#include "concmd.h"
#include "filesystem.h"
#include "timing.h"
#include <thread>
#include <atomic>
#include <algorithm>
#include <cstdio>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef SOCKET socket_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    typedef int socket_t;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

namespace Networking
{
    void Init()
    {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    }

    void Shutdown()
    {
#ifdef _WIN32
        WSACleanup();
#endif
    }

    void Update()
    {
    }

    void Ping(const std::string& host)
    {
        // Run in thread to not block engine
        std::thread([host]() {
            addrinfo hints = {}, *res;
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;

            float startTime = Time::TotalTime();

            if (getaddrinfo(host.c_str(), "80", &hints, &res) != 0) {
                Console::Error("Ping: Failed to resolve " + host);
                return;
            }

            socket_t s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
            if (connect(s, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR) {
                Console::Error("Ping: Connection failed to " + host);
            } else {
                float endTime = Time::TotalTime();
                int ms = (int)((endTime - startTime) * 1000.0f);
                Console::Log("Ping to " + host + ": " + std::to_string(ms) + "ms (TCP)");
            }

            closesocket(s);
            freeaddrinfo(res);
        }).detach();
    }

    void Download(const std::string& url, const std::string& savePath)
    {
        std::thread([url, savePath]() {
            std::string host, path;
            size_t start = url.find("://");
            size_t hostStart = (start == std::string::npos) ? 0 : start + 3;
            size_t pathStart = url.find('/', hostStart);

            if (pathStart == std::string::npos) {
                host = url.substr(hostStart);
                path = "/";
            } else {
                host = url.substr(hostStart, pathStart - hostStart);
                path = url.substr(pathStart);
            }

            addrinfo hints = {}, *res;
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;

            if (getaddrinfo(host.c_str(), "80", &hints, &res) != 0) {
                Console::Error("Download: Host resolve failed");
                return;
            }

            socket_t s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
            if (connect(s, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR) {
                Console::Error("Download: Connect failed");
                return;
            }

            std::string request = "GET " + path + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
            send(s, request.c_str(), (int)request.length(), 0);

            Console::Log("Downloading " + url + "...");

            std::vector<char> response;
            char buffer[4096];
            int bytes;
            while ((bytes = recv(s, buffer, sizeof(buffer), 0)) > 0) {
                response.insert(response.end(), buffer, buffer + bytes);
            }

            auto it = std::search(response.begin(), response.end(), "\r\n\r\n", "\r\n\r\n" + 4);
            if (it != response.end()) {
                std::vector<uint8_t> body(it + 4, response.end());

                FILE* f = fopen(Filesystem::GetFullPath(savePath).c_str(), "wb");
                if (f) {
                    fwrite(body.data(), 1, body.size(), f);
                    fclose(f);
                    Console::Log("Download finished: " + savePath + " (" + std::to_string(body.size()) + " bytes)");
                } else {
                    Console::Error("Download: Failed to write to disk");
                }
            }

            closesocket(s);
            freeaddrinfo(res);
        }).detach();
    }

    CON_COMMAND(ping, "Pings a host via TCP: ping <host>")
    {
        if (args.size() < 2)
        {
            Console::Log("Usage: ping <host>");
            return;
        }
        Ping(args[1]);
    }

    CON_COMMAND(download, "Downloads a file: download <url> <local_path>")
    {
        if (args.size() < 3)
        {
            Console::Log("Usage: download <url> <local_path>");
            return;
        }
        Download(args[1], args[2]);
    }
}