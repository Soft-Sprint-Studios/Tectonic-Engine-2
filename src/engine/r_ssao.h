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
#include "camera.h"
#include <glad/glad.h>
#include <vector>
#include <glm/glm.hpp>

class R_SSAO
{
public:
    R_SSAO();
    ~R_SSAO();

    bool Init(int width, int height);
    void Shutdown();
    void Rescale(int width, int height);
    void Render(GLuint depthTexture, const Camera& camera, GLuint quadVAO, int screenW, int screenH);
    void Bind(const R_Shader& shader);

private:
    void CreateBuffers(int width, int height);
    void DeleteBuffers();
    void GenerateSampleKernel();
    void GenerateNoiseTexture();

    GLuint m_fbo = 0;
    GLuint m_texture = 0;
    GLuint m_blurFbo = 0;
    GLuint m_blurTexture = 0;
    GLuint m_noiseTexture = 0;

    R_Shader m_ssaoShader;
    R_Shader m_blurShader;
    
    std::vector<glm::vec3> m_ssaoKernel;

    int m_width, m_height;
};