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

class FuncDoorRotating : public Entity
{
public:
    enum State
    {
        Closed,
        Opening,
        Open,
        Closing
    };

    void Spawn(const BSP::EntityData& entData) override
    {
        Entity::Spawn(entData);

        m_speed = GetFloat("speed", 100.0f);
        m_wait = GetFloat("wait", 4.0f);
        float dist = GetFloat("distance", 90.0f);

        if (HasSpawnFlag(4))
            m_axis = glm::vec3(1, 0, 0);
        else if (HasSpawnFlag(8))
            m_axis = glm::vec3(0, 0, 1);
        else 
            m_axis = glm::vec3(0, 1, 0);

        if (HasSpawnFlag(2)) 
            dist *= -1.0f;

        m_startAngles = GetAngles();
        m_targetAngles = m_startAngles + (m_axis * dist);

        m_state = Closed;
        UpdatePhysicsState();
    }

    void OnSave() override
    {
        Entity::OnSave();
        AddSaveField(DATA_FIELD(FuncDoorRotating, m_state, FieldType::Int32));
        AddSaveField(DATA_FIELD(FuncDoorRotating, m_nextActionTime, FieldType::Float));
    }

    void Think(float deltaTime) override
    {
        if (m_state == Opening)
        {
            RotateTo(m_targetAngles, deltaTime, Open);
        }
        else if (m_state == Closing)
        {
            RotateTo(m_startAngles, deltaTime, Closed);
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
        if (activator && activator->IsPlayer())
        {
            Trigger();
        }
    }

    void AcceptInput(const std::string& inputName, const std::string& parameter) override
    {
        if (inputName == "Open")
            m_state = Opening;
        else if (inputName == "Close") 
            m_state = Closing;
        else if (inputName == "Toggle") 
            Trigger();
    }

    bool IsRenderable() const override 
    { 
        return true;
    }

private:
    void Trigger()
    {
        if (m_state == Closed) 
            m_state = Opening;
        else if (m_state == Open && m_wait < 0.0f) 
            m_state = Closing;
    }

    void RotateTo(const glm::vec3& target, float dt, State arriveState)
    {
        float current = glm::length(GetAngles() - m_startAngles);
        float goal = glm::length(target - m_startAngles);
        float step = m_speed * dt;

        glm::vec3 nextAngles;
        if (glm::distance(GetAngles(), target) <= step)
        {
            nextAngles = target;
            m_state = arriveState;
            if (m_state == Open)
            {
                m_nextActionTime = (float)Time::TotalTime() + m_wait;
                FireOutput("OnFullyOpen");
            }
            else 
                FireOutput("OnFullyClosed");
        }
        else
        {
            nextAngles = GetAngles() + glm::normalize(target - GetAngles()) * step;
        }

        SetAngles(nextAngles);
        UpdatePhysicsTransform();
    }

    State m_state;
    glm::vec3 m_startAngles, m_targetAngles, m_axis;
    float m_speed, m_wait, m_nextActionTime;
};

LINK_ENTITY_TO_CLASS("func_door_rotating", FuncDoorRotating)