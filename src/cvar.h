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
    CVAR_SAVE = (1 << 0),
    CVAR_CHEAT = (1 << 1)
};

class CVar
{
public:
    CVar(const std::string& name, const std::string& defaultValue, const std::string& description, int flags = CVAR_NONE);

    std::string GetString() const
    {
        return m_value;
    }

    int GetInt() const
    {
        try
        {
            if (m_value.empty())
                return 0;

            return std::stoi(m_value);
        }
        catch (...)
        {
            return 0;
        }
    }

    float GetFloat() const
    {
        try
        {
            if (m_value.empty())
                return 0.0f;

            return std::stof(m_value);
        }
        catch (...)
        {
            return 0.0f;
        }
    }

    std::string GetDescription() const
    {
        return m_description;
    }

    bool IsCheat() const
    {
        return (m_flags & CVAR_CHEAT) != 0;
    }

    static int GetInt(const std::string& name, int defaultVal = 0);
    static float GetFloat(const std::string& name, float defaultVal = 0.0f);
    static std::string GetString(const std::string& name, const std::string& defaultVal = "");

    static void Set(const std::string& name, const std::string& value);
    static CVar* Find(const std::string& name);
    static std::unordered_map<std::string, CVar*>& GetRegistry();

    static void Init();
    static void Save();

private:
    std::string m_name;
    std::string m_value;
    std::string m_description;
    int m_flags;
};