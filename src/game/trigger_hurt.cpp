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
#include "player.h"
#include "timing.h"

class TriggerHurt : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);
        m_damage = GetFloat("damage", 10.0f);
        m_delay = GetFloat("damagewait", 0.5f);
    }

    void Stay(Entity* other) override
    {
        if (IsEnabled() && other && other->IsPlayer())
        {
            if (Time::TotalTime() >= m_nextDamageTime)
            {
                static_cast<Player*>(other)->TakeDamage(m_damage);
                m_nextDamageTime = (float)Time::TotalTime() + m_delay;
                FireOutput("OnHurtPlayer");
            }
        }
    }

    bool IsCollidable() const override 
    { 
        return false;
    }

private:
    float m_damage = 0.0f;
    float m_delay = 0.0f;
    float m_nextDamageTime = 0.0f;
};

LINK_ENTITY_TO_CLASS("trigger_hurt", TriggerHurt)