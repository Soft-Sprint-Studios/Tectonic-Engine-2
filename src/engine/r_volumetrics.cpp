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
#include "r_volumetrics.h"
#include "cvar.h"
#include <algorithm>

CVar r_volumetrics("r_volumetrics", "1", "Enable volumetric lighting (God rays).", CVAR_SAVE);
CVar r_volumetrics_downsample("r_volumetrics_downsample", "8", "Downscaling factor for volumetric buffer (higher = faster).", CVAR_SAVE);
CVar r_volumetrics_blur_passes("r_volumetrics_blur_passes", "6", "Number of blur passes for volumetrics.", CVAR_SAVE);

R_Volumetrics::R_Volumetrics() 
{
}

R_Volumetrics::~R_Volumetrics() 
{
    Shutdown();
}

bool R_Volumetrics::Init(int width, int height) 
{
    m_volShader.Load("shaders/volumetrics.vert", "shaders/volumetrics.frag");
    m_blurShader.Load("shaders/volumetrics_blur.vert", "shaders/volumetrics_blur.frag");
    CreateBuffers(width, height);
    return true;
}

void R_Volumetrics::CreateBuffers(int width, int height) 
{
    m_width = width;
    m_height = height;

    int ds = std::max(1, r_volumetrics_downsample.GetInt());
    int vW = width / ds;
    int vH = height / ds;

    glCreateFramebuffers(1, &m_fbo);
    glCreateTextures(GL_TEXTURE_2D, 1, &m_texture);
    glTextureStorage2D(m_texture, 1, GL_RGB16F, vW, vH);
    glTextureParameteri(m_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glNamedFramebufferTexture(m_fbo, GL_COLOR_ATTACHMENT0, m_texture, 0);

    glCreateFramebuffers(2, m_blurFbo);
    glCreateTextures(GL_TEXTURE_2D, 2, m_blurTexture);
    for (int i = 0; i < 2; i++)
    {
        glTextureStorage2D(m_blurTexture[i], 1, GL_RGB16F, vW, vH);
        glTextureParameteri(m_blurTexture[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_blurTexture[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(m_blurTexture[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_blurTexture[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glNamedFramebufferTexture(m_blurFbo[i], GL_COLOR_ATTACHMENT0, m_blurTexture[i], 0);
    }
}

void R_Volumetrics::DeleteBuffers() 
{
    if (m_fbo) 
        glDeleteFramebuffers(1, &m_fbo);
    if (m_texture) 
        glDeleteTextures(1, &m_texture);
    if (m_blurFbo[0]) 
        glDeleteFramebuffers(2, m_blurFbo);
    if (m_blurTexture[0]) 
        glDeleteTextures(2, m_blurTexture);
    m_fbo = m_texture = m_blurFbo[0] = m_blurFbo[1] = m_blurTexture[0] = m_blurTexture[1] = 0;
}

void R_Volumetrics::Rescale(int width, int height) 
{
    DeleteBuffers();
    CreateBuffers(width, height);
}

void R_Volumetrics::Shutdown() 
{
    DeleteBuffers();
}

void R_Volumetrics::Render(GLuint depthTexture, const Camera& camera, R_Lights* lights, GLuint quadVAO, int screenW, int screenH) 
{
    if (r_volumetrics.GetInt() == 0) 
        return;

    // Volumetric pass
    int ds = std::max(1, r_volumetrics_downsample.GetInt());
    int vW = screenW / ds;
    int vH = screenH / ds;

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, vW, vH);
    glClear(GL_COLOR_BUFFER_BIT);

    m_volShader.Bind();
    m_volShader.SetMat4("u_invProjection", glm::inverse(camera.GetProjectionMatrix()));
    m_volShader.SetMat4("u_invView", glm::inverse(camera.GetViewMatrix()));
    m_volShader.SetVec3("u_viewPos", camera.position);

    glBindTextureUnit(0, depthTexture);

    if (lights) 
        lights->Bind(m_volShader);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Blur pass
    m_blurShader.Bind();
    bool horizontal = true, first_iteration = true;
    for (int i = 0; i < r_volumetrics_blur_passes.GetInt(); i++) 
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_blurFbo[horizontal]);
        m_blurShader.SetInt("horizontal", horizontal);
        
        glBindTextureUnit(0, first_iteration ? m_texture : m_blurTexture[!horizontal]);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        horizontal = !horizontal;
        if (first_iteration) 
            first_iteration = false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenW, screenH);
}

void R_Volumetrics::Bind(const R_Shader& shader)
{
    if (r_volumetrics.GetInt() > 0) 
    {
        shader.SetInt("u_volumetrics_enabled", 1);
        glBindTextureUnit(3, m_blurTexture[0]);
    } 
    else 
    {
        shader.SetInt("u_volumetrics_enabled", 0);
    }
}