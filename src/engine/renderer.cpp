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

    if (!m_gbufferShader.Load("shaders/gbuffer.vert", "shaders/gbuffer.frag"))
    {
        Console::Error("BGFX: Failed to load G-Buffer Shader binaries!");
    }

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

    RenderWorld(camera);

    m_uiRenderer->Render();

    bgfx::frame();
}

void Renderer::RenderWorld(Camera& camera, uint32_t cubemapToExclude, bool drawWater)
{
    int w, h;
    SDL_GetWindowSize(m_windowRef->Get(), &w, &h);

    GeometryPass(camera, w, h, drawWater);
}

void Renderer::GeometryPass(Camera& camera, int renderW, int renderH, bool drawWater)
{
    bgfx::ViewId geoView = 0;

    bgfx::setViewClear(geoView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH | BGFX_CLEAR_STENCIL, 0x1A1A1AFF, 1.0f, 0);
    bgfx::setViewRect(geoView, 0, 0, (uint16_t)renderW, (uint16_t)renderH);
    bgfx::setViewFrameBuffer(geoView, BGFX_INVALID_HANDLE);

    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 proj = camera.GetProjectionMatrix();
    bgfx::setViewTransform(geoView, glm::value_ptr(view), glm::value_ptr(proj));

    Frustum frustum = camera.GetFrustum();

    m_bspRenderer->Draw(m_gbufferShader, geoView, frustum, camera.position);
}

void Renderer::DrawSceneDepth(R_Shader& shader, const struct Frustum& frustum)
{
}

void Renderer::OnWindowResize(int w, int h)
{
    bgfx::reset((uint32_t)w, (uint32_t)h, CVar::GetInt("r_vsync", 1) > 0 ? BGFX_RESET_VSYNC : BGFX_RESET_NONE);
    bgfx::setViewRect(m_mainView, 0, 0, (uint16_t)w, (uint16_t)h);
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

    if (m_uiRenderer)
    {
        m_uiRenderer->Shutdown();
        m_uiRenderer.reset();
    }

    bgfx::shutdown();
}

void Renderer::LightingPass(Camera& camera, uint32_t cubemapToExclude, int targetFBO, int renderW, int renderH, int w, int h)
{
}

void Renderer::ForwardPass(Camera& camera, int targetFBO, int renderW, int renderH)
{
}