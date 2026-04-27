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
#include "r_bloom.h"
#include "r_volumetrics.h"
#include "r_autoexposure.h"
#include "r_ssao.h"
#include "camera.h"
#include "r_shader.h"
#include <glad/glad.h>
#include <memory>

class R_PostProcess
{
public:
    R_PostProcess();
    ~R_PostProcess();

    bool Init(int width, int height);
    void Begin();
    void End();
    void Draw(const Camera& camera, class R_Lights* lights);
    void Rescale(int width, int height);
    void Shutdown();

    GLuint GetDepthTexture() 
    { 
        return m_depthTexture;
    }

private:
    GLuint m_fbo;
    GLuint m_texture;
    GLuint m_depthTexture;

    GLuint m_msFbo;
    GLuint m_msTexture;
    GLuint m_msRbo;

    GLuint m_quadVAO;
    GLuint m_quadVBO;
    R_Shader m_shader;

    int m_width, m_height;
    void SetupBuffers();

    // Postprocess subrenderers
    std::unique_ptr<R_Bloom> m_bloom;
    std::unique_ptr<R_Volumetrics> m_volumetrics;
    std::unique_ptr<R_AutoExposure> m_autoExposure;
    std::unique_ptr<R_SSAO> m_ssao;
};