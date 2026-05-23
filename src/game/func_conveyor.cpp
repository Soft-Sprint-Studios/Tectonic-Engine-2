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
#include <glm/gtc/matrix_transform.hpp>
#include <set>

class FuncConveyor : public Entity
{
public:
    void Spawn(const BSP::EntityData& entData)
    {
        Entity::Spawn(entData);
        m_speed = GetFloat("speed", 100.0f) * BSP::MAPSCALE;

        glm::vec3 angles = GetVector("movedir", { 0, 0, 0 });

        float p = glm::radians(angles.x);
        float y = glm::radians(angles.y);
        float hX = cos(p) * cos(y);
        float hY = cos(p) * sin(y);
        float hZ = -sin(p);

        m_direction = glm::normalize(glm::vec3(hX, hZ, -hY));
    }

    void Touch(Entity* other)
    {
        if (other)
        {
            m_targets.insert(other);
        }
    }

    void EndTouch(Entity* other)
    {
        if (other)
        {
            m_targets.erase(other);
        }
    }

    void Think(float dt)
    {
        for (auto* ent : m_targets)
        {
            if (ent->IsPlayer())
            {
                static_cast<Player*>(ent)->ApplyPushVelocity(m_direction * m_speed);
            }
            else
            {
                ent->SetOrigin(ent->GetOrigin() + (m_direction * m_speed * dt));
            }
        }
    }

    bool IsCollidable() const override
    {
        return false;
    }

private:
    float m_speed;
    glm::vec3 m_direction;
    std::set<Entity*> m_targets;
};

LINK_ENTITY_TO_CLASS("func_conveyor", FuncConveyor)