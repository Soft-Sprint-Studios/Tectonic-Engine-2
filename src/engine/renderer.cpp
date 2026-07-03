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
#include "console.h"
#include "cvar.h"
#include "platform.h"
#include "physics.h"
#include "entities.h"
#include "cubemap.h"
#include <SDL3/SDL.h>
#include <glm/gtc/type_ptr.hpp>

CVar cl_showfps("cl_showfps", "0", "Draw the current frames per second at the top of the screen.", CVAR_SAVE);
CVar cl_showpos("cl_showpos", "0", "Draw current position and angles at the top of the screen.", CVAR_SAVE);
CVar cl_crosshair("cl_crosshair", "1", "Toggle the crosshair overlay.", CVAR_SAVE);
CVar cl_fov("cl_fov", "75.0", "Vertical field of view.", CVAR_SAVE);
CVar r_skybox("r_skybox", "1", "Enable skybox rendering.", CVAR_SAVE);
CVar r_particles("r_particles", "1", "Enable particle system rendering.", CVAR_SAVE);
CVar r_water("r_water", "1", "Enable water rendering.", CVAR_SAVE);
CVar r_sprites("r_sprites", "1", "Enable sprite rendering.", CVAR_SAVE);
CVar r_debug_gbuffer("r_debug_gbuffer", "0", "Enable G-Buffer diagnostic overlay.", CVAR_SAVE);
CVar r_lightmap_bicubic("r_lightmap_bicubic", "1", "Enable high-quality bicubic filtering for lightmaps.", CVAR_SAVE);

CVar mat_parallax("mat_parallax", "1", "Enable Parallax Mapping.", CVAR_SAVE);
CVar mat_parallax_min_steps("mat_parallax_min_steps", "8", "Minimum ray steps for Parallax Mapping.", CVAR_SAVE);
CVar mat_parallax_max_steps("mat_parallax_max_steps", "32", "Maximum ray steps for Parallax Mapping.", CVAR_SAVE);
CVar mat_parallax_refine("mat_parallax_refine", "8", "Number of binary search refinement steps for Parallax Mapping.", CVAR_SAVE);

Renderer::Renderer() : m_windowRef(nullptr)
{
}

Renderer::~Renderer()
{
    Shutdown();
}

bool Renderer::Init(Window& window)
{
    m_windowRef = &window;

    bgfx::Init init;
    init.type = bgfx::RendererType::Vulkan;

    int w, h;
    SDL_GetWindowSize(m_windowRef->Get(), &w, &h);

    init.resolution.width = (uint32_t)w;
    init.resolution.height = (uint32_t)h;
    init.resolution.reset = CVar::GetInt("r_vsync", 1) > 0 ? BGFX_RESET_VSYNC : BGFX_RESET_NONE;

    bgfx::PlatformData pd;
#if defined(PLATFORM_WINDOWS)
    pd.nwh = (void*)SDL_GetPointerProperty(SDL_GetWindowProperties(m_windowRef->Get()), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#endif
    init.platformData = pd;

    if (!bgfx::init(init))
    {
        Console::Error("BGFX: Failed to initialize Vulkan backend.");
        return false;
    }

    bgfx::setViewClear(m_mainView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x0000FFFF, 1.0f, 0);
    bgfx::setViewRect(m_mainView, 0, 0, (uint16_t)w, (uint16_t)h);

    m_uiRenderer = std::make_unique<R_UI>();
    m_uiRenderer->Init(m_windowRef);
    m_gbuffer = std::make_unique<R_GBuffer>();
    m_gbuffer->Init(w, h);
    m_bspRenderer = std::make_unique<R_BSP>();
    m_decalRenderer = std::make_unique<R_Decals>();
    m_decalRenderer->Init();
    m_lightRenderer = std::make_unique<R_Lights>();
    m_lightRenderer->Init();

    if (!m_gbufferShader.Load("shaders/gbuffer.vert", "shaders/gbuffer.frag"))
    {
        Console::Error("BGFX: Failed to load G-Buffer Shader binaries!");
    }

    m_sDepth = bgfx::createUniform("s_gDepth", bgfx::UniformType::Sampler);
    m_sNormal = bgfx::createUniform("s_gNormal", bgfx::UniformType::Sampler);
    m_sAlbedo = bgfx::createUniform("s_gAlbedo", bgfx::UniformType::Sampler);
    m_sMRAO = bgfx::createUniform("s_gMRAO", bgfx::UniformType::Sampler);
    m_sCubemap = bgfx::createUniform("s_cubemap", bgfx::UniformType::Sampler);
    m_sLightmap = bgfx::createUniform("s_lightmap", bgfx::UniformType::Sampler);
    m_sGLightmapUV = bgfx::createUniform("s_gLightmapUV", bgfx::UniformType::Sampler);
    m_uViewPosLocal = bgfx::createUniform("u_viewPos", bgfx::UniformType::Vec4);
    m_uCubemapParams = bgfx::createUniform("u_cubemapParams", bgfx::UniformType::Vec4);
    m_uCubemapOrigin = bgfx::createUniform("u_cubemapOrigin", bgfx::UniformType::Vec4);
    m_uCubemapMins = bgfx::createUniform("u_cubemapMins", bgfx::UniformType::Vec4);
    m_uCubemapMaxs = bgfx::createUniform("u_cubemapMaxs", bgfx::UniformType::Vec4);
    m_sCsmArray = bgfx::createUniform("u_csmArray", bgfx::UniformType::Sampler);
    m_sSpotShadowMaps = bgfx::createUniform("u_spotShadowMaps", bgfx::UniformType::Sampler);
    m_sPointShadowMaps = bgfx::createUniform("u_pointShadowMaps", bgfx::UniformType::Sampler);

    uint8_t whitePixel[] = { 255, 255, 255, 255 };
    m_whiteTexture = bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_NONE, bgfx::copy(whitePixel, 4));

    uint8_t dummyCubeData[6 * 4] = { 0 };
    m_dummyCubemap = bgfx::createTextureCube(1, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_NONE, bgfx::copy(dummyCubeData, sizeof(dummyCubeData)));

    m_quadLayout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

    m_resolveShader.Load("shaders/resolve.vert", "shaders/resolve.frag");

    return true;
}

