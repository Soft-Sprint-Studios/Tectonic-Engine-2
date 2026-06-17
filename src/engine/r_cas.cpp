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
#include "r_cas.h"
#include "cvar.h"
#include <algorithm>
#include <cstring>

#define A_CPU 1
#include "ffx_a.h"
#include "ffx_cas.h"

CVar r_cas("r_cas", "1", "Enable FidelityFX Contrast Adaptive Sharpening.", CVAR_SAVE);
CVar r_cas_sharpness("r_cas_sharpness", "0.8", "CAS Sharpness intensity (0.0 to 1.0).", CVAR_SAVE);

R_CAS::R_CAS()
{
}

R_CAS::~R_CAS()
{
    Shutdown();
}

bool R_CAS::Init(int width, int height)
{
    m_computeShader.LoadCompute("shaders/cas.comp");
    m_blitShader.Load("shaders/cas_blit.vert", "shaders/cas_blit.frag");

    CreateBuffers(width, height);
    return true;
}

void R_CAS::CreateBuffers(int width, int height)
{
    glGenFramebuffers(1, &m_inputFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_inputFbo);

    glGenTextures(1, &m_inputTex);
    glBindTexture(GL_TEXTURE_2D, m_inputTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_inputTex, 0);

    glGenTextures(1, &m_outputTex);
    glBindTexture(GL_TEXTURE_2D, m_outputTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void R_CAS::DeleteBuffers()
{
    if (m_inputFbo) 
        glDeleteFramebuffers(1, &m_inputFbo);
    if (m_inputTex) 
        glDeleteTextures(1, &m_inputTex);
    if (m_outputTex) 
        glDeleteTextures(1, &m_outputTex);

    m_inputFbo = m_inputTex = m_outputTex = 0;
}

void R_CAS::Rescale(int width, int height)
{
    DeleteBuffers();
    CreateBuffers(width, height);
}

void R_CAS::Shutdown()
{
    DeleteBuffers();
}

void R_CAS::Render(GLuint quadVAO, int width, int height)
{
    m_computeShader.Bind();

    varAU4(const0);
    varAU4(const1);

    CasSetup(const0, const1, r_cas_sharpness.GetFloat(), (float)width, (float)height, (float)width, (float)height);

    glm::vec4 c0(glm::uintBitsToFloat(const0[0]), glm::uintBitsToFloat(const0[1]), glm::uintBitsToFloat(const0[2]), glm::uintBitsToFloat(const0[3]));
    glm::vec4 c1(glm::uintBitsToFloat(const1[0]), glm::uintBitsToFloat(const1[1]), glm::uintBitsToFloat(const1[2]), glm::uintBitsToFloat(const1[3]));

    m_computeShader.SetVec4("u_const0", c0);
    m_computeShader.SetVec4("u_const1", c1);

    glBindTextureUnit(0, m_inputTex);
    glBindImageTexture(1, m_outputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    int groupX = (width + 15) / 16;
    int groupY = (height + 15) / 16;

    glDispatchCompute(groupX, groupY, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);

    m_blitShader.Bind();
    glBindTextureUnit(0, m_outputTex);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}