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

class ParticleEntity : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);
        m_effect = GetValue("effect_name");
        m_sys = Particles::CreateSystem(m_effect, m_origin);
        if (m_sys) 
        {
            glm::vec3 angles = GetVector("angles");
            m_sys->SetAngles(angles);
            m_isActive = !HasSpawnFlag(1);
            m_sys->SetActive(m_isActive);
        }
    }

    void OnSave() override
    {
        Entity::OnSave();
        AddSaveField(DATA_FIELD(ParticleEntity, m_effect, FieldType::String));
        AddSaveField(DATA_FIELD(ParticleEntity, m_isActive, FieldType::Bool));
    }

    void AcceptInput(const std::string& input, const std::string& param) override
    {
        if (!m_sys) 
            return;
        if (input == "Start") 
            m_sys->SetActive(true);
        if (input == "Stop") 
            m_sys->SetActive(false);
    }

private:
    std::string m_effect;
    std::shared_ptr<ParticleSystem> m_sys;
    bool m_isActive = true;
};

LINK_ENTITY_TO_CLASS("particle_system", ParticleEntity)