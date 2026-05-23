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
#include <glm/ext/matrix_transform.hpp>

class FuncMoveLinear : public Entity
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

        m_speed = GetFloat("speed", 100.0f) * BSP::MAPSCALE;
        float moveDistance = GetFloat("movedistance", 128.0f) * BSP::MAPSCALE;
        float startPosition = GetFloat("startposition", 0.0f);
        
        glm::vec3 angles = GetVector("movedir", { 0, 0, 0 });
        glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(angles.y), glm::vec3(0, 1, 0));
        m_moveDir = glm::vec3(rot * glm::vec4(1, 0, 0, 0));

        m_startOrigin = GetOrigin();
        m_endOrigin = m_startOrigin + (m_moveDir * moveDistance);

        SetOrigin(glm::mix(m_startOrigin, m_endOrigin, startPosition));
        m_state = (startPosition > 0.5f) ? Open : Closed;
        
        UpdatePhysicsTransform();
        UpdatePhysicsState();
    }

    void Think(float deltaTime) override
    {
        if (m_state == Opening)
        {
            MoveTo(m_endOrigin, deltaTime, Open);
        }
        else if (m_state == Closing)
        {
            MoveTo(m_startOrigin, deltaTime, Closed);
        }
    }

    void AcceptInput(const std::string& inputName, const std::string& parameter) override
    {
        if (inputName == "Open")
        {
            if (m_state == Closed || m_state == Closing)
                m_state = Opening;
        }
        else if (inputName == "Close")
        {
            if (m_state == Open || m_state == Opening)
                m_state = Closing;
        }
        else if (inputName == "Toggle")
        {
            if (m_state == Open || m_state == Opening)
                m_state = Closing;
            else
                m_state = Opening;
        }
    }

    void OnSave() override
    {
        Entity::OnSave();
        AddSaveField(DATA_FIELD(FuncMoveLinear, m_state, FieldType::Int32));
        AddSaveField(DATA_FIELD(FuncMoveLinear, m_startOrigin, FieldType::Vec3));
        AddSaveField(DATA_FIELD(FuncMoveLinear, m_endOrigin, FieldType::Vec3));
    }

    bool IsRenderable() const override
    {
        return true;
    }

private:
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

        Entity::UpdatePhysicsTransform();
    }

    State m_state = Closed;
    glm::vec3 m_startOrigin;
    glm::vec3 m_endOrigin;
    glm::vec3 m_moveDir;
    float m_speed = 0.0f;
};

LINK_ENTITY_TO_CLASS("func_movelinear", FuncMoveLinear)