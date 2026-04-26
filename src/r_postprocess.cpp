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
#include "r_state.h"
#include "console.h"
#include "cvar.h"
#include "postprocess.h"
#include "timing.h"
#include "r_bloom.h"

CVar r_postprocess("r_postprocess", "1", "Enable entire post-processing stack.", CVAR_SAVE);
CVar r_gamma("r_gamma", "1.7", "Display gamma correction value.", CVAR_SAVE);
CVar r_tonemap("r_tonemap", "1", "Enable filmic ACES tonemapping.", CVAR_SAVE);

R_PostProcess::R_PostProcess()
    : m_fbo(0), m_texture(0), m_depthTexture(0),
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

    m_shader.Load("shaders/postprocess.vert", "shaders/postprocess.frag");

    m_bloom = std::make_unique<R_Bloom>();
    m_bloom->Init(m_width, m_height);

    m_volumetrics = std::make_unique<R_Volumetrics>();
    m_volumetrics->Init(m_width, m_height);

    m_autoExposure = std::make_unique<R_AutoExposure>();
    m_autoExposure->Init();

    m_ssao = std::make_unique<R_SSAO>();
    m_ssao->Init(m_width, m_height);

    SetupBuffers();

    // Full-screen quad
    float quadVertices[] = 
    {
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
        glDeleteTextures(1, &m_depthTexture);
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);

    glGenTextures(1, &m_depthTexture);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);

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

    R_State::SetMultisample(useMSAA);

    R_State::Clear(true, true, false);
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
        glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void R_PostProcess::Draw(const Camera& camera, R_Lights* lights)
{
    // Run the subrenderers
    m_autoExposure->Update(m_texture, m_width, m_height);
    m_bloom->Render(m_texture, m_quadVAO, m_width, m_height);
    m_volumetrics->Render(m_depthTexture, camera, lights, m_quadVAO, m_width, m_height);
    m_ssao->Render(m_depthTexture, camera, m_quadVAO, m_width, m_height);

    m_shader.Bind();
    m_shader.SetInt("u_screenTexture", 0);
    m_shader.SetInt("u_depthTexture", 1);
    m_autoExposure->Bind();
    m_bloom->Bind(m_shader);
    m_volumetrics->Bind(m_shader);
    m_ssao->Bind(m_shader);

    m_shader.SetInt("u_postprocess_enabled", r_postprocess.GetInt());

    const auto& ppSettings = PostProcess::GetCurrentSettings();
    m_shader.SetFloat("u_time", (float)Time::TotalTime());
    m_shader.SetFloat("u_vignetteStrength", ppSettings.vignetteStrength);
    m_shader.SetFloat("u_chromaStrength", ppSettings.chromaStrength);
    m_shader.SetFloat("u_grainStrength", ppSettings.grainStrength);
    m_shader.SetFloat("u_bwStrength", ppSettings.bwStrength);
    m_shader.SetFloat("u_sharpenStrength", ppSettings.sharpenStrength);
    m_shader.SetInt("u_fogEnabled", ppSettings.fogEnabled);
    m_shader.SetVec3("u_fogColor", ppSettings.fogColor);
    m_shader.SetFloat("u_fogStart", ppSettings.fogStart);
    m_shader.SetFloat("u_fogEnd", ppSettings.fogEnd);
    m_shader.SetInt("u_fogAffectsSky", ppSettings.fogAffectsSky ? 1 : 0);
    m_shader.SetInt("u_tonemap_enabled", r_tonemap.GetInt());
    m_shader.SetFloat("u_Gamma", r_gamma.GetFloat());
    m_shader.SetMat4("u_invProjection", glm::inverse(camera.GetProjectionMatrix()));

    glBindVertexArray(m_quadVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void R_PostProcess::Rescale(int width, int height)
{
    m_width = width;
    m_height = height;
    m_bloom->Rescale(width, height);
    m_volumetrics->Rescale(width, height);
    m_ssao->Rescale(width, height);
    SetupBuffers();
}

void R_PostProcess::Shutdown()
{
    if (m_bloom)
    {
        m_bloom->Shutdown();
        m_bloom.reset();
    }

    if (m_volumetrics)
    {
        m_volumetrics->Shutdown();
        m_volumetrics.reset();
    }

    if (m_autoExposure)
    {
        m_autoExposure->Shutdown();
        m_autoExposure.reset();
    }

    if (m_ssao)
    {
        m_ssao->Shutdown();
        m_ssao.reset();
    }

    if (m_fbo != 0) 
        glDeleteFramebuffers(1, &m_fbo);
    if (m_texture != 0) 
        glDeleteTextures(1, &m_texture);
    if (m_depthTexture != 0)
        glDeleteTextures(1, &m_depthTexture);

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