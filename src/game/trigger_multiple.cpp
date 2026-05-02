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

class TriggerMultiple : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);
        m_wait = GetFloat("wait", 0.0f);
        m_nextFireTime = 0.0f;
    }

    void OnSave() override
    {
        Entity::OnSave();
        AddSaveField(DATA_FIELD(TriggerMultiple, m_wait, FieldType::Float));
        AddSaveField(DATA_FIELD(TriggerMultiple, m_nextFireTime, FieldType::Float));
        AddSaveField(DATA_FIELD(TriggerMultiple, m_disabled, FieldType::Bool));
    }

    void Touch(Entity* other) override
    {
        if (m_disabled) 
            return;

        if (other && other->IsPlayer())
        {
            if (Time::TotalTime() >= m_nextFireTime)
            {
                FireOutput("OnTrigger");

                // If wait is -1 it behaves like a trigger_once
                if (m_wait < 0)
                {
                    m_disabled = true;
                }
                else
                {
                    m_nextFireTime = (float)Time::TotalTime() + m_wait;
                }
            }
        }
    }

    void AcceptInput(const std::string& input, const std::string& param) override
    {
        if (input == "Enable") 
            m_disabled = false;
        if (input == "Disable") 
            m_disabled = true;
        if (input == "Toggle") 
            m_disabled = !m_disabled;
    }

    bool IsCollidable() const override 
    { 
        return false; 
    }

protected:
    float m_wait = 0.0f;
    float m_nextFireTime = 0.0f;
    bool m_disabled = false;
};

LINK_ENTITY_TO_CLASS("trigger_multiple", TriggerMultiple)