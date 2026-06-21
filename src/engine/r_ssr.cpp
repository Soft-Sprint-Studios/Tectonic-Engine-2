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
#include "r_ssr.h"
#include "cvar.h"

CVar r_ssr("r_ssr", "0", "Enable Screen Space Reflections.", CVAR_SAVE);
CVar r_ssr_max_dist("r_ssr_max_dist", "20.0", "Maximum ray distance for SSR.", CVAR_SAVE);
CVar r_ssr_resolution("r_ssr_resolution", "0.3", "Step resolution for ray marching.", CVAR_SAVE);
CVar r_ssr_thickness("r_ssr_thickness", "1.0", "Z-thickness for ray collision.", CVAR_SAVE);
CVar r_ssr_steps("r_ssr_steps", "64", "Number of linear search steps for SSR.", CVAR_SAVE);
CVar r_ssr_binary_steps("r_ssr_binary_steps", "16", "Number of binary search refinement steps.", CVAR_SAVE);
CVar r_ssr_downsample("r_ssr_downsample", "2", "Downscaling factor for SSR buffer (higher = faster).", CVAR_SAVE);

R_SSR::R_SSR() 
{
}

R_SSR::~R_SSR() 
{ 
    Shutdown(); 
}

bool R_SSR::Init(int width, int height)
{
    m_ssrShader.Load("shaders/ssr.vert", "shaders/ssr.frag");
    m_blurShader.Load("shaders/ssr.vert", "shaders/ssr_blur.frag");
    CreateBuffers(width, height);
    return true;
}

void R_SSR::CreateBuffers(int width, int height)
{
    m_width = width;
    m_height = height;

    int ds = std::max(1, r_ssr_downsample.GetInt());
    int ssrW = width / ds;
    int ssrH = height / ds;

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, ssrW, ssrH, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);

    glGenFramebuffers(1, &m_blurFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_blurFbo);

    glGenTextures(1, &m_blurTexture);
    glBindTexture(GL_TEXTURE_2D, m_blurTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, ssrW, ssrH, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_blurTexture, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void R_SSR::Render(GLuint depthTex, GLuint normalTex, GLuint mraoTex, GLuint sceneTex, const Camera& camera, GLuint quadVAO)
{
    if (r_ssr.GetInt() == 0) 
    {
        return;
    }

    int ds = std::max(1, r_ssr_downsample.GetInt());
    int ssrW = m_width / ds;
    int ssrH = m_height / ds;

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, ssrW, ssrH);
    glClear(GL_COLOR_BUFFER_BIT);

    m_ssrShader.Bind();
    m_ssrShader.SetMat4("u_projection", camera.GetProjectionMatrix());
    m_ssrShader.SetMat4("u_invProjection", glm::inverse(camera.GetProjectionMatrix()));
    m_ssrShader.SetMat4("u_view", camera.GetViewMatrix());
    
    m_ssrShader.SetFloat("u_maxDist", r_ssr_max_dist.GetFloat());
    m_ssrShader.SetFloat("u_resolution", r_ssr_resolution.GetFloat());
    m_ssrShader.SetFloat("u_thickness", r_ssr_thickness.GetFloat());
    m_ssrShader.SetInt("u_maxSteps", r_ssr_steps.GetInt());
    m_ssrShader.SetInt("u_binarySteps", r_ssr_binary_steps.GetInt());

    glBindTextureUnit(0, depthTex);
    glBindTextureUnit(1, normalTex);
    glBindTextureUnit(2, mraoTex);
    glBindTextureUnit(3, sceneTex);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindFramebuffer(GL_FRAMEBUFFER, m_blurFbo);
    m_blurShader.Bind();
    glBindTextureUnit(0, m_texture);
    m_blurShader.SetInt("u_ssrTex", 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);
}

void R_SSR::Bind(const R_Shader& shader)
{
    if (r_ssr.GetInt() > 0)
    {
        shader.SetInt("u_ssr_enabled", 1);
        glBindTextureUnit(5, m_blurTexture);
    }
    else
    {
        shader.SetInt("u_ssr_enabled", 0);
    }
}

void R_SSR::DeleteBuffers()
{
    if (m_fbo) 
        glDeleteFramebuffers(1, &m_fbo);
    if (m_texture)
        glDeleteTextures(1, &m_texture);
    if (m_blurFbo) 
        glDeleteFramebuffers(1, &m_blurFbo);
    if (m_blurTexture)
        glDeleteTextures(1, &m_blurTexture);
}

void R_SSR::Rescale(int width, int height) 
{ 
    DeleteBuffers(); 
    CreateBuffers(width, height); 
}

void R_SSR::Shutdown() 
{ 
    DeleteBuffers(); 
}