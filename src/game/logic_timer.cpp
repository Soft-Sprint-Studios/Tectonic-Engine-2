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
#include "timing.h"

class LogicTimer : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);
        m_enabled = !HasSpawnFlag(1);
        m_refireTime = GetFloat("refire", 1.0f);
        m_nextFire = (float)Time::TotalTime() + m_refireTime;
    }

    void OnSave() override
    {
        Entity::OnSave();
        AddSaveField(DATA_FIELD(LogicTimer, m_enabled, FieldType::Bool));
        AddSaveField(DATA_FIELD(LogicTimer, m_nextFire, FieldType::Float));
    }

    void Think(float deltaTime) override
    {
        if (!m_enabled)
        {
            return;
        }

        if (Time::TotalTime() >= m_nextFire)
        {
            FireOutput("OnTimer");
            m_nextFire = (float)Time::TotalTime() + m_refireTime;
        }
    }

    void AcceptInput(const std::string& input, const std::string& param) override
    {
        if (input == "Enable")
        {
            m_enabled = true;
        }
        else if (input == "Disable")
        {
            m_enabled = false;
        }
        else if (input == "Toggle")
        {
            m_enabled = !m_enabled;
        }
        else if (input == "SetTimer" && !param.empty())
        {
            m_refireTime = std::stof(param);
        }
    }

    bool IsCollidable() const override
    {
        return false;
    }

private:
    bool m_enabled = true;
    float m_refireTime = 1.0f;
    float m_nextFire = 0.0f;
};

LINK_ENTITY_TO_CLASS("logic_timer", LogicTimer)