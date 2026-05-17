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
#include "particles.h"
#include "physics.h"
#include <random>
#include <cmath>

class FuncPrecipitation : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);

        std::string type = "rain";
        if (GetInt("preciptype", 0) == 1)
        {
            type = "snow";
        }

        m_sys = Particles::CreateSystem(type, GetOrigin());

        if (m_physObject && m_sys)
        {
            btVector3 aabbMin, aabbMax;
            m_physObject->getCollisionShape()->getAabb(m_physObject->getWorldTransform(), aabbMin, aabbMax);
            m_mins = { aabbMin.x(), aabbMin.y(), aabbMin.z() };
            m_maxs = { aabbMax.x(), aabbMax.y(), aabbMax.z() };

            float height = m_maxs.y - m_mins.y;
            auto& def = m_sys->GetDef();
            float avgSpeed = (def.minSpeed + def.maxSpeed) * 0.5f;
            float g = 9.81f * def.gravity;

            if (g > 0.001f)
            {
                def.lifetime = (-avgSpeed + std::sqrt(avgSpeed * avgSpeed + 2.0f * g * height)) / g;
            }
            else
            {
                def.lifetime = height / std::max(0.01f, avgSpeed);
            }

            m_sys->SetAngles({ -90, 0, 0 });
            m_sys->SetActive(IsEnabled());
        }
    }

    void SetEnabled(bool state) override
    {
        Entity::SetEnabled(state);
        if (m_sys)
        {
            m_sys->SetActive(state);
        }
    }

    void Think(float deltaTime) override
    {
        if (m_sys && m_sys->GetDef().rate > 0)
        {
            static std::mt19937 gen(std::random_device{}());
            std::uniform_real_distribution<float> distX(m_mins.x, m_maxs.x);
            std::uniform_real_distribution<float> distZ(m_mins.z, m_maxs.z);

            float x = distX(gen);
            float z = distZ(gen);
            m_sys->SetOrigin({ x, m_maxs.y, z });
        }
    }

    bool IsRenderable() const override 
    { 
        return false; 
    }

    bool IsCollidable() const override 
    { 
        return false; 
    }

private:
    std::shared_ptr<ParticleSystem> m_sys;
    glm::vec3 m_mins, m_maxs;
};

LINK_ENTITY_TO_CLASS("func_precipitation", FuncPrecipitation)