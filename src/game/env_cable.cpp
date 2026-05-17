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
#include "cables.h"

class EnvCable : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);

        CableDef def;
        def.targetEntity = GetValue("CableEnd");
        def.width = GetFloat("width", 0.5f) * BSP::MAPSCALE;
        def.slack = GetFloat("slack", 16.0f) * BSP::MAPSCALE;
        def.segments = GetInt("segments", 16);

        m_cable = Cables::CreateCable(def);
    }

    void Think(float deltaTime) override
    {
        if (!m_cable) 
            return;
        
        m_cable->GetDef().startPos = GetOrigin();
        auto target = EntityManager::FindEntityByName(m_cable->GetDef().targetEntity);
        if (target)
        {
            m_cable->GetDef().endPos = target->GetOrigin();
        }
    }

    bool IsCollidable() const override 
    { 
        return false; 
    }

private:
    std::shared_ptr<Cable> m_cable;
};

LINK_ENTITY_TO_CLASS("env_cable", EnvCable)