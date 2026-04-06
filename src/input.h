#pragma once
#include <SDL3/SDL.h>
#include <unordered_map>

class Input
{
public:
    void BeginFrame();
    void ProcessEvent(const SDL_Event& e);

    bool GetKey(SDL_Scancode key) const;
    bool GetKeyDown(SDL_Scancode key) const;

    float GetMouseDeltaX() const;
    float GetMouseDeltaY() const;

private:
    std::unordered_map<SDL_Scancode, bool> m_keys;
    std::unordered_map<SDL_Scancode, bool> m_pressed;

    float m_mouseDeltaX = 0.0f;
    float m_mouseDeltaY = 0.0f;
};