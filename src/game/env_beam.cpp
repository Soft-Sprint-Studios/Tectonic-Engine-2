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
#include "beams.h"

class EnvBeam : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);

        BeamDef def;
        def.endEntity = GetValue("LightningEnd");
        def.width = GetFloat("width", 2.0f) * BSP::MAPSCALE;

        glm::vec3 col = GetVector("rendercolor", { 255, 255, 255 });
        def.color = col / 255.0f;

        m_beam = Beams::CreateBeam(def);
    }

    void SetEnabled(bool state) override
    {
        Entity::SetEnabled(state);
        if (m_beam)
        {
            m_beam->SetActive(state);
        }
    }

    void Think(float deltaTime) override
    {
        Entity::Think(deltaTime);

        if (m_beam)
        {
            m_beam->GetDef().startPos = m_origin;

            auto target = EntityManager::FindEntityByName(m_beam->GetDef().endEntity);
            if (target)
            {
                m_beam->GetDef().endPos = target->GetOrigin();
            }
            else
            {
                m_beam->GetDef().endPos = m_origin;
            }
        }
    }

    bool IsCollidable() const override
    {
        return false;
    }

private:
    std::shared_ptr<Beam> m_beam;
};

LINK_ENTITY_TO_CLASS("env_beam", EnvBeam)