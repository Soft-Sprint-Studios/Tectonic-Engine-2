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

class TriggerPush : public Entity
{
public:
    void Spawn(const BSP::EntityData& entData) override
    {
        Entity::Spawn(entData);
        m_speed = GetFloat("speed", 400.0f) * BSP::MAPSCALE;

        glm::vec3 angles = GetVector("movedir", { 0, 0, 0 });

        float p = glm::radians(angles.x);
        float y = glm::radians(angles.y);
        float hX = cos(p) * cos(y);
        float hY = cos(p) * sin(y);
        float hZ = -sin(p);

        glm::vec3 dir;
        dir.x = hX;
        dir.y = hZ;
        dir.z = -hY;

        m_pushDir = glm::normalize(dir);
    }

    void Stay(Entity* other) override
    {
        if (IsEnabled() && other && other->IsPlayer())
        {
            Player* p = static_cast<Player*>(other);
            p->ApplyPushVelocity(m_pushDir * m_speed);
        }
    }

    bool IsCollidable() const override
    {
        return false;
    }

private:
    float m_speed = 0.0f;
    glm::vec3 m_pushDir;
};

LINK_ENTITY_TO_CLASS("trigger_push", TriggerPush)