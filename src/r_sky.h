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
#include "shader.h"
#include "camera.h"
#include <glad/glad.h>
#include <string>
#include <vector>

class R_Sky
{
public:
    R_Sky();
    ~R_Sky();

    bool Init(const std::string& skyName);
    void Draw(const Camera& camera);
    void Shutdown();

    // Dynamic sky data
    static glm::vec3 s_sunDir;
    static glm::vec3 s_sunColor;
    static bool s_useDynamic;
    static bool s_hasCSM;

private:
    GLuint m_vao, m_vbo;
    GLuint m_cubemapTexture;
    Shader m_shader;

    void LoadCubemap(const std::string& skyName);
};