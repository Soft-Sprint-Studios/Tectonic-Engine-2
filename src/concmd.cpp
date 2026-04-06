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
#include "concmd.h"

std::unordered_map<std::string, ConCmd*>& ConCmd::GetRegistry()
{
    static std::unordered_map<std::string, ConCmd*> s_cmdRegistry;
    return s_cmdRegistry;
}

ConCmd::ConCmd(const std::string& name, ConCmdCallback callback, const std::string& description)
    : m_name(name), m_callback(callback), m_description(description)
{
    GetRegistry()[name] = this;
}

ConCmd* ConCmd::Find(const std::string& name)
{
    auto& registry = GetRegistry();
    auto it = registry.find(name);
    return (it != registry.end()) ? it->second : nullptr;
}

void ConCmd::Execute(const std::vector<std::string>& args)
{
    if (m_callback)
    {
        m_callback(args);
    }
}