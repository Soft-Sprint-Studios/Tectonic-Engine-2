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
#include "r_ui.h"
#include <glad/glad.h>

class R_GBuffer
{
public:
    R_GBuffer();
    ~R_GBuffer();

    bool Init(int width, int height);
    void Shutdown();
    void Rescale(int width, int height);

    void Bind();
    void Unbind();

    void DrawDebug(int w, int h);

    GLuint GetFBO() const;
    GLuint GetNormalTex() const;
    GLuint GetAlbedoSpecTex() const;
    GLuint GetLightmapUVTex() const;
    GLuint GetDepthTex() const;

private:
    GLuint m_fbo = 0;
    GLuint m_normalTex = 0;
    GLuint m_albedoSpecTex = 0;
    GLuint m_lightmapUVTex = 0;
    GLuint m_depthTex = 0;

    R_Shader m_debugShader;
    GLuint m_quadVAO = 0;
    GLuint m_quadVBO = 0;
    void InitDebugQuad();

    int m_width = 0;
    int m_height = 0;
};