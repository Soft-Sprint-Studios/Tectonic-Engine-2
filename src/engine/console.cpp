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
#include "renderer.h"
#include "r_ui.h"
#include "timing.h"
#include <cmath>
#include <vector>
#include <string>
#include <glm/glm.hpp>

namespace Console
{
    CVar sv_cheats("sv_cheats", "0", "Allow cheat commands and variables.", CVAR_SAVE);

    struct ConsoleLine
    {
        std::string text;
        glm::vec4 color;
    };

    static std::vector<ConsoleLine> s_history;
    static std::string s_inputBuffer;
    static bool s_opened = false;
    static float s_animPos = 0.0f;

    // Colors
    static const glm::vec4 COL_NORMAL = { 1.0f, 1.0f, 1.0f, 1.0f }; // White
    static const glm::vec4 COL_WARN = { 1.0f, 1.0f, 0.0f, 1.0f }; // Yellow
    static const glm::vec4 COL_ERROR = { 1.0f, 0.0f, 0.0f, 1.0f }; // Red
    static const glm::vec4 COL_INPUT = { 0.0f, 1.0f, 1.0f, 1.0f }; // Cyan

    static std::vector<std::string> SplitArgs(const std::string& command)
    {
        std::vector<std::string> args;
        std::string current;
        bool inQuotes = false;

        for (char c : command)
        {
            if (c == '\"')
            {
                inQuotes = !inQuotes;
            }
            else if (c == ' ' && !inQuotes)
            {
                if (!current.empty())
                {
                    args.push_back(current);
                }
                current.clear();
            }
            else
            {
                current += c;
            }
        }
        if (!current.empty())
        {
            args.push_back(current);
        }
        return args;
    }

    void Init()
    {
        Log("Tectonic Console Initialized.");
    }

    void Update()
    {
    }

    void Shutdown()
    {
        s_history.clear();
    }

    void Toggle()
    {
        s_opened = !s_opened;
        if (s_opened)
        {
            SDL_StartTextInput(SDL_GL_GetCurrentWindow());
        }
        else
        {
            SDL_StopTextInput(SDL_GL_GetCurrentWindow());
        }
    }

    bool IsOpen()
    {
        return s_opened;
    }

    bool HandleEvent(const SDL_Event& e)
    {
        if (!s_opened) 
            return false;

        if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) 
            return false;

        if (e.type == SDL_EVENT_TEXT_INPUT)
        {
            s_inputBuffer += e.text.text;
            return true;
        }

        if (e.type == SDL_EVENT_KEY_DOWN)
        {
            if (e.key.key == SDLK_BACKSPACE)
            {
                if (!s_inputBuffer.empty()) 
                    s_inputBuffer.pop_back();
                return true;
            }
            else if (e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER)
            {
                if (!s_inputBuffer.empty())
                {
                    Log("> " + s_inputBuffer);
                    s_history.back().color = COL_INPUT;

                    Execute(s_inputBuffer);
                    s_inputBuffer.clear();
                }
                return true;
            }

            return true;
        }

        return false;
    }

    void Draw(Renderer* renderer)
    {
        float target = s_opened ? 1.0f : 0.0f;

        if (std::abs(s_animPos - target) > 0.001f)
        {
            s_animPos = glm::mix(s_animPos, target, 0.15f);
        }
        else
        {
            s_animPos = target;
        }

        if (s_animPos <= 0.0f)
        {
            return;
        }

        auto ui = renderer->GetUI();
        int w, h;
        SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &w, &h);

        float conH = (float)h * 0.45f;
        float yOffset = (s_animPos - 1.0f) * conH;

        ui->DrawRect(0, yOffset, (float)w, conH, { 0.1f, 0.1f, 0.1f, 0.95f });
        ui->DrawRect(0, yOffset + conH - 3.0f, (float)w, 3.0f, { 1.0f, 0.5f, 0.0f, 1.0f });

        // Draw history
        float y = yOffset + conH - 55.0f;
        for (int i = (int)s_history.size() - 1; i >= 0; --i)
        {
            ui->DrawText(s_history[i].text, 10.0f, y, s_history[i].color);
            y -= 20.0f;
            if (y < yOffset)
            {
                break;
            }
        }

        bool showCursor = std::fmod(Time::TotalTime(), 0.8f) < 0.4f;
        std::string cursor = showCursor ? "_" : "";

        ui->DrawText("] " + s_inputBuffer + cursor, 10.0f, yOffset + conH - 32.0f, COL_NORMAL);
    }

    void Execute(const std::string& fullLine)
    {
        std::vector<std::string> args = SplitArgs(fullLine);
        if (args.empty())
        {
            return;
        }

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

    void Log(const std::string& message)
    {
        ConsoleLine line;
        line.text = message;
        line.color = COL_NORMAL;
        s_history.push_back(line);

        if (s_history.size() > 128)
        {
            s_history.erase(s_history.begin());
        }
    }

    void Warn(const std::string& message)
    {
        ConsoleLine line;
        line.text = message;
        line.color = COL_WARN;
        s_history.push_back(line);
    }

    void Error(const std::string& message)
    {
        ConsoleLine line;
        line.text = message;
        line.color = COL_ERROR;
        s_history.push_back(line);
    }

    CON_COMMAND(clear, "Clears the console history")
    {
        s_history.clear();
    }
}