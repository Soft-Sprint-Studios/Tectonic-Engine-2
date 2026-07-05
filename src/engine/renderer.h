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
#include "r_sky.h"
#include "r_glass.h"
#include "r_sprites.h"
#include "r_beams.h"
#include "r_cables.h"
#include "r_particles.h"
#include "r_overlay.h"
#include "r_interior_parallax.h"
#include "r_video.h"
#include "r_monitors.h"
#include <bgfx/bgfx.h>
#include <memory>

namespace RenderView
{
    enum Enum : bgfx::ViewId
    {
        CSM_0 = 0,
        CSM_1 = 1,
        CSM_2 = 2,
        CSM_3 = 3,
        ShadowBase = 4,
        ReflectionGBuffer = 70,
        WaterReflection = 71,
        MonitorGBuffer = 72,
        MonitorResolve = 73,
        GBuffer = 74,
        Resolve = 75,
        Forward = 76,
        GlassBlit = 77,
        TransparentDraw = 78,
        AutoExposure = 79,
        Bloom = 80,
        SSAO = 81,
        SSAO_Blur = 82,
        SSR = 83,
        SSR_Blur = 84,
        Volumetrics = 85,
        Volumetrics_Blur = 86,
        MotionBlur = 87,
        CAS = 88,
        PostProcess = 89,
        GBufferDebug0 = 90,
        GBufferDebug1 = 91,
        GBufferDebug2 = 92,
        GBufferDebug3 = 93,
        GBufferDebug4 = 94,
        GBufferDebug5 = 95,
        GBufferDebug6 = 96,
        GBufferDebug7 = 97,
        UI = 98
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
    void ForwardPass(Camera& camera, bgfx::ViewId viewId, int renderW, int renderH);

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
    std::unique_ptr<R_Sky> m_skyRenderer;
    std::unique_ptr<R_Glass> m_glassRenderer;
    std::unique_ptr<R_Sprites> m_spriteRenderer;
    std::unique_ptr<R_Beams> m_beamRenderer;
    std::unique_ptr<R_Cables> m_cableRenderer;
    std::unique_ptr<R_Particles> m_particleRenderer;
    std::unique_ptr<R_Overlay> m_overlayRenderer;
    std::unique_ptr<R_InteriorParallax> m_interiorRenderer;
    std::unique_ptr<R_Video> m_videoRenderer;
    std::unique_ptr<R_Monitors> m_monitorRenderer;
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