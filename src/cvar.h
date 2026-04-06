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
#include <string>
#include <unordered_map>

enum CVarFlags
{
    CVAR_NONE = 0,
    CVAR_SAVE = (1 << 0)
};

class CVar
{
public:
    CVar(const std::string& name, const std::string& defaultValue, int flags = CVAR_NONE);

    static void Set(const std::string& name, const std::string& value);
    static CVar* Find(const std::string& name);
    static std::unordered_map<std::string, CVar*>& GetRegistry();

    static void Init();
    static void Save();

    std::string GetString() const
    {
        return m_value;
    }

    int GetInt() const
    {
        return std::stoi(m_value);
    }

    float GetFloat() const
    {
        return std::stof(m_value);
    }

private:
    std::string m_name;
    std::string m_value;
    int m_flags;
};