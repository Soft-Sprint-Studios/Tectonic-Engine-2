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
#include "camera.h"
#include "r_shader.h"
#include <bgfx/bgfx.h>

class R_MotionBlur
{
public:
    R_MotionBlur();
    ~R_MotionBlur();

    bool Init(int width, int height);
    void Shutdown();
    void Rescale(int width, int height);
    void Render(bgfx::ViewId viewId, bgfx::TextureHandle sceneTex, bgfx::TextureHandle depthTex, const Camera& camera);
    void Bind(bgfx::UniformHandle s_motionBlurTex);

    bgfx::TextureHandle GetTexture() const 
    { 
        return m_texture; 
    }

private:
    void CreateBuffers(int width, int height);
    void DeleteBuffers();

    bgfx::TextureHandle m_texture = BGFX_INVALID_HANDLE;

    R_Shader m_shader;

    bgfx::UniformHandle m_sSceneTex = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sDepthTex = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uInvViewProj = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uPrevViewProj = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uBlurParams = BGFX_INVALID_HANDLE;

    int m_width = 0, m_height = 0;
};