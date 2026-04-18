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
#include "dynamic_light.h"
#include "shader.h"
#include "camera.h"
#include "r_cascade.h"
#include <glad/glad.h>
#include <memory>

class R_BSP;
class R_Models;

class R_Lights
{
public:
    R_Lights();
    ~R_Lights();

    bool Init();
    void RenderShadowMaps(Camera& camera, R_BSP* bsp, R_Models* models);
    void Bind(const Shader& shader);
    void Shutdown();

private:
    void SetupShadowMap(std::shared_ptr<DynamicLight> light);

    Shader m_shadowSpotShader;
    Shader m_shadowPointShader;

    GLuint m_SpotShadow;
    GLuint m_PointShadow;

    std::unique_ptr<R_Cascade> m_cascade;
    static glm::vec3 s_sunDir;
    static glm::vec3 s_sunColor;
};