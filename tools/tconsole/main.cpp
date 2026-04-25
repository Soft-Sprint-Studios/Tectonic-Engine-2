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
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Box.H>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
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

#define TCONSOLE_PORT 28016
#define BUFFER_SIZE 4096

Fl_Double_Window* window = nullptr;
Fl_Text_Display* text_display = nullptr;
Fl_Text_Buffer* text_buffer = nullptr;
Fl_Text_Buffer* style_buffer = nullptr;
Fl_Input* input_field = nullptr;
Fl_Button* send_button = nullptr;
Fl_Menu_Bar* menu_bar = nullptr;
Fl_Box* status_bar = nullptr;

Fl_Text_Display::Style_Table_Entry style_table[] = {
    { FL_WHITE, FL_COURIER, 14 }, // A - Normal
    { FL_YELLOW, FL_COURIER, 14 }, // B - Warning
    { FL_RED, FL_COURIER, 14 }, // C - Error
    { FL_CYAN, FL_COURIER_BOLD, 14}, // D - User Input
};

struct CvarInfo {
    std::string name;
    std::string value;
    std::string description;
};

struct CommandInfo {
    std::string name;
    std::string description;
};

socket_t server_socket = INVALID_SOCKET;
socket_t client_socket = INVALID_SOCKET;
std::vector<std::string> message_queue;
std::vector<CvarInfo> g_cvars;
std::vector<CommandInfo> g_commands;
std::mutex queue_mutex;
std::thread server_thread;
bool should_exit = false;

void append_message(const std::string& msg, char style_char = 'A') {
    std::string clean_msg = msg;

    if (msg.rfind("[ERROR]", 0) == 0) {
        style_char = 'C';
        clean_msg = msg.substr(7);
    }
    else if (msg.rfind("[WARNING]", 0) == 0) {
        style_char = 'B';
        clean_msg = msg.substr(9);
    }
    else if (msg.rfind("[LOG]", 0) == 0) {
        style_char = 'A';
        clean_msg = msg.substr(5);
    }
    else if (msg.rfind("[TConsole]", 0) == 0) {
        style_char = 'D';
        clean_msg = msg.substr(10);
    }
    else if (msg.rfind("> ", 0) == 0) {
        style_char = 'D';
    }

    if (!clean_msg.empty() && clean_msg[0] == ' ')
        clean_msg.erase(0, 1);

    text_buffer->append(clean_msg.c_str());
    text_buffer->append("\n");

    std::string style_line(clean_msg.length(), style_char);
    style_buffer->append(style_line.c_str());
    style_buffer->append("\n");

    text_display->scroll(text_display->count_lines(0, text_buffer->length(), 1), 0);
}

class ConsoleInput : public Fl_Input {
public:
    ConsoleInput(int X, int Y, int W, int H)
        : Fl_Input(X, Y, W, H) {
    }

    int handle(int event) override {
        if (event == FL_KEYBOARD && Fl::event_key() == FL_Tab) {
            const char* current_text = value();
            int current_len = (int)strlen(current_text);

            if (current_len == 0) 
                return 1;

            std::vector<std::string> matches;
            for (const auto& cvar : g_cvars) {
                if (cvar.name.rfind(current_text, 0) == 0) {
                    matches.push_back(cvar.name);
                }
            }
            for (const auto& cmd : g_commands) {
                if (cmd.name.rfind(current_text, 0) == 0) {
                    matches.push_back(cmd.name);
                }
            }

            if (matches.empty()) 
                return 1;

            if (matches.size() == 1) {
                value(matches[0].c_str());
                insert(" ");
                position(size());
            }
            else {
                append_message("> " + std::string(current_text), 'D');
                for (const auto& match : matches) {
                    append_message("  " + match);
                }
            }
            return 1;
        }
        return Fl_Input::handle(event);
    }
};

void idle_callback(void*) {
    std::vector<std::string> local_queue;
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        if (!message_queue.empty()) {
            local_queue.swap(message_queue);
        }
    }

    for (const auto& msg : local_queue) {
        if (msg.rfind("register_cvar", 0) == 0) {
            char name[64], value_str[128], desc[512];
            if (sscanf(msg.c_str(), "register_cvar \"%63[^\"]\" \"%127[^\"]\" \"%511[^\"]\"", name, value_str, desc) == 3) {
                g_cvars.push_back({ name, value_str, desc });
            }
        }
        else if (msg.rfind("register_cmd", 0) == 0) {
            char name[64], desc[128];
            if (sscanf(msg.c_str(), "register_cmd \"%63[^\"]\" \"%127[^\"]\"", name, desc) == 2) {
                g_commands.push_back({ name, desc });
            }
        }
        else {
            append_message(msg);
        }
    }

    static bool last_connected_state = false;
    if (last_connected_state != (client_socket != INVALID_SOCKET)) {
        last_connected_state = (client_socket != INVALID_SOCKET);
        if (last_connected_state) {
            status_bar->label("Engine Connected.");
        }
        else {
            status_bar->label("Waiting for engine connection...");
        }
    }

    Fl::repeat_timeout(0.05, idle_callback);
}

