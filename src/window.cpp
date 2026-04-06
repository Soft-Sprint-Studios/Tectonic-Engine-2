#include "window.h"
#include "console.h"
#include "cvar.h"
#include <glad/glad.h>

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
    if (CVar::Find("r_multisample")->GetInt() > 0)
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, CVar::Find("r_multisample_samples")->GetInt());
    }
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    m_window = SDL_CreateWindow(title, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
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

    SDL_GL_SetSwapInterval(CVar::Find("r_vsync")->GetInt());

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