bool Renderer::LoadMap(const std::string& path)
{
    BSP::MapData map = BSP::Load(path);
    if (!map.loaded)
    {
        return false;
    }

    m_bspRenderer->Init(map);

    Physics::AddBSPCollision(map.collision.vertices, map.collision.indices);

    for (const auto& entData : map.entities)
    {
        EntityManager::SpawnEntity(entData.className, entData);
    }

    return true;
}

void Renderer::Render(Camera& camera)
{
    bgfx::touch(m_mainView);

    m_lightRenderer->RenderShadowMaps(camera, this);

    RenderWorld(camera);

    m_uiRenderer->Render();

    bgfx::frame();
}

void Renderer::RenderWorld(Camera& camera, uint32_t cubemapToExclude, bool drawWater)
{
    int w, h;
    SDL_GetWindowSize(m_windowRef->Get(), &w, &h);

    GeometryPass(camera, w, h, drawWater);
    LightingPass(camera, cubemapToExclude, 0, w, h, w, h);
}

void Renderer::GeometryPass(Camera& camera, int renderW, int renderH, bool drawWater)
{
    bgfx::ViewId geoView = RenderView::GBuffer;

    bgfx::setViewClear(geoView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH | BGFX_CLEAR_STENCIL, 0x1A1A1AFF, 1.0f, 0);
    bgfx::setViewRect(geoView, 0, 0, (uint16_t)renderW, (uint16_t)renderH);
    bgfx::setViewFrameBuffer(geoView, m_gbuffer->GetFBO());

    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 proj = camera.GetProjectionMatrix();
    bgfx::setViewTransform(geoView, glm::value_ptr(view), glm::value_ptr(proj));

    Frustum frustum = camera.GetFrustum();

    m_bspRenderer->Draw(m_gbufferShader, geoView, frustum, camera.position);
    m_decalRenderer->Draw(geoView, camera, frustum, Decals::GetActiveDecals());
}

void Renderer::DrawSceneDepth(bgfx::ViewId viewId, R_Shader& shader, const struct Frustum& frustum)
{
    m_bspRenderer->Draw(shader, viewId, frustum, glm::vec3(0.0f), true);
}

void Renderer::OnWindowResize(int w, int h)
{
    bgfx::reset((uint32_t)w, (uint32_t)h, CVar::GetInt("r_vsync", 1) > 0 ? BGFX_RESET_VSYNC : BGFX_RESET_NONE);
    bgfx::setViewRect(m_mainView, 0, 0, (uint16_t)w, (uint16_t)h);
    if (m_gbuffer)
    {
        m_gbuffer->Rescale(w, h);
    }
    if (m_uiRenderer)
    {
        m_uiRenderer->OnWindowResize(w, h);
    }
}

void Renderer::Shutdown()
{
    if (m_gbuffer)
    {
        m_gbuffer->Shutdown();
        m_gbuffer.reset();
    }

    if (m_bspRenderer)
    {
        m_bspRenderer->Shutdown();
        m_bspRenderer.reset();
    }

    if (m_decalRenderer)
    {
        m_decalRenderer->Shutdown();
        m_decalRenderer.reset();
    }

    if (m_lightRenderer)
    {
        m_lightRenderer->Shutdown();
        m_lightRenderer.reset();
    }

    if (bgfx::isValid(m_sDepth))
    {
        bgfx::destroy(m_sDepth);
        bgfx::destroy(m_sNormal);
        bgfx::destroy(m_sAlbedo);
        bgfx::destroy(m_sMRAO);
        bgfx::destroy(m_sCubemap);
        bgfx::destroy(m_sLightmap);
        bgfx::destroy(m_sGLightmapUV);
        bgfx::destroy(m_uViewPosLocal);
        bgfx::destroy(m_uCubemapParams);
        bgfx::destroy(m_uCubemapOrigin);
        bgfx::destroy(m_uCubemapMins);
        bgfx::destroy(m_uCubemapMaxs);
        bgfx::destroy(m_dummyCubemap);
        bgfx::destroy(m_whiteTexture);
        bgfx::destroy(m_sCsmArray);
        bgfx::destroy(m_sSpotShadowMaps);
        bgfx::destroy(m_sPointShadowMaps);

        m_sDepth = BGFX_INVALID_HANDLE;
    }

    if (m_uiRenderer)
    {
        m_uiRenderer->Shutdown();
        m_uiRenderer.reset();
    }

    bgfx::shutdown();
}

