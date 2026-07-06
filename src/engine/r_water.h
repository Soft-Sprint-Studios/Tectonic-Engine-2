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
#include "r_texture.h"
#include "camera.h"
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <bgfx/bgfx.h>

struct WaterSurface
{
    uint32_t start;
    uint32_t count;
    float height;
    std::string textureName;
};

class R_Water
{
public:
    void Init(int width, int height);
    void AddSurface(const WaterSurface& surface);
    void ClearSurfaces();
    void RenderReflection(class Renderer* renderer, const Camera& mainCam);
    void Draw(bgfx::ViewId viewId, const Camera& camera, bgfx::VertexBufferHandle bspVbo, bgfx::TextureHandle lightmap);
    void Rescale(int width, int height);
    void Shutdown();

private:
    R_Shader m_shader;
    std::vector<WaterSurface> m_surfaces;
    std::vector<int32_t> m_starts;
    std::vector<uint32_t> m_counts;
    bgfx::FrameBufferHandle m_reflectFBO = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_reflectTex = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_reflectDepthTex = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_whiteTexture = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_defaultNormal = BGFX_INVALID_HANDLE;

    bgfx::UniformHandle m_sReflectionTexture = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sDudvMap = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sNormalMap = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sFlowMap = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sLightmap = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uViewPos = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uTime = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uFlowParams = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uReflectViewProj = BGFX_INVALID_HANDLE;

    glm::mat4 m_reflectView;
    glm::mat4 m_reflectProj;
    int m_width, m_height;
};