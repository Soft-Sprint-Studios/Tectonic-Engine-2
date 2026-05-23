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
#include "fade.h"

class EnvFade : public Entity
{
public:
    void Spawn(const BSP::EntityData& entData) override
    {
        Entity::Spawn(entData);
        m_duration = GetFloat("duration", 2.0f);
        m_holdTime = GetFloat("holdtime", 0.0f);
        
        glm::vec3 col = GetVector("rendercolor", { 0, 0, 0 });
        float alpha = GetFloat("renderamt", 255.0f) / 255.0f;
        m_fadeColor = glm::vec4(col / 255.0f, alpha);
    }

    void AcceptInput(const std::string& input, const std::string& param) override
    {
        Entity::AcceptInput(input, param);
        if (input == "Fade")
        {
            bool fadeIn = HasSpawnFlag(2);
            Fade::Start(m_fadeColor, m_duration, m_holdTime, fadeIn);
        }
    }

    bool IsCollidable() const override
    {
        return false;
    }

private:
    float m_duration = 2.0f;
    float m_holdTime = 0.0f;
    glm::vec4 m_fadeColor{ 0, 0, 0, 1 };
};

LINK_ENTITY_TO_CLASS("env_fade", EnvFade)