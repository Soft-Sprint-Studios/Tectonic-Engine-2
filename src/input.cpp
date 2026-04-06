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