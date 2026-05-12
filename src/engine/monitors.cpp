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
#include "monitors.h"

Monitor::Monitor(const MonitorDef& def) : m_def(def)
{
}

namespace Monitors
{
    static std::vector<std::shared_ptr<Monitor>> s_monitors;

    void Init()
    {
    }

    void Update()
    {
        for (auto it = s_monitors.begin(); it != s_monitors.end();)
        {
            if (it->use_count() <= 1)
            {
                it = s_monitors.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void Clear()
    {
        s_monitors.clear();
    }

    std::shared_ptr<Monitor> CreateMonitor(const MonitorDef& def)
    {
        auto m = std::make_shared<Monitor>(def);
        s_monitors.push_back(m);
        return m;
    }

    const std::vector<std::shared_ptr<Monitor>>& GetActiveMonitors()
    {
        return s_monitors;
    }
}