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
#include "physics.h"
#include "timing.h"
#include <btBulletDynamicsCommon.h>

class FuncPendulum : public Entity
{
public:
    void Spawn(const BSP::EntityData& entData) override
    {
        Entity::Spawn(entData);

        m_speed = GetFloat("speed", 50.0f);
        m_distance = GetFloat("distance", 90.0f);

        if (HasSpawnFlag(4))
        {
            m_axis = glm::vec3(1, 0, 0);
        }
        else if (HasSpawnFlag(8))
        {
            m_axis = glm::vec3(0, 1, 0);
        }
        else
        {
            m_axis = glm::vec3(0, 0, 1);
        }

        m_baseAngles = GetAngles();
    }

    void OnSave() override
    {
        Entity::OnSave();
        AddSaveField(DATA_FIELD(FuncPendulum, m_baseAngles, FieldType::Vec3));
    }

    void Think(float deltaTime) override
    {
        float time = (float)Time::TotalTime();
        float currentSwing = m_distance * sin(time * (m_speed * 0.1f));

        SetAngles(m_baseAngles + (m_axis * currentSwing));

        UpdatePhysicsTransform();
    }

    bool IsRenderable() const override
    {
        return true;
    }

private:
    void UpdatePhysicsTransform()
    {
        if (m_physObject)
        {
            btTransform trans = m_physObject->getWorldTransform();

            btQuaternion rot;
            glm::vec3 ang = GetAngles();
            rot.setEuler(glm::radians(ang.y), glm::radians(ang.x), glm::radians(ang.z));

            trans.setRotation(rot);
            m_physObject->setWorldTransform(trans);
        }
    }

    float m_speed = 0.0f;
    float m_distance = 0.0f;
    glm::vec3 m_axis;
    glm::vec3 m_baseAngles;
};

LINK_ENTITY_TO_CLASS("func_pendulum", FuncPendulum)