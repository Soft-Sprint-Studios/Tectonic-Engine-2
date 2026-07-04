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
#include "r_shader.h"
#include "camera.h"
#include <bgfx/bgfx.h>

class R_SSR
{
public:
    R_SSR();
    ~R_SSR();

    bool Init(int width, int height);
    void Shutdown();
    void Rescale(int width, int height);

    void Render(bgfx::ViewId viewId, bgfx::TextureHandle depthTex, bgfx::TextureHandle normalTex, bgfx::TextureHandle mraoTex, bgfx::TextureHandle sceneTex, const Camera& camera);
    void Bind(bgfx::UniformHandle s_ssrTex);

    bgfx::TextureHandle GetTexture() const 
    { 
        return m_blurTexture; 
    }

private:
    void CreateBuffers(int width, int height);
    void DeleteBuffers();

    bgfx::TextureHandle m_texture = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_blurTexture = BGFX_INVALID_HANDLE;

    R_Shader m_ssrShader;
    R_Shader m_blurShader;

    bgfx::UniformHandle m_sDepth = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sNormal = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sMRAO = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sScene = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sSSR = BGFX_INVALID_HANDLE;

    bgfx::UniformHandle m_uParams1 = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uParams2 = BGFX_INVALID_HANDLE;

    int m_width = 0, m_height = 0;
};