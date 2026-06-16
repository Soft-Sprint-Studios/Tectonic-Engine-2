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
#include "ambient_zone.h"
#include "timing.h"

class TriggerAmbientZone : public Entity
{
public:
    void Spawn(const BSP::EntityData& entData) override
    {
        Entity::Spawn(entData);
        m_zoneName = GetValue("zone_name");
    }

    void Touch(Entity* other) override
    {
        if (IsEnabled() && other && other->IsPlayer())
        {
            AmbientZone::Activate(m_zoneName);
        }
    }

    void EndTouch(Entity* other) override
    {
        if (other && other->IsPlayer())
        {
            AmbientZone::Clear();
        }
    }

    void SetEnabled(bool state) override
    {
        Entity::SetEnabled(state);
        if (!state)
        {
            AmbientZone::Clear();
        }
    }

    bool IsCollidable() const override
    {
        return false;
    }

private:
    std::string m_zoneName;
};

LINK_ENTITY_TO_CLASS("trigger_ambient_zone", TriggerAmbientZone)