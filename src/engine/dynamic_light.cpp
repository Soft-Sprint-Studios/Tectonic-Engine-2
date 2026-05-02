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
#include "dynamic_light.h"

DynamicLight::DynamicLight(const DynamicLightDef& def, const glm::vec3& position)
    : m_def(def), m_position(position), m_direction(glm::vec3(0.0f, -1.0f, 0.0f)), m_active(true)
{
}

namespace DynamicLights
{
    static std::vector<std::shared_ptr<DynamicLight>> s_lights;

    void Init()
    {
    }

    void Update()
    {
        for (auto it = s_lights.begin(); it != s_lights.end();)
        {
            if (it->use_count() <= 1)
            {
                it = s_lights.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void Shutdown()
    {
        Clear();
    }

    void Clear()
    {
        s_lights.clear();
    }

    std::shared_ptr<DynamicLight> CreatePointLight(const glm::vec3& position, const glm::vec3& color, float radius)
    {
        DynamicLightDef def;
        def.type = LightType::Point;
        def.color = color;
        def.radius = radius;
        def.innerAngle = 0.0f;
        def.outerAngle = 0.0f;

        auto light = std::make_shared<DynamicLight>(def, position);
        s_lights.push_back(light);
        return light;
    }

    std::shared_ptr<DynamicLight> CreateSpotLight(const glm::vec3& position, const glm::vec3& direction, const glm::vec3& color, float radius, float innerAngle, float outerAngle)
    {
        DynamicLightDef def;
        def.type = LightType::Spot;
        def.color = color;
        def.radius = radius;
        def.innerAngle = innerAngle;
        def.outerAngle = outerAngle;

        auto light = std::make_shared<DynamicLight>(def, position);
        light->SetDirection(direction);
        s_lights.push_back(light);
        return light;
    }

    const std::vector<std::shared_ptr<DynamicLight>>& GetActiveLights()
    {
        return s_lights;
    }
}

void DynamicLight::SetActive(bool active)
{
    m_active = active;
}

void DynamicLight::SetPosition(const glm::vec3& position)
{
    m_position = position;
}

void DynamicLight::SetDirection(const glm::vec3& direction)
{
    m_direction = direction;
}

bool DynamicLight::IsActive() const
{
    return m_active;
}

DynamicLightDef& DynamicLight::GetDef()
{
    return m_def;
}

const glm::vec3& DynamicLight::GetPosition() const
{
    return m_position;
}

const glm::vec3& DynamicLight::GetDirection() const
{
    return m_direction;
}