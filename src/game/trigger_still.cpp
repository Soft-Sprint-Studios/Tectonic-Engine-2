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

class TriggerStill : public Entity
{
public:
    void Spawn(const BSP::EntityData& entData) override
    {
        Entity::Spawn(entData);
        m_waitTime = GetFloat("waittime", 3.0f);
        m_timer = 0.0f;
        m_fired = false;
    }

    void Think(float deltaTime) override
    {
        if (!m_playerInside || m_fired)
        {
            return;
        }

        auto ent = EntityManager::FindEntityByClass("info_player_start");
        if (!ent) 
            return;

        auto player = std::dynamic_pointer_cast<Player>(ent);

        float distMoved = glm::distance(player->GetOrigin(), m_lastPlayerPos);

        if (distMoved < 0.01f)
        {
            m_timer += deltaTime;
            if (m_timer >= m_waitTime)
            {
                FireOutput("OnStill");
                m_fired = true;
            }
        }
        else
        {
            if (m_timer > 0.0f)
            {
                FireOutput("OnMoving");
            }
            m_timer = 0.0f;
        }

        m_lastPlayerPos = player->GetOrigin();
    }

    void Touch(Entity* other) override
    {
        if (other && other->IsPlayer())
        {
            m_playerInside = true;
            m_lastPlayerPos = other->GetOrigin();
        }
    }

    void EndTouch(Entity* other) override
    {
        if (other && other->IsPlayer())
        {
            m_playerInside = false;
            m_timer = 0.0f;
            m_fired = false;
        }
    }

    void OnSave() override
    {
        Entity::OnSave();
        AddSaveField(DATA_FIELD(TriggerStill, m_timer, FieldType::Float));
        AddSaveField(DATA_FIELD(TriggerStill, m_fired, FieldType::Bool));
    }

    bool IsCollidable() const override
    { 
        return false; 
    }

private:
    float m_waitTime = 3.0f;
    float m_timer = 0.0f;
    bool m_playerInside = false;
    bool m_fired = false;
    glm::vec3 m_lastPlayerPos{ 0.0f };
};

LINK_ENTITY_TO_CLASS("trigger_still", TriggerStill)