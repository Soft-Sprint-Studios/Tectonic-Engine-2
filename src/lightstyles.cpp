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
#include "lightstyles.h"
#include <cmath>
#include <algorithm>

namespace LightStyles
{
    static std::map<int, LightStyle> s_styles;
    static std::vector<float> s_values;

    void Init()
    {
        s_values.assign(256, 1.0f);

        // Copied from Pathos engine lightstyles
        SetStyle(0, "m", 10.0f, false); // Normal
        SetStyle(1, "mmnmmommommnonmmonqnmmo", 10.0f, true); // Flicker A
        SetStyle(2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba", 10.0f, true); // Slow Strong Pulse
        SetStyle(3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg", 10.0f, true); // Candle A
        SetStyle(4, "mamamamamama", 10.0f, true); // Fast Strobe
        SetStyle(5, "jklmnopqrstuvwxyzyxwvutsrqponmlkj", 10.0f, true); // Gentle Pulse
        SetStyle(6, "nmonqnmomnmomomno", 10.0f, true); // Flicker B
        SetStyle(7, "mmmaaaabcdefgmmmmaaaammmaamm", 10.0f, true); // Candle B
        SetStyle(8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa", 10.0f, true); // Candle C
        SetStyle(9, "aaaaaaaazzzzzzzz", 10.0f, true); // Slow Strobe
        SetStyle(10, "mmammmammmaamamaaammmma", 10.0f, true); // Fluorescent Flicker
        SetStyle(11, "abcdefghijklmnopqrrqponmlkjihgfedcba", 10.0f, true); // Slow Pulse No Black
    }

    void SetStyle(int index, const std::string& pattern, float frameRate, bool interpolate)
    {
        LightStyle s;
        s.pattern = pattern;
        s.frameRate = frameRate;
        s.interpolate = interpolate;
        s_styles[index] = s;
    }

    // Function based on pathos engine implementation
    void Update(float currentTime)
    {
        for (auto const& [index, style] : s_styles)
        {
            if (style.pattern.empty())
            {
                s_values[index] = 1.0f;
                continue;
            }

            auto GetVal = [&](int i) -> float
            {
                int len = (int)style.pattern.length();
                char c = style.pattern[i % len];
                float letterIdx = (float)(std::clamp(c, 'a', 'z') - 'a');
                return (letterIdx * 22.0f) / 256.0f;
            };

            float frame = currentTime * style.frameRate;

            if (style.interpolate)
            {
                float interp = frame - std::floor(frame);
                int i1 = (int)std::floor(frame);
                s_values[index] = GetVal(i1) * (1.0f - interp) + GetVal(i1 + 1) * interp;
            }
            else
            {
                int i1 = (int)std::floor(frame);
                s_values[index] = GetVal(i1);
            }
        }
    }

    float GetModifier(int index)
    {
        if (index < 0 || index >= 256)
        {
            return 1.0f;
        }
        return s_values[index];
    }
}