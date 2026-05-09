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
#include "fade.h"
#include <algorithm>

namespace Fade
{
    enum class FadeState
    {
        None,
        Fading,
        Holding
    };

    static glm::vec4 s_color{ 0, 0, 0, 0 };
    static float s_duration = 0.0f;
    static float s_holdTime = 0.0f;
    static float s_elapsed = 0.0f;
    static bool s_isFadeIn = false;
    static FadeState s_state = FadeState::None;

    void Start(const glm::vec4& color, float duration, float holdTime, bool fadeIn)
    {
        s_color = color;
        s_duration = std::max(0.001f, duration);
        s_holdTime = holdTime;
        s_isFadeIn = fadeIn;
        s_elapsed = 0.0f;
        s_state = FadeState::Fading;
    }

    void Update(float dt)
    {
        if (s_state == FadeState::None)
        {
            return;
        }

        s_elapsed += dt;

        if (s_state == FadeState::Fading)
        {
            if (s_elapsed >= s_duration)
            {
                s_elapsed = 0.0f;
                s_state = (s_holdTime > 0.0f) ? FadeState::Holding : FadeState::None;
                
                // If we were fading in (to transparency), we are done.
                if (s_isFadeIn && s_holdTime <= 0.0f)
                {
                    s_state = FadeState::None;
                }
            }
        }
        else if (s_state == FadeState::Holding)
        {
            if (s_elapsed >= s_holdTime)
            {
                s_state = FadeState::None;
                s_elapsed = 0.0f;
            }
        }
    }

    glm::vec4 GetCurrentFade()
    {
        if (s_state == FadeState::None)
        {
            return glm::vec4(0.0f);
        }

        float alpha = s_color.a;

        if (s_state == FadeState::Fading)
        {
            float t = std::clamp(s_elapsed / s_duration, 0.0f, 1.0f);
            alpha = s_isFadeIn ? glm::mix(s_color.a, 0.0f, t) : glm::mix(0.0f, s_color.a, t);
        }
        
        return glm::vec4(s_color.r, s_color.g, s_color.b, alpha);
    }
}