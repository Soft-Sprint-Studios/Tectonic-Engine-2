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
#include <random>

class TriggerChance : public Entity
{
public:
    void Spawn(const BSP::EntityData& entData) override
    {
        Entity::Spawn(entData);
        m_chance = GetFloat("chance", 50.0f);
        m_wait = GetFloat("wait", 1.0f);
        m_nextFireTime = 0.0f;
    }

    void OnSave() override
    {
        Entity::OnSave();
        AddSaveField(DATA_FIELD(TriggerChance, m_nextFireTime, FieldType::Float));
    }

    void Touch(Entity* other) override
    {
        if (other && other->IsPlayer())
        {
            if (Time::TotalTime() >= m_nextFireTime)
            {
                CalculateChance();

                if (m_wait < 0.0f)
                {
                    SetEnabled(true);
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
        if (input == "Trigger")
        {
            if (!IsEnabled())
            {
                CalculateChance();
            }
        }
    }

    bool IsCollidable() const override
    {
        return false;
    }

private:
    void CalculateChance()
    {
        static std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<float> dist(0.0f, 100.0f);

        if (dist(gen) <= m_chance)
        {
            FireOutput("OnPass");
        }
        else
        {
            FireOutput("OnFail");
        }
    }

    float m_chance = 50.0f;
    float m_wait = 1.0f;
    float m_nextFireTime = 0.0f;
};

LINK_ENTITY_TO_CLASS("trigger_chance", TriggerChance)