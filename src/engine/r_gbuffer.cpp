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
#include "r_gbuffer.h"
#include <iostream>

R_GBuffer::R_GBuffer()
{
}

R_GBuffer::~R_GBuffer()
{
    Shutdown();
}

bool R_GBuffer::Init(int width, int height)
{
    m_width = width;
    m_height = height;

    m_debugShader.Load("shaders/gbuffer_debug.vert", "shaders/gbuffer_debug.frag");
    m_debugShader.Bind();
    InitDebugQuad();

    glCreateFramebuffers(1, &m_fbo);

    // Normal
    glCreateTextures(GL_TEXTURE_2D, 1, &m_normalTex);
    glTextureStorage2D(m_normalTex, 1, GL_RGBA16F, width, height);
    glTextureParameteri(m_normalTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_normalTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glNamedFramebufferTexture(m_fbo, GL_COLOR_ATTACHMENT0, m_normalTex, 0);

    // Albedo
    glCreateTextures(GL_TEXTURE_2D, 1, &m_albedoTex);
    glTextureStorage2D(m_albedoTex, 1, GL_RGBA8, width, height);
    glTextureParameteri(m_albedoTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_albedoTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glNamedFramebufferTexture(m_fbo, GL_COLOR_ATTACHMENT1, m_albedoTex, 0);

    // MRAO (Metallic, Roughness, AO)
    glCreateTextures(GL_TEXTURE_2D, 1, &m_mraoTex);
    glTextureStorage2D(m_mraoTex, 1, GL_RGBA8, width, height);
    glTextureParameteri(m_mraoTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_mraoTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glNamedFramebufferTexture(m_fbo, GL_COLOR_ATTACHMENT2, m_mraoTex, 0);

    // Lightmap UVs
    glCreateTextures(GL_TEXTURE_2D, 1, &m_lightmapUVTex);
    glTextureStorage2D(m_lightmapUVTex, 1, GL_RG32F, width, height);
    glTextureParameteri(m_lightmapUVTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_lightmapUVTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glNamedFramebufferTexture(m_fbo, GL_COLOR_ATTACHMENT3, m_lightmapUVTex, 0);

    GLenum attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    glNamedFramebufferDrawBuffers(m_fbo, 4, attachments);

    // Depth Buffer
    glCreateTextures(GL_TEXTURE_2D, 1, &m_depthTex);
    glTextureStorage2D(m_depthTex, 1, GL_DEPTH_COMPONENT24, width, height);
    glTextureParameteri(m_depthTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_depthTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_depthTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTextureParameteri(m_depthTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTextureParameterfv(m_depthTex, GL_TEXTURE_BORDER_COLOR, borderColor);
    glNamedFramebufferTexture(m_fbo, GL_DEPTH_ATTACHMENT, m_depthTex, 0);

    return true;
}

void R_GBuffer::Shutdown()
{
    if (m_fbo != 0)
    {
        glDeleteFramebuffers(1, &m_fbo);
    }
    if (m_normalTex != 0)
    {
        glDeleteTextures(1, &m_normalTex);
    }
    if (m_albedoTex != 0)
    {
        glDeleteTextures(1, &m_albedoTex);
    }
    if (m_mraoTex != 0)
    {
        glDeleteTextures(1, &m_mraoTex);
    }
    if (m_lightmapUVTex != 0)
    {
        glDeleteTextures(1, &m_lightmapUVTex);
    }
    if (m_depthTex != 0)
    {
        glDeleteTextures(1, &m_depthTex);
    }
    if (m_quadVAO != 0)
    {
        glDeleteVertexArrays(1, &m_quadVAO);
        m_quadVAO = 0;
    }
    if (m_quadVBO != 0)
    {
        glDeleteBuffers(1, &m_quadVBO);
        m_quadVBO = 0;
    }

    m_fbo = 0;
    m_normalTex = 0;
    m_albedoTex = 0;
    m_mraoTex = 0;
    m_lightmapUVTex = 0;
    m_depthTex = 0;
}

void R_GBuffer::Rescale(int width, int height)
{
    Shutdown();
    Init(width, height);
}

void R_GBuffer::Bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
}

void R_GBuffer::Unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void R_GBuffer::InitDebugQuad()
{
    float quadVertices[] =
    {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glCreateVertexArrays(1, &m_quadVAO);
    glCreateBuffers(1, &m_quadVBO);
    glNamedBufferData(m_quadVBO, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

    glVertexArrayVertexBuffer(m_quadVAO, 0, m_quadVBO, 0, 4 * sizeof(float));

    glEnableVertexArrayAttrib(m_quadVAO, 0);
    glVertexArrayAttribFormat(m_quadVAO, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(m_quadVAO, 0, 0);

    glEnableVertexArrayAttrib(m_quadVAO, 1);
    glVertexArrayAttribFormat(m_quadVAO, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
    glVertexArrayAttribBinding(m_quadVAO, 1, 0);
}

void R_GBuffer::DrawDebug(int w, int h)
{
    glDisable(GL_DEPTH_TEST);
    m_debugShader.Bind();

    int dw = w / 8;
    int dh = h / 8;
    int debugY = h - dh - 10;

    glBindVertexArray(m_quadVAO);

    glViewport(0, debugY, dw, dh);
    m_debugShader.SetInt("u_mode", 0);
    glBindTextureUnit(0, m_depthTex);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glViewport(dw, debugY, dw, dh);
    m_debugShader.SetInt("u_mode", 1);
    glBindTextureUnit(0, m_normalTex);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glViewport(dw * 2, debugY, dw, dh);
    m_debugShader.SetInt("u_mode", 7);
    glBindTextureUnit(0, m_normalTex);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glViewport(dw * 3, debugY, dw, dh);
    m_debugShader.SetInt("u_mode", 2);
    glBindTextureUnit(0, m_albedoTex);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glViewport(dw * 4, debugY, dw, dh);
    m_debugShader.SetInt("u_mode", 4);
    glBindTextureUnit(0, m_mraoTex);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glViewport(dw * 5, debugY, dw, dh);
    m_debugShader.SetInt("u_mode", 5);
    glBindTextureUnit(0, m_mraoTex);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glViewport(dw * 6, debugY, dw, dh);
    m_debugShader.SetInt("u_mode", 6);
    glBindTextureUnit(0, m_mraoTex);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glViewport(dw * 7, debugY, dw, dh);
    m_debugShader.SetInt("u_mode", 3);
    glBindTextureUnit(0, m_lightmapUVTex);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glViewport(0, 0, w, h);
    glEnable(GL_DEPTH_TEST);
}

GLuint R_GBuffer::GetFBO() const
{
    return m_fbo;
}

GLuint R_GBuffer::GetNormalTex() const
{
    return m_normalTex;
}

GLuint R_GBuffer::GetAlbedoTex() const
{
    return m_albedoTex;
}

GLuint R_GBuffer::GetMRAOTex() const
{
    return m_mraoTex;
}

GLuint R_GBuffer::GetLightmapUVTex() const
{
    return m_lightmapUVTex;
}

GLuint R_GBuffer::GetDepthTex() const
{
    return m_depthTex;
}