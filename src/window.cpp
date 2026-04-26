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
#include "window.h"
#include "console.h"
#include "cvar.h"
#include <glad/glad.h>

CVar r_fullscreen("r_fullscreen", "1", "Enable fullscreen mode.", CVAR_SAVE);
CVar r_vsync("r_vsync", "1", "Enable vertical synchronization.", CVAR_SAVE);
CVar r_multisample("r_multisample", "1", "Enable MSAA anti-aliasing.", CVAR_SAVE);
CVar r_multisample_samples("r_multisample_samples", "4", "Number of MSAA samples.", CVAR_SAVE);

bool Window::Init(const char* title, int width, int height)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        Console::Error("SDL_Init Error: " + std::string(SDL_GetError()));
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    if (r_multisample.GetInt() > 0)
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, r_multisample_samples.GetInt());
    }
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    if (r_fullscreen.GetInt() > 0)
    {
        const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
        if (mode)
        {
            width = mode->w;
            height = mode->h;
        }
        flags |= SDL_WINDOW_BORDERLESS;
    }

    m_window = SDL_CreateWindow(title, width, height, flags);
    if (!m_window)
    {
        Console::Error("Window Creation Error: " + std::string(SDL_GetError()));
        return false;
    }

    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext)
    {
        Console::Error("OpenGL Context Error: " + std::string(SDL_GetError()));
        return false;
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        Console::Error("Failed to initialize GLAD");
        return false;
    }

    SDL_GL_SetSwapInterval(r_vsync.GetInt());

    SDL_SetWindowRelativeMouseMode(m_window, true);

    return true;
}

void Window::Swap()
{
    SDL_GL_SwapWindow(m_window);
}

void Window::Shutdown()
{
    if (m_glContext)
    {
        SDL_GL_DestroyContext(m_glContext);
    }

    if (m_window)
    {
        SDL_DestroyWindow(m_window);
    }

    SDL_Quit();
}