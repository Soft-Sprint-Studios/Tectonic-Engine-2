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
#include "camera_shake.h"
#include "timing.h"
#include "entities.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace CameraShake
{
    struct ShakeInstance
    {
        glm::vec3 center;
        float amplitude;
        float frequency;
        float duration;
        float radius;
        float elapsed;
    };

    static std::vector<ShakeInstance> s_shakes;
    static glm::vec3 s_posOffset{ 0.0f };
    static glm::vec3 s_angOffset{ 0.0f };

    void AddShake(const glm::vec3& center, float amplitude, float frequency, float duration, float radius)
    {
        s_shakes.push_back({ center, amplitude, frequency, duration, radius, 0.0f });
    }

    void Update(float dt)
    {
        s_posOffset = glm::vec3(0.0f);
        s_angOffset = glm::vec3(0.0f);
        float time = Time::TotalTime();

        auto player = EntityManager::FindEntityByClass("info_player_start");
        glm::vec3 playerPos = player ? player->GetOrigin() : glm::vec3(0.0f);

        for (auto it = s_shakes.begin(); it != s_shakes.end();)
        {
            it->elapsed += dt;
            if (it->elapsed >= it->duration)
            {
                it = s_shakes.erase(it);
                continue;
            }

            float percentLife = 1.0f - (it->elapsed / it->duration);
            float distWeight = 1.0f;

            if (it->radius > 0.001f)
            {
                float dist = glm::distance(playerPos, it->center);
                distWeight = std::clamp(1.0f - (dist / it->radius), 0.0f, 1.0f);
            }

            float intensity = it->amplitude * percentLife * distWeight;
            
            s_posOffset.x += sin(time * it->frequency * 60.0f) * (intensity * 0.5f);
            s_posOffset.y += cos(time * it->frequency * 70.0f) * (intensity * 0.5f);
            s_posOffset.z += sin(time * it->frequency * 50.0f) * (intensity * 0.5f);

            s_angOffset.x += sin(time * it->frequency * 100.0f) * (intensity * 20.0f);
            s_angOffset.y += cos(time * it->frequency * 110.0f) * (intensity * 15.0f);

            ++it;
        }
    }

    glm::vec3 GetPositionOffset()
    {
        return s_posOffset;
    }

    glm::vec3 GetAngleOffset()
    {
        return s_angOffset;
    }
}