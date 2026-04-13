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
#include "r_lights.h"
#include <glad/glad.h>

class R_Volumetrics 
{
public:
    R_Volumetrics();
    ~R_Volumetrics();

    bool Init(int width, int height);
    void Shutdown();
    void Rescale(int width, int height);
    void Render(GLuint depthTexture, const Camera& camera, R_Lights* lights, GLuint quadVAO, int screenW, int screenH);
    void Bind(const Shader& shader);

private:
    void CreateBuffers(int width, int height);
    void DeleteBuffers();

    GLuint m_fbo = 0;
    GLuint m_texture = 0;
    GLuint m_blurFbo[2] = {0, 0};
    GLuint m_blurTexture[2] = {0, 0};

    Shader m_volShader;
    Shader m_blurShader;
    
    int m_width, m_height;
};