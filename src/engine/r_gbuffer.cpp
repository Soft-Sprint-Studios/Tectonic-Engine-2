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
#include "console.h"

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

    uint64_t rtFlags = BGFX_TEXTURE_RT | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;

    m_normalTex = bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1, bgfx::TextureFormat::RG16F, rtFlags);
    m_albedoTex = bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1, bgfx::TextureFormat::RGBA8, rtFlags);
    m_mraoTex = bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1, bgfx::TextureFormat::RGBA8, rtFlags);
    m_lightmapUVTex = bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1, bgfx::TextureFormat::RG32F, rtFlags);
    m_depthTex = bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1, bgfx::TextureFormat::D24S8, rtFlags);

    bgfx::TextureHandle attachments[] =
    {
        m_normalTex,
        m_albedoTex,
        m_mraoTex,
        m_lightmapUVTex,
        m_depthTex
    };

    m_fbo = bgfx::createFrameBuffer(5, attachments, true);

    return true;
}

void R_GBuffer::Shutdown()
{
    if (bgfx::isValid(m_fbo))
    {
        bgfx::destroy(m_fbo);
        m_fbo = BGFX_INVALID_HANDLE;
    }
}

void R_GBuffer::Rescale(int width, int height)
{
    Shutdown();
    Init(width, height);
}

bgfx::FrameBufferHandle R_GBuffer::GetFBO() const
{
    return m_fbo;
}

bgfx::TextureHandle R_GBuffer::GetDepthTex() const
{
    return m_depthTex;
}

bgfx::TextureHandle R_GBuffer::GetAlbedoTex() const
{
    return m_albedoTex;
}

bgfx::TextureHandle R_GBuffer::GetNormalTex() const
{
    return m_normalTex;
}

bgfx::TextureHandle R_GBuffer::GetMRAOTex() const
{
    return m_mraoTex;
}

bgfx::TextureHandle R_GBuffer::GetLightmapUVTex() const
{
    return m_lightmapUVTex;
}