void Renderer::LightingPass(Camera& camera, uint32_t cubemapToExclude, int targetFBO, int renderW, int renderH, int w, int h)
{
    bgfx::ViewId lightingView = RenderView::Resolve;

    bgfx::setViewClear(lightingView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000FF, 1.0f, 0);
    bgfx::setViewRect(lightingView, 0, 0, (uint16_t)w, (uint16_t)h);
    bgfx::setViewFrameBuffer(lightingView, BGFX_INVALID_HANDLE);

    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 proj = camera.GetProjectionMatrix();
    bgfx::setViewTransform(lightingView, glm::value_ptr(view), glm::value_ptr(proj));

    const Cubemap::Probe* closest = Cubemap::FindClosest(camera.position);
    bool useCubemap = (closest != nullptr);

    float cParams[4] = { useCubemap ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f };
    float cOrigin[4] = { 0.0f };
    float cMins[4] = { 0.0f };
    float cMaxs[4] = { 0.0f };

    if (useCubemap)
    {
        cOrigin[0] = closest->origin.x; cOrigin[1] = closest->origin.y; cOrigin[2] = closest->origin.z;
        cMins[0] = closest->mins.x; cMins[1] = closest->mins.y; cMins[2] = closest->mins.z;
        cMaxs[0] = closest->maxs.x; cMaxs[1] = closest->maxs.y; cMaxs[2] = closest->maxs.z;

        bgfx::TextureHandle cubemapTex = { (uint16_t)closest->id };
        bgfx::setTexture(4, m_sCubemap, bgfx::isValid(cubemapTex) ? cubemapTex : m_dummyCubemap);
    }
    else
    {
        bgfx::setTexture(4, m_sCubemap, m_dummyCubemap);
    }

    bgfx::setUniform(m_uCubemapParams, cParams);
    bgfx::setUniform(m_uCubemapOrigin, cOrigin);
    bgfx::setUniform(m_uCubemapMins, cMins);
    bgfx::setUniform(m_uCubemapMaxs, cMaxs);

    float vp[4] = { camera.position.x, camera.position.y, camera.position.z, 0.0f };
    bgfx::setUniform(m_uViewPosLocal, vp);

    m_lightRenderer->Bind(m_resolveShader);
    bgfx::setTexture(13, m_sCsmArray, m_lightRenderer->GetCascadeShadowTexture());
    bgfx::setTexture(14, m_sSpotShadowMaps, m_lightRenderer->GetSpotShadowTexture());
    bgfx::setTexture(15, m_sPointShadowMaps, m_lightRenderer->GetPointShadowTexture());

    bgfx::setTexture(0, m_sDepth, m_gbuffer->GetDepthTex());
    bgfx::setTexture(1, m_sNormal, m_gbuffer->GetNormalTex());
    bgfx::setTexture(2, m_sAlbedo, m_gbuffer->GetAlbedoTex());
    bgfx::setTexture(3, m_sMRAO, m_gbuffer->GetMRAOTex());

    bgfx::TextureHandle lmTex = m_bspRenderer->GetLightmapTexture();
    bgfx::setTexture(5, m_sLightmap, bgfx::isValid(lmTex) ? lmTex : m_whiteTexture);
    bgfx::setTexture(6, m_sGLightmapUV, m_gbuffer->GetLightmapUVTex());

    bgfx::TransientVertexBuffer tvb;
    bgfx::allocTransientVertexBuffer(&tvb, 6, m_quadLayout);

    struct QuadVertex
    {
        float x, y, z;
        float u, v;
    };

    QuadVertex* v = (QuadVertex*)tvb.data;
    v[0] = { -1.0f,  1.0f, 0.0f, 0.0f, 0.0f };
    v[1] = { -1.0f, -1.0f, 0.0f, 0.0f, 1.0f };
    v[2] = { 1.0f, -1.0f, 0.0f, 1.0f, 1.0f };
    v[3] = { -1.0f,  1.0f, 0.0f, 0.0f, 0.0f };
    v[4] = { 1.0f, -1.0f, 0.0f, 1.0f, 1.0f };
    v[5] = { 1.0f,  1.0f, 0.0f, 1.0f, 0.0f };

    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
    bgfx::submit(lightingView, m_resolveShader.GetProgram());
}

void Renderer::ForwardPass(Camera& camera, int targetFBO, int renderW, int renderH)
{
}