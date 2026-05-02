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
#include <glm/glm.hpp>
#include <memory>
#include <vector>

enum class LightType
{
    Point,
    Spot
};

struct DynamicLightDef
{
    LightType type;
    glm::vec3 color;
    float radius;
    float innerAngle;
    float outerAngle;

    float volumetricIntensity = 0.0f;
    int volumetricSteps = 32;

    bool castsShadows = false;
    bool isStaticShadow = false;
    bool shadowRendered = false;
    int shadowRes = 512;

    uint32_t shadowFBO = 0;
    uint32_t shadowTex = 0;
    uint32_t shadowDepthTex = 0;

    glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);
};

class DynamicLight
{
public:
    DynamicLight(const DynamicLightDef& def, const glm::vec3& position);

    void SetActive(bool active);
    void SetPosition(const glm::vec3& position);
    void SetDirection(const glm::vec3& direction);

    bool IsActive() const;
    DynamicLightDef& GetDef();
    const glm::vec3& GetPosition() const;
    const glm::vec3& GetDirection() const;

private:
    DynamicLightDef m_def;
    glm::vec3 m_position;
    glm::vec3 m_direction;
    bool m_active;
};

namespace DynamicLights
{
    void Init();
    void Update();
    void Shutdown();
    void Clear();

    std::shared_ptr<DynamicLight> CreatePointLight(const glm::vec3& position, const glm::vec3& color, float radius);
    std::shared_ptr<DynamicLight> CreateSpotLight(const glm::vec3& position, const glm::vec3& direction, const glm::vec3& color, float radius, float innerAngle, float outerAngle);

    const std::vector<std::shared_ptr<DynamicLight>>& GetActiveLights();
}