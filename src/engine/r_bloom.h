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
#include <bgfx/bgfx.h>
#include <vector>
#include <glm/glm.hpp>

class R_Bloom
{
public:
    R_Bloom();
    ~R_Bloom();

    bool Init(int width, int height);
    void Shutdown();
    void Rescale(int width, int height);
    void Render(bgfx::ViewId viewId, bgfx::TextureHandle sourceTexture, int screenW, int screenH);
    void Bind(bgfx::UniformHandle s_bloomTex);

    bgfx::TextureHandle GetBloomTexture() const;

private:
    struct Mip
    {
        glm::ivec2 size;
        bgfx::TextureHandle downsampleTex = BGFX_INVALID_HANDLE;
        bgfx::TextureHandle upsampleTex = BGFX_INVALID_HANDLE; 
    };

    void CreateChain(int width, int height);
    void DeleteChain();

    R_Shader m_downsampleShader;
    R_Shader m_upsampleShader;

    std::vector<Mip> m_mipChain;

    bgfx::UniformHandle m_sSource = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sCurrentMip = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uBloomParams = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uBloomParams2 = BGFX_INVALID_HANDLE;
};