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

class TriggerRepeat : public Entity
{
public:
    void Spawn(const BSP::EntityData& entData) override
    {
        Entity::Spawn(entData);
        m_delay = GetFloat("delay", 1.0f);
        m_totalRepeats = GetInt("repeats", -1);
        m_repeatsRemaining = 0;
        m_nextFireTime = 0.0f;
    }

    void OnSave() override
    {
        Entity::OnSave();
        AddSaveField(DATA_FIELD(TriggerRepeat, m_delay, FieldType::Float));
        AddSaveField(DATA_FIELD(TriggerRepeat, m_totalRepeats, FieldType::Int32));
        AddSaveField(DATA_FIELD(TriggerRepeat, m_repeatsRemaining, FieldType::Int32));
        AddSaveField(DATA_FIELD(TriggerRepeat, m_nextFireTime, FieldType::Float));
    }

    void Think(float deltaTime) override
    {
        if (IsEnabled() && Time::TotalTime() >= m_nextFireTime)
        {
            FireNow();
        }
    }

    void Touch(Entity* other) override
    {
        if (!IsEnabled() && other && other->IsPlayer())
        {
            SetEnabled(true);
            m_repeatsRemaining = m_totalRepeats;
            FireNow();
        }
    }

    void AcceptInput(const std::string& input, const std::string& param) override
    {
        Entity::AcceptInput(input, param);

        if (input == "Trigger" || input == "Start")
        {
            SetEnabled(true);
            m_repeatsRemaining = m_totalRepeats;
            FireNow();
        }
        else if (input == "Stop")
        {
            SetEnabled(false);
        }
    }

    bool IsCollidable() const override
    {
        return false;
    }

private:
    void FireNow()
    {
        if (!IsEnabled())
        {
            return;
        }
    }

    float m_delay = 1.0f;
    int m_totalRepeats = -1;
    int m_repeatsRemaining = 0;
    float m_nextFireTime = 0.0f;
};

LINK_ENTITY_TO_CLASS("trigger_repeat", TriggerRepeat)