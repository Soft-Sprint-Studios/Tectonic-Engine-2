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
#include "r_shader.h"
#include "r_texture.h"
#include "camera.h"
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glad/glad.h>

struct WaterSurface
{
    uint32_t start;
    uint32_t count;
    float height;
};

class R_Waters
{
public:
    void Init(int width, int height);
    void AddSurface(const WaterSurface& surface);
    void ClearSurfaces();
    void RenderReflection(class Renderer* renderer, const Camera& mainCam);
    void Draw(const Camera& camera, GLuint vao);
    void Shutdown();

private:
    R_Shader m_shader;
    std::vector<WaterSurface> m_surfaces;
    std::vector<GLint> m_starts;
    std::vector<GLsizei> m_counts;
    GLuint m_reflectFBO = 0;
    GLuint m_reflectTex = 0;
    GLuint m_reflectRBO = 0;
    glm::mat4 m_reflectView;
    glm::mat4 m_reflectProj;
    std::shared_ptr<R_Texture> m_normalMap;
    std::shared_ptr<R_Texture> m_dudvMap;
    int m_width, m_height;
};