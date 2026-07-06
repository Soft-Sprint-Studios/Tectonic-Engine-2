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
#include "renderer.h"
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

    m_debugShader.Load("shaders/gbuffer_debug.vert", "shaders/gbuffer_debug.frag");
    m_sTex0 = bgfx::createUniform("s_tex0", bgfx::UniformType::Sampler);
    m_sTex1 = bgfx::createUniform("s_tex1", bgfx::UniformType::Sampler);
    m_uMode = bgfx::createUniform("u_mode", bgfx::UniformType::Vec4);

    return true;
}

void R_GBuffer::Shutdown()
{
    if (bgfx::isValid(m_fbo))
    {
        bgfx::destroy(m_fbo);
        m_fbo = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_sTex0))
    {
        bgfx::destroy(m_sTex0);
        bgfx::destroy(m_sTex1);
        bgfx::destroy(m_uMode);
        m_sTex0 = m_sTex1 = m_uMode = BGFX_INVALID_HANDLE;
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

void R_GBuffer::DrawDebug(int w, int h)
{
    int dw = w / 8;
    int dh = h / 8;
    int debugY = 10;

    bgfx::VertexLayout layout;
    layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

    struct DebugVertex
    {
        float x, y, z;
        float u, v;
    };

    DebugVertex vertices[] =
    {
        { -1.0f,  1.0f, 0.0f, 0.0f, 0.0f },
        { -1.0f, -1.0f, 0.0f, 0.0f, 1.0f },
        {  1.0f, -1.0f, 0.0f, 1.0f, 1.0f },
        { -1.0f,  1.0f, 0.0f, 0.0f, 0.0f },
        {  1.0f, -1.0f, 0.0f, 1.0f, 1.0f },
        {  1.0f,  1.0f, 0.0f, 1.0f, 0.0f }
    };

    auto SubmitPass = [&](bgfx::ViewId viewId, int x, int y, int mode, bgfx::TextureHandle tex0, bgfx::TextureHandle tex1)
        {
            bgfx::setViewRect(viewId, (uint16_t)x, (uint16_t)y, (uint16_t)dw, (uint16_t)dh);
            bgfx::setViewClear(viewId, BGFX_CLEAR_NONE, 0, 1.0f, 0);
            bgfx::setViewTransform(viewId, nullptr, nullptr);

            float modeParams[4] = { (float)mode, 0.0f, 0.0f, 0.0f };
            bgfx::setUniform(m_uMode, modeParams);

            bgfx::setTexture(0, m_sTex0, tex0);
            if (bgfx::isValid(tex1))
            {
                bgfx::setTexture(1, m_sTex1, tex1);
            }

            bgfx::TransientVertexBuffer tvb;
            bgfx::allocTransientVertexBuffer(&tvb, 6, layout);
            std::memcpy(tvb.data, vertices, sizeof(vertices));

            bgfx::setVertexBuffer(0, &tvb);
            bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
            bgfx::submit(viewId, m_debugShader.GetProgram());
        };

    SubmitPass(RenderView::GBufferDebug0, 0, debugY, 0, m_depthTex, BGFX_INVALID_HANDLE);
    SubmitPass(RenderView::GBufferDebug1, dw, debugY, 1, m_normalTex, BGFX_INVALID_HANDLE);
    SubmitPass(RenderView::GBufferDebug2, dw * 2, debugY, 7, m_albedoTex, m_mraoTex);
    SubmitPass(RenderView::GBufferDebug3, dw * 3, debugY, 2, m_albedoTex, BGFX_INVALID_HANDLE);
    SubmitPass(RenderView::GBufferDebug4, dw * 4, debugY, 4, m_mraoTex, BGFX_INVALID_HANDLE);
    SubmitPass(RenderView::GBufferDebug5, dw * 5, debugY, 5, m_mraoTex, BGFX_INVALID_HANDLE);
    SubmitPass(RenderView::GBufferDebug6, dw * 6, debugY, 6, m_mraoTex, BGFX_INVALID_HANDLE);
    SubmitPass(RenderView::GBufferDebug7, dw * 7, debugY, 3, m_lightmapUVTex, BGFX_INVALID_HANDLE);
}