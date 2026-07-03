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
#include "window.h"
#include "camera.h"
#include "r_shader.h"
#include "r_texture.h"
#include "r_ui.h"
#include "r_gbuffer.h"
#include "r_bsp.h"
#include "r_models.h"
#include "r_decals.h"
#include "r_lights.h"
#include "r_postprocess.h"
#include "r_water.h"
#include <bgfx/bgfx.h>
#include <memory>

namespace RenderView
{
    enum Enum : bgfx::ViewId
    {
        ReflectionGBuffer = 0,
        WaterReflection = 1,
        GBuffer = 2,
        Resolve = 3,
        Forward = 4,
        PostProcess = 5,
        UI = 6,
        CSM_0 = 7,
        CSM_1 = 8,
        CSM_2 = 9,
        CSM_3 = 10,
        ShadowBase = 11
    };
}

class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool Init(Window& window);
    bool LoadMap(const std::string& path);
    void Shutdown();
    void Render(Camera& camera);
    void RenderWorld(Camera& camera, uint32_t cubemapToExclude = 0, bool drawWater = true, bgfx::ViewId geoView = RenderView::GBuffer, bgfx::ViewId lightingView = RenderView::Resolve, bgfx::FrameBufferHandle targetFB = BGFX_INVALID_HANDLE);
    void DrawSceneDepth(bgfx::ViewId viewId, R_Shader& shader, const struct Frustum& frustum);
    void OnWindowResize(int w, int h);

    R_UI* GetUI() const
    {
        return m_uiRenderer.get();
    }

private:
    void GeometryPass(Camera& camera, int renderW, int renderH, bool drawWater, bgfx::ViewId geoView = RenderView::GBuffer);
    void LightingPass(Camera& camera, uint32_t cubemapToExclude, int targetFBO, int renderW, int renderH, int w, int h, bgfx::ViewId lightingView = RenderView::Resolve, bgfx::FrameBufferHandle targetFB = BGFX_INVALID_HANDLE);
    void ForwardPass(Camera& camera, int targetFBO, int renderW, int renderH);

    Window* m_windowRef;
    R_Shader m_gbufferShader;
    R_Shader m_resolveShader;
    bgfx::ViewId m_mainView = 0;

    std::unique_ptr<R_UI> m_uiRenderer;
    std::unique_ptr<R_GBuffer> m_gbuffer;
    std::unique_ptr<R_BSP> m_bspRenderer;
    std::unique_ptr<R_Models> m_modelRenderer;
    std::unique_ptr<R_Decals> m_decalRenderer;
    std::unique_ptr<R_Lights> m_lightRenderer;
    std::unique_ptr<R_Water> m_waterRenderer;
    std::unique_ptr<R_PostProcess> m_postProcess;

    bgfx::UniformHandle m_sDepth = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sNormal = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sAlbedo = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sMRAO = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sCubemap = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sLightmap = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sGLightmapUV = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uViewPosLocal = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uCubemapParams = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uCubemapOrigin = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uCubemapMins = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uCubemapMaxs = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_dummyCubemap = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_whiteTexture = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sCsmArray = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sSpotShadowMaps = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sPointShadowMaps = BGFX_INVALID_HANDLE;
    bgfx::VertexLayout m_quadLayout;
};