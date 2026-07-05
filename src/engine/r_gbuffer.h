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
#pragma once
#include <bgfx/bgfx.h>

class R_GBuffer
{
public:
    R_GBuffer();
    ~R_GBuffer();

    bool Init(int width, int height);
    void Shutdown();
    void Rescale(int width, int height);

    bgfx::FrameBufferHandle GetFBO() const;
    bgfx::TextureHandle GetDepthTex() const;
    bgfx::TextureHandle GetAlbedoTex() const;
    bgfx::TextureHandle GetNormalTex() const;
    bgfx::TextureHandle GetMRAOTex() const;
    bgfx::TextureHandle GetLightmapUVTex() const;

    void DrawDebug(int w, int h);

private:
    R_Shader m_debugShader;
    bgfx::UniformHandle m_sTex0 = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sTex1 = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uMode = BGFX_INVALID_HANDLE;

    bgfx::FrameBufferHandle m_fbo = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_normalTex = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_albedoTex = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_mraoTex = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_lightmapUVTex = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_depthTex = BGFX_INVALID_HANDLE;

    int m_width = 0;
    int m_height = 0;
};