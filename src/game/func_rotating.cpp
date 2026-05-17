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
#include <btBulletDynamicsCommon.h>

class FuncRotating : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);

        m_maxSpeed = GetFloat("speed", 100.0f);
        m_friction = GetFloat("fan_friction", 1.0f);

        if (HasSpawnFlag(4))
        {
            m_axis = glm::vec3(1, 0, 0);
        }
        else if (HasSpawnFlag(8))
        {
            m_axis = glm::vec3(0, 0, 1);
        }
        else
        {
            m_axis = glm::vec3(0, 1, 0);
        }

        if (HasSpawnFlag(2))
        {
            m_maxSpeed *= -1.0f;
        }

        if (IsEnabled())
        {
            m_targetSpeed = m_maxSpeed;
        }

        m_currentSpeed = m_targetSpeed;
    }

    void OnSave() override
    {
        Entity::OnSave();
        AddSaveField(DATA_FIELD(FuncRotating, m_currentSpeed, FieldType::Float));
        AddSaveField(DATA_FIELD(FuncRotating, m_targetSpeed, FieldType::Float));
    }

    void Think(float deltaTime) override
    {
        // Acceleration/Deceleration
        if (m_currentSpeed != m_targetSpeed)
        {
            float accel = m_friction * 100.0f * deltaTime;

            if (m_currentSpeed < m_targetSpeed)
            {
                m_currentSpeed = std::min(m_currentSpeed + accel, m_targetSpeed);
            }
            else
            {
                m_currentSpeed = std::max(m_currentSpeed - accel, m_targetSpeed);
            }
        }

        if (std::abs(m_currentSpeed) > 0.01f)
        {
            glm::vec3 ang = GetAngles();
            ang += m_axis * m_currentSpeed * deltaTime;

            // Wrap angles
            ang = glm::mod(ang + 360.0f, 360.0f);
            SetAngles(ang);

            UpdatePhysicsTransform();
        }
    }

    void AcceptInput(const std::string& inputName, const std::string& parameter) override
    {
        if (inputName == "Start")
        {
            m_targetSpeed = m_maxSpeed;
            FireOutput("OnStarted");
        }
        else if (inputName == "Stop")
        {
            m_targetSpeed = 0.0f;
            FireOutput("OnStopped");
        }
        else if (inputName == "Toggle")
        {
            if (std::abs(m_targetSpeed) > 0.01f)
            {
                AcceptInput("Stop", "");
            }
            else
            {
                AcceptInput("Start", "");
            }
        }
        else if (inputName == "Reverse")
        {
            m_maxSpeed *= -1.0f;

            if (std::abs(m_targetSpeed) > 0.01f)
            {
                m_targetSpeed = m_maxSpeed;
            }
        }
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

    float m_currentSpeed = 0.0f;
    float m_targetSpeed = 0.0f;
    float m_maxSpeed = 0.0f;
    float m_friction = 1.0f;
    glm::vec3 m_axis;
};

LINK_ENTITY_TO_CLASS("func_rotating", FuncRotating)