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
#include "r_autoexposure.h"
#include "r_bloom.h"
#include "r_ssao.h"
#include "r_ssr.h"
#include "r_motionblur.h"
#include "r_volumetrics.h"
#include "r_cas.h"
#include "camera.h"
#include <bgfx/bgfx.h>
#include <memory>

class R_PostProcess
{
public:
    R_PostProcess();
    ~R_PostProcess();

    bool Init(int width, int height, bgfx::TextureHandle depthTexture);
    void Begin();
    void Draw(const Camera& camera, class R_Lights* lights, class R_GBuffer* gbuffer);
    void Rescale(int width, int height, bgfx::TextureHandle depthTexture);
    void Shutdown();

    bgfx::FrameBufferHandle GetFBO() const
    { 
        return m_fbo;
    }

    bgfx::TextureHandle GetTexture() const 
    {
        return m_texture; 
    }

private:
    void SetupBuffers(bgfx::TextureHandle depthTexture);

    bgfx::FrameBufferHandle m_fbo = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_texture = BGFX_INVALID_HANDLE;

    bgfx::VertexLayout m_quadLayout;
    R_Shader m_shader;

    bgfx::UniformHandle m_sSceneTexture = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sDepthTexture = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sBloomTexture = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sSsaoTexture = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sSsrTexture = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sMotionBlurTexture = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sVolumetricTexture = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uParams = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uBloomParams = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uSsaoParams = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uSsrParams = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uMotionBlurParams = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uVolumetricParams = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uColorParams = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uFogColor = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uFogParams = BGFX_INVALID_HANDLE;

    int m_width = 0;
    int m_height = 0;

    // Postprocess subrenderers
    std::unique_ptr<R_AutoExposure> m_autoExposure;
    std::unique_ptr<R_Bloom> m_bloom;
    std::unique_ptr<R_SSAO> m_ssao;
    std::unique_ptr<R_SSR> m_ssr;
    std::unique_ptr<R_MotionBlur> m_motionBlur;
    std::unique_ptr<R_Volumetrics> m_volumetrics;
    std::unique_ptr<R_CAS> m_cas;
};