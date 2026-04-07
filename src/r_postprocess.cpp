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
#include "r_postprocess.h"
#include "console.h"
#include "cvar.h"
#include "postprocess.h"
#include "timing.h"

R_PostProcess::R_PostProcess()
    : m_fbo(0), m_texture(0), m_rbo(0),
    m_msFbo(0), m_msTexture(0), m_msRbo(0),
    m_quadVAO(0), m_quadVBO(0), m_width(0), m_height(0)
{
}

R_PostProcess::~R_PostProcess()
{
    Shutdown();
}

bool R_PostProcess::Init(int width, int height)
{
    m_width = width;
    m_height = height;

    if (!m_shader.Load("shaders/postprocess.vert", "shaders/postprocess.frag"))
    {
        Console::Error("PostProcess: Failed to load shaders");
        return false;
    }

    SetupBuffers();

    // Full-screen quad
    float quadVertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
    return true;
}

void R_PostProcess::SetupBuffers()
{
    if (m_fbo != 0)
    {
        glDeleteFramebuffers(1, &m_fbo);
        glDeleteTextures(1, &m_texture);
        glDeleteRenderbuffers(1, &m_rbo);
    }
    if (m_msFbo != 0)
    {
        glDeleteFramebuffers(1, &m_msFbo);
        glDeleteTextures(1, &m_msTexture);
        glDeleteRenderbuffers(1, &m_msRbo);
        m_msFbo = 0;
    }

    int samples = 4;
    bool useMSAA = false;

    if (CVar* cvSamples = CVar::Find("r_multisample_samples"))
    {
        samples = cvSamples->GetInt();
    }
    if (CVar* cvMSAA = CVar::Find("r_multisample"))
    {
        useMSAA = cvMSAA->GetInt() > 0;
    }

    // Setup resolve FBO
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_width, m_height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);

    if (!useMSAA)
    {
        glGenRenderbuffers(1, &m_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        Console::Error("PostProcess: Resolve Framebuffer is not complete!");
    }

    // Setup multisample FBO
    if (useMSAA)
    {
        glGenFramebuffers(1, &m_msFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_msFbo);

        glGenTextures(1, &m_msTexture);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_msTexture);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB16F, m_width, m_height, GL_TRUE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_msTexture, 0);

        glGenRenderbuffers(1, &m_msRbo);
        glBindRenderbuffer(GL_RENDERBUFFER, m_msRbo);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, m_width, m_height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_msRbo);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            Console::Error("PostProcess: Multisample Framebuffer is not complete!");
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void R_PostProcess::Begin()
{
    bool useMSAA = false;
    if (CVar* cvMSAA = CVar::Find("r_multisample"))
    {
        useMSAA = cvMSAA->GetInt() > 0;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, (useMSAA && m_msFbo != 0) ? m_msFbo : m_fbo);

    if (useMSAA)
    {
        glEnable(GL_MULTISAMPLE);
    }
    else
    {
        glDisable(GL_MULTISAMPLE);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void R_PostProcess::End()
{
    bool useMSAA = false;
    if (CVar* cvMSAA = CVar::Find("r_multisample"))
    {
        useMSAA = cvMSAA->GetInt() > 0;
    }

    if (useMSAA && m_msFbo != 0)
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_msFbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
        glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void R_PostProcess::Draw()
{
    m_shader.Bind();
    m_shader.SetInt("screenTexture", 0);

    const auto& ppSettings = PostProcess::GetCurrentSettings();
    m_shader.SetFloat("u_time", (float)Time::TotalTime());
    m_shader.SetFloat("u_vignetteStrength", ppSettings.vignetteStrength);
    m_shader.SetFloat("u_chromaStrength", ppSettings.chromaStrength);
    m_shader.SetFloat("u_grainStrength", ppSettings.grainStrength);
    m_shader.SetFloat("u_bwStrength", ppSettings.bwStrength);

    glBindVertexArray(m_quadVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void R_PostProcess::Rescale(int width, int height)
{
    m_width = width;
    m_height = height;
    SetupBuffers();
}

void R_PostProcess::Shutdown()
{
    if (m_fbo != 0) 
        glDeleteFramebuffers(1, &m_fbo);
    if (m_texture != 0) 
        glDeleteTextures(1, &m_texture);
    if (m_rbo != 0) 
        glDeleteRenderbuffers(1, &m_rbo);

    if (m_msFbo != 0) 
        glDeleteFramebuffers(1, &m_msFbo);
    if (m_msTexture != 0) 
        glDeleteTextures(1, &m_msTexture);
    if (m_msRbo != 0) 
        glDeleteRenderbuffers(1, &m_msRbo);

    if (m_quadVAO != 0) 
        glDeleteVertexArrays(1, &m_quadVAO);
    if (m_quadVBO != 0) 
        glDeleteBuffers(1, &m_quadVBO);

    m_fbo = m_msFbo = 0;
}