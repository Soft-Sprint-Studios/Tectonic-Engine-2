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
#include "dynamic_light.h"
#include "lightstyles.h"

class LightDynamicPoint : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);

        glm::vec4 colorData = GetVector4("_light", glm::vec4(255.0f, 255.0f, 255.0f, 200.0f));
        m_baseColor = glm::vec3(colorData.x, colorData.y, colorData.z) / 255.0f;
        m_baseColor *= (colorData.w / 255.0f) * 2.0f;

        float radius = GetFloat("distance", 500.0f) * BSP::MAPSCALE;

        m_light = DynamicLights::CreatePointLight(m_origin, m_baseColor, radius);
        if (m_light)
        {
            m_isActive = !HasSpawnFlag(1);
            m_light->SetActive(m_isActive);
            auto& def = const_cast<DynamicLightDef&>(m_light->GetDef());
            def.castsShadows = HasSpawnFlag(2);
            def.isStaticShadow = HasSpawnFlag(4);
            def.shadowRes = GetInt("shadow_res", 512);
            def.volumetricIntensity = GetFloat("volumetric_intensity", 0.0f);
            def.volumetricSteps = GetInt("volumetric_steps", 32);
        }
    }

    void OnSave() override
    {
        Entity::OnSave();
        AddSaveField(DATA_FIELD(LightDynamicPoint, m_baseColor, FieldType::Vec3));
        m_isActive = m_light->IsActive();
        AddSaveField(DATA_FIELD(LightDynamicPoint, m_isActive, FieldType::Bool));
    }

    void Think(float deltaTime) override
    {
        Entity::Think(deltaTime);
        if (m_light)
        {
            m_light->SetPosition(m_origin);

            int styleIndex = GetInt("style", 0);
            float mod = LightStyles::GetModifier(styleIndex);

            m_light->GetDef().color = m_baseColor * mod;
        }
    }

    void AcceptInput(const std::string& input, const std::string& param) override
    {
        if (!m_light)
        {
            return;
        }

        if (input == "Enable")
        {
            m_light->SetActive(true);
        }
        else if (input == "Disable")
        {
            m_light->SetActive(false);
        }
        else if (input == "Toggle")
        {
            m_light->SetActive(!m_light->IsActive());
        }
    }

    bool IsCollidable() const override
    {
        return false;
    }

protected:
    std::shared_ptr<DynamicLight> m_light;
    glm::vec3 m_baseColor;
    bool m_isActive = true;
};

LINK_ENTITY_TO_CLASS("light_dynamic_point", LightDynamicPoint)