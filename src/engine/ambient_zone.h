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
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

namespace AmbientZone
{
    struct SpotSoundRule
    {
        std::string soundFile;
        float minVolume = 1.0f;
        float maxVolume = 1.0f;
        float minPitch = 1.0f;
        float maxPitch = 1.0f;
        float minInterval = 5.0f;
        float maxInterval = 15.0f;
        float nextPlayTime = 0.0f;
    };

    struct Def
    {
        std::string name;
        std::string loopFile;
        float loopVolume = 1.0f;
        std::vector<SpotSoundRule> spots;
    };

    void Init();
    void Shutdown();
    void Activate(const std::string& name);
    void Update(float dt, const glm::vec3& playerPos);
    void Clear();
}