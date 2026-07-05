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
#include <algorithm>

CVar r_autoexposure("r_autoexposure", "1", "Enable compute-based auto exposure.", CVAR_SAVE);
CVar r_exposure_speed("r_exposure_speed", "1.5", "Speed of exposure adjustment.", CVAR_SAVE);
CVar r_exposure_target("r_exposure_target", "0.12", "Target average luminance for the scene.", CVAR_SAVE);
CVar r_exposure_min("r_exposure_min", "0.85", "Minimum allowed exposure multiplier.", CVAR_SAVE);
CVar r_exposure_max("r_exposure_max", "1.8", "Maximum allowed exposure multiplier.", CVAR_SAVE);
CVar r_autoexposure_res("r_autoexposure_res", "4", "Downscale factor for auto exposure (higher = faster).", CVAR_SAVE);

R_AutoExposure::R_AutoExposure() 
{
}

R_AutoExposure::~R_AutoExposure() 
{ 
    Shutdown(); 
}

void R_AutoExposure::Init()
{
    m_clearShader.LoadCompute("shaders/lum_clear.comp");
    m_histogramShader.LoadCompute("shaders/lum_histogram.comp");
    m_averageShader.LoadCompute("shaders/lum_average.comp");

    m_uintLayout.begin()
        .add(bgfx::Attrib::TexCoord0, 1, bgfx::AttribType::Float)
        .end();

    m_vec4Layout.begin()
        .add(bgfx::Attrib::TexCoord0, 4, bgfx::AttribType::Float)
        .end();

    m_histogramBuffer = bgfx::createDynamicVertexBuffer(256, m_uintLayout, BGFX_BUFFER_COMPUTE_READ_WRITE);

    float initialLum[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
    const bgfx::Memory* mem = bgfx::copy(initialLum, sizeof(initialLum));
    m_lumBuffer = bgfx::createDynamicVertexBuffer(mem, m_vec4Layout, BGFX_BUFFER_COMPUTE_READ_WRITE);

    m_uExposureParams1 = bgfx::createUniform("u_exposureParams1", bgfx::UniformType::Vec4);
    m_uExposureParams2 = bgfx::createUniform("u_exposureParams2", bgfx::UniformType::Vec4);
}

void R_AutoExposure::Render(bgfx::ViewId viewId, bgfx::TextureHandle screenTexture, int width, int height)
{
    bgfx::setBuffer(0, m_histogramBuffer, bgfx::Access::Write);
    bgfx::dispatch(viewId, m_clearShader.GetProgram(), 256 / 64, 1, 1);

    int ds = std::max(1, r_autoexposure_res.GetInt());
    int vW = width / ds;
    int vH = height / ds;

    bgfx::setImage(0, screenTexture, 0, bgfx::Access::Read, bgfx::TextureFormat::RGBA16F);
    bgfx::setBuffer(1, m_histogramBuffer, bgfx::Access::ReadWrite);
    bgfx::dispatch(viewId, m_histogramShader.GetProgram(), (vW + 15) / 16, (vH + 15) / 16, 1);

    float p1[4] = { Time::DeltaTime(), r_exposure_speed.GetFloat(), r_exposure_target.GetFloat(), r_exposure_min.GetFloat() };
    float p2[4] = { r_exposure_max.GetFloat(), r_autoexposure.GetInt() > 0 ? 1.0f : 0.0f, 0.0f, 0.0f };

    bgfx::setUniform(m_uExposureParams1, p1);
    bgfx::setUniform(m_uExposureParams2, p2);
    bgfx::setBuffer(1, m_histogramBuffer, bgfx::Access::Read);
    bgfx::setBuffer(2, m_lumBuffer, bgfx::Access::ReadWrite);
    bgfx::dispatch(viewId, m_averageShader.GetProgram(), 1, 1, 1);
}

void R_AutoExposure::Bind()
{
    bgfx::setBuffer(6, m_lumBuffer, bgfx::Access::Read);
}

void R_AutoExposure::Shutdown()
{
    if (bgfx::isValid(m_histogramBuffer)) 
        bgfx::destroy(m_histogramBuffer);
    if (bgfx::isValid(m_lumBuffer)) 
        bgfx::destroy(m_lumBuffer);
    if (bgfx::isValid(m_uExposureParams1)) 
        bgfx::destroy(m_uExposureParams1);
    if (bgfx::isValid(m_uExposureParams2)) 
        bgfx::destroy(m_uExposureParams2);

    m_histogramBuffer = m_lumBuffer = BGFX_INVALID_HANDLE;
    m_uExposureParams1 = m_uExposureParams2 = BGFX_INVALID_HANDLE;
}