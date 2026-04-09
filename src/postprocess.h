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
#include <glm/vec3.hpp>

namespace PostProcess
{
    struct Settings
    {
        float vignetteStrength = 0.0f;
        float chromaStrength = 0.0f;
        float grainStrength = 0.0f;
        float bwStrength = 0.0f;
        bool fogEnabled = false;
        glm::vec3 fogColor{ 0.5f, 0.6f, 0.7f };
        float fogStart = 50.0f;
        float fogEnd = 200.0f;
        bool fogAffectsSky = true;
    };

    void SetVignette(float strength);
    void SetChroma(float strength);
    void SetGrain(float strength);
    void SetBW(float strength);
    void SetFog(bool enabled, const glm::vec3& color, float start, float end, bool affectsSky);

    const Settings& GetCurrentSettings();
}