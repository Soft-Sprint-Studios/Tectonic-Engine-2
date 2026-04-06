#pragma once
#include <SDL3/SDL.h>

class Window
{
public:
    bool Init(const char* title, int width, int height);
    void Shutdown();
    void Swap();

    SDL_Window* Get() const
    {
        return m_window;
    }

private:
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;
};