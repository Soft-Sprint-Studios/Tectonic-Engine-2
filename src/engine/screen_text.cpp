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
#include "screen_text.h"
#include "renderer.h"
#include "r_ui.h"
#include <vector>
#include <algorithm>

namespace ScreenText
{
    struct Message
    {
        std::string text;
        float x, y;
        glm::vec4 color;
        float fadeIn, hold, fadeOut;
        float elapsed;
        float scale;
    };

    static std::vector<Message> s_messages;

    void Add(const std::string& text, float x, float y, const glm::vec4& color, float fadeIn, float hold, float fadeOut, float scale)
    {
        s_messages.push_back({ text, x, y, color, fadeIn, hold, fadeOut, 0.0f, scale });
    }

    void Update(float dt)
    {
        for (auto it = s_messages.begin(); it != s_messages.end();)
        {
            it->elapsed += dt;
            if (it->elapsed >= (it->fadeIn + it->hold + it->fadeOut))
            {
                it = s_messages.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void Draw(Renderer* renderer)
    {
        auto ui = renderer->GetUI();
        int screenW, screenH;
        SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &screenW, &screenH);

        for (auto& msg : s_messages)
        {
            float alpha = 1.0f;
            if (msg.elapsed < msg.fadeIn)
            {
                alpha = msg.elapsed / msg.fadeIn;
            }
            else if (msg.elapsed > (msg.fadeIn + msg.hold))
            {
                float fadeElapsed = msg.elapsed - (msg.fadeIn + msg.hold);
                alpha = 1.0f - (fadeElapsed / msg.fadeOut);
            }

            glm::vec4 drawCol = msg.color;
            drawCol.a *= std::clamp(alpha, 0.0f, 1.0f);

            // -1 indicates centered text
            float drawX = (msg.x == -1.0f) ? (screenW * 0.5f) : (screenW * msg.x);
            float drawY = (msg.y == -1.0f) ? (screenH * 0.5f) : (screenH * msg.y);

            ui->DrawText(msg.text, drawX, drawY, drawCol, msg.scale);
        }
    }

    void Clear()
    {
        s_messages.clear();
    }
}