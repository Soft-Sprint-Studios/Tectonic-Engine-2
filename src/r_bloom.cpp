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
#include "r_bloom.h"
#include "cvar.h"
#include <cmath>

CVar r_bloom("r_bloom", "1", CVAR_SAVE);
CVar r_bloom_intensity("r_bloom_intensity", "2", CVAR_SAVE);
CVar r_bloom_threshold("r_bloom_threshold", "0.9", CVAR_SAVE);
CVar r_bloom_scatter("r_bloom_scatter", "0.7", CVAR_SAVE);
CVar r_bloom_passes("r_bloom_passes", "6", CVAR_SAVE);

R_Bloom::R_Bloom()
{
}

R_Bloom::~R_Bloom()
{
    Shutdown();
}

bool R_Bloom::Init(int width, int height)
{
    m_downsampleShader.Load("shaders/postprocess.vert", "shaders/bloom_downsample.frag");
    m_upsampleShader.Load("shaders/postprocess.vert", "shaders/bloom_upsample.frag");
    CreateChain(width, height);
    return true;
}

void R_Bloom::Shutdown()
{
    DeleteChain();
}

void R_Bloom::Rescale(int width, int height)
{
    DeleteChain();
    CreateChain(width, height);
}

void R_Bloom::CreateChain(int width, int height)
{
    glm::ivec2 mipSize(width, height);
    int passes = std::max(1, r_bloom_passes.GetInt());

    for (int i = 0; i < passes; i++)
    {
        mipSize /= 2;
        if (mipSize.x == 0 || mipSize.y == 0)
            break;

        Mip mip;
        mip.size = mipSize;

        glGenFramebuffers(1, &mip.fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, mip.fbo);

        glGenTextures(1, &mip.texture);
        glBindTexture(GL_TEXTURE_2D, mip.texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, mip.size.x, mip.size.y, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mip.texture, 0);

        m_mipChain.push_back(mip);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void R_Bloom::DeleteChain()
{
    for (const auto& mip : m_mipChain)
    {
        glDeleteFramebuffers(1, &mip.fbo);
        glDeleteTextures(1, &mip.texture);
    }
    m_mipChain.clear();
}

void R_Bloom::Render(GLuint sourceTexture, GLuint quadVAO, int screenW, int screenH)
{
    if (r_bloom.GetInt() == 0 || m_mipChain.empty())
        return;

    m_downsampleShader.Bind();
    m_downsampleShader.SetFloat("u_threshold", r_bloom_threshold.GetFloat());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sourceTexture);

    glBindVertexArray(quadVAO);
    for (size_t i = 0; i < m_mipChain.size(); ++i)
    {
        const auto& mip = m_mipChain[i];
        glViewport(0, 0, mip.size.x, mip.size.y);
        glBindFramebuffer(GL_FRAMEBUFFER, mip.fbo);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindTexture(GL_TEXTURE_2D, mip.texture);
        m_downsampleShader.SetFloat("u_threshold", 0.0f);
    }

    m_upsampleShader.Bind();
    m_upsampleShader.SetFloat("u_scatter", r_bloom_scatter.GetFloat());
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    for (int i = (int)m_mipChain.size() - 1; i > 0; --i)
    {
        const auto& mip = m_mipChain[i];
        const auto& nextMip = m_mipChain[i - 1];

        glBindTexture(GL_TEXTURE_2D, mip.texture);
        glViewport(0, 0, nextMip.size.x, nextMip.size.y);
        glBindFramebuffer(GL_FRAMEBUFFER, nextMip.fbo);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenW, screenH);
}

void R_Bloom::Bind(const Shader& shader)
{
    if (r_bloom.GetInt() > 0 && !m_mipChain.empty())
    {
        shader.SetInt("u_bloom_enabled", 1);
        shader.SetFloat("u_bloom_intensity", r_bloom_intensity.GetFloat());
        shader.SetInt("bloomTexture", 2);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_mipChain[0].texture);
    }
    else
    {
        shader.SetInt("u_bloom_enabled", 0);
    }
}

GLuint R_Bloom::GetResultTexture() const
{
    return m_mipChain.empty() ? 0 : m_mipChain[0].texture;
}