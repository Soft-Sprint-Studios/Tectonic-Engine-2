#include "input.h"

void Input::BeginFrame()
{
    m_pressed.clear();
    m_mouseDeltaX = 0.0f;
    m_mouseDeltaY = 0.0f;
}

void Input::ProcessEvent(const SDL_Event& e)
{
    if (e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP)
    {
        bool isDown = e.key.down;
        SDL_Scancode sc = e.key.scancode;

        m_keys[sc] = isDown;

        if (isDown && !e.key.repeat)
            m_pressed[sc] = true;
    }
    else if (e.type == SDL_EVENT_MOUSE_MOTION)
    {
        m_mouseDeltaX += (float)e.motion.xrel;
        m_mouseDeltaY += (float)e.motion.yrel;
    }
}

bool Input::GetKey(SDL_Scancode key) const
{
    auto it = m_keys.find(key);
    return it != m_keys.end() && it->second;
}

bool Input::GetKeyDown(SDL_Scancode key) const
{
    auto it = m_pressed.find(key);
    return it != m_pressed.end() && it->second;
}

float Input::GetMouseDeltaX() const
{
    return m_mouseDeltaX;
}

float Input::GetMouseDeltaY() const
{
    return m_mouseDeltaY;
}