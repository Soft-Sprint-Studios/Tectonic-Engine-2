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
#include "entities.h"
#include "dynamic_sky.h"

class Dynamic_Sky : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);
        DynamicSky::SetEnabled(true);

        glm::vec3 angles = GetVector("angles", { 0, 0, 0 });
        float p = glm::radians(angles.x);
        float y = glm::radians(angles.y);

        float hx = cos(p) * cos(y);
        float hy = cos(p) * sin(y);
        float hz = -sin(p);

        DynamicSky::SetSunDirection(glm::normalize(glm::vec3(-hx, hz, hy)));

        glm::vec4 lightData = GetVector4("_light", glm::vec4(255, 255, 255, 255));
        glm::vec3 color = glm::vec3(lightData.x, lightData.y, lightData.z) / 255.0f;
        float intensity = lightData.w / 255.0f;

        DynamicSky::SetSunColor(color * intensity);
        DynamicSky::SetVolumetrics(GetFloat("volumetric_intensity", 0.0f), GetInt("volumetric_steps", 8));
        DynamicSky::SetCSM(GetInt("hascsm", 1) != 0);
    }   

    void AcceptInput(const std::string& input, const std::string& param) override
    {
        if (input == "Enable")
            DynamicSky::SetEnabled(true);
        if (input == "Disable")
            DynamicSky::SetEnabled(false);
    }
};

LINK_ENTITY_TO_CLASS("dynamic_sky", Dynamic_Sky)