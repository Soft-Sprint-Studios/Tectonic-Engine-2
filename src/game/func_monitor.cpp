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
#include "monitors.h"

class FuncMonitor : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);

        MonitorDef def;
        def.cameraName = GetValue("camera");
        def.position = m_origin;
        def.angles = GetVector("angles", { 0, 0, 0 });
        def.scale = GetVector("scale", { 1, 1, 1 });
        def.resolution = GetInt("resolution", 512);
        def.grayscale = (GetInt("effects") == 1);
        def.isStatic = HasSpawnFlag(2);

        m_handle = Monitors::CreateMonitor(def);
    }

    void SetEnabled(bool state) override
    {
        Entity::SetEnabled(state);
        if (m_handle)
        {
            m_handle->GetDef().active = state;
        }
    }

private:
    std::shared_ptr<Monitor> m_handle;
};

LINK_ENTITY_TO_CLASS("func_monitor", FuncMonitor)