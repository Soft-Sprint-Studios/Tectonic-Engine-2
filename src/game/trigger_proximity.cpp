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

class TriggerProximity : public Entity
{
public:
    void Spawn(const BSP::EntityData& entData) override
    {
        Entity::Spawn(entData);
        m_radius = GetFloat("radius", 128.0f) * BSP::MAPSCALE;
    }

    void OnSave() override
    {
        Entity::OnSave();
        AddSaveField(DATA_FIELD(TriggerProximity, m_radius, FieldType::Float));
        AddSaveField(DATA_FIELD(TriggerProximity, m_playerInside, FieldType::Bool));
    }

    void Think(float deltaTime) override
    {
        Entity::Think(deltaTime);

        auto ent = EntityManager::FindEntityByClass("info_player_start");
        if (!ent)
        {
            return;
        }

        auto player = std::dynamic_pointer_cast<Player>(ent);
        float dist = glm::distance(GetOrigin(), player->GetOrigin());

        if (dist <= m_radius)
        {
            if (!m_playerInside)
            {
                m_playerInside = true;
                FireOutput("OnNearest");
            }
        }
        else
        {
            if (m_playerInside)
            {
                m_playerInside = false;
                FireOutput("OnFarther");
            }
        }
    }

    bool IsCollidable() const override
    {
        return false;
    }

private:
    float m_radius = 0.0f;
    bool m_playerInside = false;
};

LINK_ENTITY_TO_CLASS("trigger_proximity", TriggerProximity)