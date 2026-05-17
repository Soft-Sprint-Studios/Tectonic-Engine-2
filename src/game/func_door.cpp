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

class FuncDoor : public Entity
{
public:
    enum State
    {
        Closed,
        Opening,
        Open,
        Closing
    };

    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);

        m_speed = GetFloat("speed", 100.0f) * BSP::MAPSCALE;
        m_wait = GetFloat("wait", 4.0f);

        float customDist = GetFloat("movedistance", 0.0f);

        // Calculate move direction from angles
        glm::vec3 angles = GetVector("angles", { 0, 0, 0 });
        if (angles == glm::vec3(-1, 0, 0))
        {
            m_moveDir = glm::vec3(0, 1, 0);
        }
        else if (angles == glm::vec3(-2, 0, 0))
        {
            m_moveDir = glm::vec3(0, -1, 0);
        }
        else
        {
            float yaw = glm::radians(angles.y);
            m_moveDir = glm::vec3(cos(yaw), 0, -sin(yaw));
        }

        m_startOrigin = GetOrigin();

        if (customDist <= 0.001f && m_physObject)
        {
            btVector3 aabbMin, aabbMax;
            m_physObject->getCollisionShape()->getAabb(btTransform::getIdentity(), aabbMin, aabbMax);
            glm::vec3 size = { aabbMax.x() - aabbMin.x(), aabbMax.y() - aabbMin.y(), aabbMax.z() - aabbMin.z() };
            m_moveDistance = glm::abs(glm::dot(size, m_moveDir));
        }
        else
        {
            m_moveDistance = customDist * BSP::MAPSCALE;
        }

        m_targetOrigin = m_startOrigin + (m_moveDir * m_moveDistance);

        if (HasSpawnFlag(2))
        {
            SetOrigin(m_targetOrigin);
            m_state = Open;
            UpdatePhysicsTransform();
        }

        UpdatePhysicsState();
    }

    void Think(float deltaTime) override
    {
        if (m_state == Opening)
        {
            MoveTo(m_targetOrigin, deltaTime, Open);
        }
        else if (m_state == Closing)
        {
            MoveTo(m_startOrigin, deltaTime, Closed);
        }
        else if (m_state == Open && m_wait >= 0.0f)
        {
            if (Time::TotalTime() >= m_nextActionTime)
            {
                m_state = Closing;
                FireOutput("OnClose");
            }
        }
    }

    void OnPress(Entity* activator) override
    {
        Trigger();
    }

    void Touch(Entity* other) override
    {
        if (other && other->IsPlayer())
        {
            if (!HasSpawnFlag(4)) // Use Only
            {
                Trigger();
            }
        }
    }

    void AcceptInput(const std::string& inputName, const std::string& parameter) override
    {
        if (inputName == "Open")
        {
            OpenDoor();
        }
        else if (inputName == "Close")
        {
            CloseDoor();
        }
        else if (inputName == "Toggle")
        {
            Trigger();
        }
    }

    bool IsRenderable() const override
    {
        return true;
    }

    void OnSave() override
    {
        Entity::OnSave();
        AddSaveField(DATA_FIELD(FuncDoor, m_state, FieldType::Int32));
        AddSaveField(DATA_FIELD(FuncDoor, m_startOrigin, FieldType::Vec3));
        AddSaveField(DATA_FIELD(FuncDoor, m_targetOrigin, FieldType::Vec3));
        AddSaveField(DATA_FIELD(FuncDoor, m_nextActionTime, FieldType::Float));
    }

private:
    void Trigger()
    {
        if (m_state == Closed)
        {
            OpenDoor();
        }
        else if (m_state == Open && m_wait < 0.0f)
        {
            CloseDoor();
        }
    }

    void OpenDoor()
    {
        if (m_state == Closed || m_state == Closing)
        {
            m_state = Opening;
            FireOutput("OnOpen");
        }
    }

    void CloseDoor()
    {
        if (m_state == Open || m_state == Opening)
        {
            m_state = Closing;
            FireOutput("OnClose");
        }
    }

    void MoveTo(const glm::vec3& target, float dt, State arriveState)
    {
        float dist = glm::distance(GetOrigin(), target);
        float step = m_speed * dt;

        if (step >= dist)
        {
            SetOrigin(target);
            m_state = arriveState;

            if (m_state == Open)
            {
                m_nextActionTime = (float)Time::TotalTime() + m_wait;
                FireOutput("OnFullyOpen");
            }
            else
            {
                FireOutput("OnFullyClosed");
            }
        }
        else
        {
            SetOrigin(GetOrigin() + glm::normalize(target - GetOrigin()) * step);
        }

        UpdatePhysicsTransform();
    }

    void UpdatePhysicsTransform()
    {
        if (m_physObject)
        {
            btTransform trans;
            trans.setIdentity();
            glm::vec3 worldPos = GetOrigin();
            trans.setOrigin({ worldPos.x, worldPos.y, worldPos.z });
            m_physObject->setWorldTransform(trans);
        }
    }

    State m_state = Closed;
    glm::vec3 m_startOrigin;
    glm::vec3 m_targetOrigin;
    glm::vec3 m_moveDir;
    float m_moveDistance = 0.0f;
    float m_speed = 0.0f;
    float m_wait = 0.0f;
    float m_nextActionTime = 0.0f;
};

LINK_ENTITY_TO_CLASS("func_door", FuncDoor)