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
#include "r_autoexposure.h"
#include "cvar.h"
#include "timing.h"

CVar r_autoexposure("r_autoexposure", "1", CVAR_SAVE);
CVar r_exposure_speed("r_exposure_speed", "1.5", CVAR_SAVE);
CVar r_exposure_target("r_exposure_target", "0.12", CVAR_SAVE);
CVar r_exposure_min("r_exposure_min", "0.85", CVAR_SAVE);
CVar r_exposure_max("r_exposure_max", "1.8", CVAR_SAVE);

R_AutoExposure::R_AutoExposure()
{
}

R_AutoExposure::~R_AutoExposure()
{
    Shutdown();
}

void R_AutoExposure::Init()
{
    m_histogramShader.LoadCompute("shaders/lum_histogram.comp");
    m_averageShader.LoadCompute("shaders/lum_average.comp");

    glGenBuffers(1, &m_histogramBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_histogramBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 256 * sizeof(uint32_t), NULL, GL_DYNAMIC_COPY);

    glGenBuffers(1, &m_lumBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_lumBuffer);
    float initialExposure = 1.0f;
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float), &initialExposure, GL_DYNAMIC_COPY);
}

void R_AutoExposure::Update(GLuint screenTexture, int width, int height)
{
    // Run the autoexposure compute shader
    if (r_autoexposure.GetInt() > 0)
    {
        uint32_t zeros[256] = { 0 };
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_histogramBuffer);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(zeros), zeros);

        m_histogramShader.Bind();
        glBindImageTexture(0, screenTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_histogramBuffer);
        glDispatchCompute((width + 15) / 16, (height + 15) / 16, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        m_averageShader.Bind();
        m_averageShader.SetFloat("u_deltaTime", Time::DeltaTime());
        m_averageShader.SetFloat("u_speed", r_exposure_speed.GetFloat());
        m_averageShader.SetFloat("u_targetLum", r_exposure_target.GetFloat());
        m_averageShader.SetFloat("u_minExp", r_exposure_min.GetFloat());
        m_averageShader.SetFloat("u_maxExp", r_exposure_max.GetFloat());

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_histogramBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_lumBuffer);
        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
    else
    {
        float defaultExp = 1.0f;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_lumBuffer);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float), &defaultExp);
    }
}

void R_AutoExposure::Bind()
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_lumBuffer);
}

void R_AutoExposure::Shutdown()
{
    if (m_histogramBuffer) 
        glDeleteBuffers(1, &m_histogramBuffer);
    if (m_lumBuffer) 
        glDeleteBuffers(1, &m_lumBuffer);

    m_histogramBuffer = m_lumBuffer = 0;
}