void send_command_callback(Fl_Widget*, void*) {
    const char* command = input_field->value();
    std::string cmd_str = command;

    if (cmd_str == "help") {
        append_message("> help", 'D');
        append_message("=== Available Commands ===", 'B');
        for (const auto& cmd : g_commands) {
            append_message("  " + cmd.name + " : " + cmd.description);
        }

        append_message("=== Available CVars ===", 'B');
        for (const auto& cvar : g_cvars) {
            char info[1024];
            snprintf(info, sizeof(info), "  %-25s - %s", cvar.name.c_str(), cvar.description.c_str());
            append_message(info);
        }

        input_field->value("");
        input_field->take_focus();
        return;
    }

    if (strlen(command) > 0 && client_socket != INVALID_SOCKET) {
        std::string full_command = std::string(command) + "\n";
        send(client_socket, full_command.c_str(), (int)full_command.length(), 0);
        append_message("> " + std::string(command), 'D');
        input_field->value("");
    }
    else if (strlen(command) > 0) {
        append_message("[!] Engine not connected. Command not sent: " + std::string(command), 'C');
    }
    input_field->take_focus();
}

void input_callback(Fl_Widget*, void*) {
    if (Fl::event_key() == FL_Enter || Fl::event_key() == FL_KP_Enter) {
        send_command_callback(nullptr, nullptr);
    }
}

void on_window_close(Fl_Widget*, void*) {
    should_exit = true;

    if (server_socket != INVALID_SOCKET) {
#ifdef _WIN32
        shutdown(server_socket, SD_BOTH);
#else
        shutdown(server_socket, SHUT_RDWR);
#endif
        closesocket(server_socket);
        server_socket = INVALID_SOCKET;
    }

    if (client_socket != INVALID_SOCKET) {
#ifdef _WIN32
        shutdown(client_socket, SD_BOTH);
#else
        shutdown(client_socket, SHUT_RDWR);
#endif
        closesocket(client_socket);
        client_socket = INVALID_SOCKET;
    }

    if (server_thread.joinable()) {
        server_thread.join();
    }

    window->hide();
}

void on_quit_cb(Fl_Widget* w, void* data) {
    on_window_close(w, data);
}

void on_clear_cb(Fl_Widget*, void*) {
    text_buffer->text("");
    style_buffer->text("");
}

void on_about_cb(Fl_Widget*, void*) {
    fl_message_title("About Tectonic Console 2");
    fl_message("A remote console for the Tectonic Engine 2.\n\n"
        "Copyright (c) 2025-2026 Soft Sprint Studios");
}

void server_loop() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) 
        return;

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TCONSOLE_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        closesocket(server_socket);
        return;
    }

    listen(server_socket, 1);

    while (!should_exit) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_len);

        if (client_socket == INVALID_SOCKET) {
            if (should_exit) 
                break;
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            message_queue.push_back("[TConsole] Engine connected.");
        }
        Fl::awake();

        send(client_socket, "ok", 2, 0);

        char buffer[BUFFER_SIZE];
        int bytes_received;
        std::string line_buffer;

        while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
            buffer[bytes_received] = '\0';
            line_buffer += buffer;

            size_t pos;
            while ((pos = line_buffer.find('\n')) != std::string::npos) {
                std::string line = line_buffer.substr(0, pos);
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }

                if (!line.empty()) {
                    std::lock_guard<std::mutex> lock(queue_mutex);
                    message_queue.push_back(line);
                }
                Fl::awake();
                line_buffer.erase(0, pos + 1);
            }
        }

        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            message_queue.push_back("[TConsole] Engine disconnected.");
            g_cvars.clear();
            g_commands.clear();
        }
        Fl::awake();
        closesocket(client_socket);
        client_socket = INVALID_SOCKET;
    }

#ifdef _WIN32
    WSACleanup();
#endif
}

int main(int argc, char** argv) {
    window = new Fl_Double_Window(800, 600, "Tectonic Console 2");
    window->callback(on_window_close);

    menu_bar = new Fl_Menu_Bar(0, 0, 800, 25);
    menu_bar->add("File/Quit", FL_CTRL + 'q', on_quit_cb, window);
    menu_bar->add("Edit/Clear", FL_CTRL + 'l', on_clear_cb, window);
    menu_bar->add("Help/About", 0, on_about_cb, window);

    text_buffer = new Fl_Text_Buffer();
    style_buffer = new Fl_Text_Buffer();

    text_display = new Fl_Text_Display(10, 35, 780, 515);
    text_display->buffer(text_buffer);
    text_display->highlight_data(style_buffer, style_table, sizeof(style_table) / sizeof(style_table[0]), 'A', 0, 0);
    text_display->color(FL_BLACK);
    text_display->textcolor(FL_WHITE);
    text_display->textfont(FL_COURIER);
    text_display->textsize(14);

    input_field = new ConsoleInput(10, 560, 700, 30);
    input_field->callback(input_callback);
    input_field->when(FL_WHEN_ENTER_KEY | FL_WHEN_RELEASE);

    send_button = new Fl_Button(720, 560, 70, 30, "Send");
    send_button->callback(send_command_callback);

    status_bar = new Fl_Box(0, 590, 800, 10, "Waiting for engine connection...");
    status_bar->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    status_bar->box(FL_FLAT_BOX);

    window->resizable(text_display);
    window->end();
    window->show(argc, argv);

    Fl::add_timeout(0.05, idle_callback);

    server_thread = std::thread(server_loop);

    return Fl::run();
}