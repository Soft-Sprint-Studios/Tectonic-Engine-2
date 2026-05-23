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

class FuncPlat : public Entity
{
public:
    enum State
    {
        Bottom,
        Up,
        Top,
        Down
    };

    void Spawn(const BSP::EntityData& entData) override
    {
        Entity::Spawn(entData);

        m_speed = GetFloat("speed", 50.0f) * BSP::MAPSCALE;
        m_wait = GetFloat("wait", 3.0f);
        
        float height = GetFloat("height", 0.0f);

        m_startOrigin = GetOrigin();

        m_targetOrigin = m_startOrigin + glm::vec3(0.0f, height * BSP::MAPSCALE, 0.0f);
        m_state = Bottom;

        UpdatePhysicsState();
    }

    void OnSave() override
    {
        Entity::OnSave();
        AddSaveField(DATA_FIELD(FuncPlat, m_state, FieldType::Int32));
        AddSaveField(DATA_FIELD(FuncPlat, m_startOrigin, FieldType::Vec3));
        AddSaveField(DATA_FIELD(FuncPlat, m_targetOrigin, FieldType::Vec3));
        AddSaveField(DATA_FIELD(FuncPlat, m_nextActionTime, FieldType::Float));
    }

    void Think(float deltaTime) override
    {
        if (m_state == Up)
        {
            MoveTo(m_targetOrigin, deltaTime, Top);
        }
        else if (m_state == Down)
        {
            MoveTo(m_startOrigin, deltaTime, Bottom);
        }
        else if (m_state == Top && m_wait >= 0.0f)
        {
            if (Time::TotalTime() >= m_nextActionTime)
            {
                m_state = Down;
            }
        }
    }

    void Touch(Entity* other) override
    {
        if (HasSpawnFlag(1))
        {
            return;
        }

        if (m_targetName.empty() && m_state == Bottom && other && other->IsPlayer())
        {
            Trigger();
        }
    }

    void AcceptInput(const std::string& inputName, const std::string& parameter) override
    {
        if (inputName == "Toggle")
        {
            Trigger();
        }
        else if (inputName == "GoUp")
        {
            if (m_state == Bottom || m_state == Down)
            {
                m_state = Up;
            }
        }
        else if (inputName == "GoDown")
        {
            if (m_state == Top || m_state == Up)
            {
                m_state = Down;
            }
        }
    }

    bool IsRenderable() const override
    {
        return true;
    }

private:
    void Trigger()
    {
        if (m_state == Bottom)
        {
            m_state = Up;
        }
        else if (m_state == Top && m_wait < 0.0f)
        {
            m_state = Down;
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

            if (m_state == Top)
            {
                m_nextActionTime = (float)Time::TotalTime() + m_wait;
            }
        }
        else
        {
            SetOrigin(GetOrigin() + glm::normalize(target - GetOrigin()) * step);
        }

        Entity::UpdatePhysicsTransform();
    }

    State m_state = Bottom;
    glm::vec3 m_startOrigin;
    glm::vec3 m_targetOrigin;
    float m_speed = 0.0f;
    float m_wait = 0.0f;
    float m_nextActionTime = 0.0f;
};

LINK_ENTITY_TO_CLASS("func_plat", FuncPlat)