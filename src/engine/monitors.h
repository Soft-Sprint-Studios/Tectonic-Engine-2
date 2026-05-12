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
#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>

struct MonitorDef
{
    std::string cameraName;
    int resolution = 512;
    bool grayscale = false;
    bool isStatic = false;
    bool active = true;
};

class Monitor
{
public:
    Monitor(const MonitorDef& def);

    MonitorDef& GetDef()
    {
        return m_def;
    }

    bool IsActive() const
    {
        return m_def.active;
    }

    void SetActive(bool state)
    {
        m_def.active = state;
    }

    bool HasRenderedOnce() const
    {
        return m_hasRendered;
    }

    void SetRenderedOnce(bool state)
    {
        m_hasRendered = state;
    }

private:
    MonitorDef m_def;
    bool m_hasRendered = false;
};

namespace Monitors
{
    void Init();
    void Update();
    void Clear();

    std::shared_ptr<Monitor> CreateMonitor(const MonitorDef& def);
    const std::vector<std::shared_ptr<Monitor>>& GetActiveMonitors();
}