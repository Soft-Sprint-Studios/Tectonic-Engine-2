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
#include "postprocess.h"
#include <algorithm>

namespace PostProcess
{
    static Settings s_currentSettings;

    void SetVignette(float strength)
    {
        s_currentSettings.vignetteStrength = std::max(0.0f, strength);
    }

    void SetChroma(float strength)
    {
        s_currentSettings.chromaStrength = std::max(0.0f, strength);
    }

    void SetGrain(float strength)
    {
        s_currentSettings.grainStrength = std::max(0.0f, strength);
    }

    void SetBW(float strength)
    {
        s_currentSettings.bwStrength = std::clamp(strength, 0.0f, 1.0f);
    }

    void SetFog(bool enabled, const glm::vec3& color, float start, float end, bool affectsSky)
    {
        s_currentSettings.fogEnabled = enabled;
        s_currentSettings.fogColor = color;
        s_currentSettings.fogStart = start;
        s_currentSettings.fogEnd = end;
        s_currentSettings.fogAffectsSky = affectsSky;
    }

    const Settings& GetCurrentSettings()
    {
        return s_currentSettings;
    }
}