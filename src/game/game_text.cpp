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
#include "entities.h"
#include "screen_text.h"

class GameText : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);

        m_message = GetValue("message", "");
        m_x = GetFloat("x", -1.0f);
        m_y = GetFloat("y", -1.0f);
        
        glm::vec3 col = GetVector("color", { 255, 255, 255 });
        m_color = glm::vec4(col / 255.0f, 1.0f);

        m_fadeIn = GetFloat("fadein", 0.5f);
        m_holdTime = GetFloat("holdtime", 4.0f);
        m_fadeOut = GetFloat("fadeout", 0.5f);
        m_scale = GetFloat("scale", 1.0f);
    }

    void AcceptInput(const std::string& inputName, const std::string& parameter) override
    {
        Entity::AcceptInput(inputName, parameter);

        if (inputName == "Display")
        {
            if (IsEnabled() && !m_message.empty())
            {
                ScreenText::Add(m_message, m_x, m_y, m_color, m_fadeIn, m_holdTime, m_fadeOut, m_scale);
            }
        }
    }

    bool IsCollidable() const override
    {
        return false;
    }

private:
    std::string m_message;
    float m_x, m_y;
    glm::vec4 m_color;
    float m_fadeIn, m_holdTime, m_fadeOut, m_scale;
};

LINK_ENTITY_TO_CLASS("game_text", GameText)