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
#include "dynamic_light.h"
#include "r_shader.h"
#include "camera.h"
#include "r_cascade.h"
#include <bgfx/bgfx.h>
#include <memory>

class R_BSP;
class R_Models;

class R_Lights
{
public:
    R_Lights();
    ~R_Lights();

    bool Init();
    void RenderShadowMaps(Camera& camera, class Renderer* renderer);
    void Bind(const R_Shader& shader);
    void Shutdown();

    bgfx::TextureHandle GetSpotShadowTexture() const 
    { 
        return m_SpotShadow; 
    }

    bgfx::TextureHandle GetPointShadowTexture() const 
    { 
        return m_PointShadow; 
    }

    bgfx::TextureHandle GetCascadeShadowTexture() const 
    { 
        return m_cascade->GetTexArray(); 
    }

private:
    void SetupShadowMap(std::shared_ptr<DynamicLight> light);

    struct GPULight
    {
        float posRadius[4];
        float colorVol[4];
        float dirInner[4];
        float shadowData[4];
        float lightSpace[16];
        float shadowLayer;
        float pad[3];
    };

    bgfx::DynamicIndexBufferHandle m_pointLightSSBO = BGFX_INVALID_HANDLE;
    bgfx::DynamicIndexBufferHandle m_spotLightSSBO = BGFX_INVALID_HANDLE;

    int m_nextSpotLayer = 0;
    int m_nextPointLayer = 0;

    R_Shader m_shadowSpotShader;
    R_Shader m_shadowCascadeShader;
    R_Shader m_shadowPointShader;

    bgfx::TextureHandle m_SpotShadow = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_PointShadow = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_shadowDepthTex = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_PointDepth = BGFX_INVALID_HANDLE;

    bgfx::FrameBufferHandle m_spotFB[8];
    bgfx::FrameBufferHandle m_pointFB[8][6];

    bgfx::UniformHandle m_sCsmArray = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sSpotShadowMaps = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sPointShadowMaps = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uLightParams = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uShadowParams = BGFX_INVALID_HANDLE;

    std::unique_ptr<R_Cascade> m_cascade;
};  