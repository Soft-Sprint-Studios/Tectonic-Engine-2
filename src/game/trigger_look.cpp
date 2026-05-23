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

class TriggerLook : public Entity
{
public:
    void Spawn(const BSP::EntityData& entData) override
    {
        Entity::Spawn(entData);

        m_targetNameLook = GetValue("target");
        m_threshold = GetFloat("looktime", 0.9f);
        m_lookTimeRequired = GetFloat("lookduration", 0.5f);
        m_lookDist = GetFloat("lookdist", 500.0f) * BSP::MAPSCALE;
        m_curLookTime = 0.0f;
    }

    void OnSave() override
    {
        Entity::OnSave();
        AddSaveField(DATA_FIELD(TriggerLook, m_curLookTime, FieldType::Float));
        AddSaveField(DATA_FIELD(TriggerLook, m_targetNameLook, FieldType::String));
    }

    void Stay(Entity* other) override
    {
        if (other && other->IsPlayer())
        {
            auto targetEnt = EntityManager::FindEntityByName(m_targetNameLook);
            if (!targetEnt)
            {
                return;
            }

            Player* p = static_cast<Player*>(other);
            glm::vec3 targetOrigin = targetEnt->GetOrigin();
            glm::vec3 playerOrigin = p->GetOrigin();

            // Distance check
            float dist = glm::distance(targetOrigin, playerOrigin);
            if (dist > m_lookDist)
            {
                m_curLookTime = 0.0f;
                return;
            }

            // Gaze check
            glm::vec3 forward = p->GetViewForward();
            glm::vec3 toTarget = glm::normalize(targetOrigin - playerOrigin);
            float dot = glm::dot(forward, toTarget);

            if (dot >= m_threshold)
            {
                m_curLookTime += Time::DeltaTime();
                if (m_curLookTime >= m_lookTimeRequired)
                {
                    FireOutput("OnLookingAt");

                    if (HasSpawnFlag(2))
                    {
                        SetEnabled(false);
                    }
                    m_curLookTime = 0.0f;
                }
            }
            else
            {
                m_curLookTime = 0.0f;
            }
        }
    }

    bool IsCollidable() const override
    {
        return false;
    }

private:
    std::string m_targetNameLook;
    float m_threshold;
    float m_lookTimeRequired;
    float m_lookDist;
    float m_curLookTime;
};

LINK_ENTITY_TO_CLASS("trigger_look", TriggerLook)