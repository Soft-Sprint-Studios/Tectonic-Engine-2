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
#include "bsploader.h"
#include "physics.h"
#include "entities.h"
#include "cubemap.h"
#include "fade.h"
#include <glm/glm.hpp>
#include <ctime>
#include "concmd.h"
#include <cstring>

CVar cl_showfps("cl_showfps", "0", "Draw the current frames per second at the top of the screen.", CVAR_SAVE);
CVar cl_showpos("cl_showpos", "0", "Draw current position and angles at the top of the screen.", CVAR_SAVE);
CVar cl_crosshair("cl_crosshair", "1", "Toggle the crosshair overlay.", CVAR_SAVE);
CVar cl_fov("cl_fov", "75.0", "Vertical field of view.", CVAR_SAVE);
CVar r_skybox("r_skybox", "1", "Enable skybox rendering.", CVAR_SAVE);
CVar r_particles("r_particles", "1", "Enable particle system rendering.", CVAR_SAVE);
CVar r_water("r_water", "1", "Enable water rendering.", CVAR_SAVE);
CVar r_sprites("r_sprites", "1", "Enable sprite rendering.", CVAR_SAVE);
CVar r_wireframe("r_wireframe", "0", "Render the scene in wireframe mode.", CVAR_NONE);
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

    m_gbufferShader.Load("shaders/gbuffer.vert", "shaders/gbuffer.frag");
    m_resolveShader.Load("shaders/resolve.vert", "shaders/resolve.frag");

    // Global GL State
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    m_postProcess = std::make_unique<R_PostProcess>();
    int ww, wh;
    SDL_GetWindowSize(m_windowRef->Get(), &ww, &wh);

    m_gbuffer = std::make_unique<R_GBuffer>();
    m_gbuffer->Init(ww, wh);

    float quadVertices[] =
    {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glNamedBufferData(m_quadVBO, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    m_postProcess->Init(ww, wh);

    m_bspRenderer = std::make_unique<R_BSP>();
    m_modelRenderer = std::make_unique<R_Models>();
    m_skyRenderer = std::make_unique<R_Sky>();
    m_particleRenderer = std::make_unique<R_Particles>();
    m_lightRenderer = std::make_unique<R_Lights>();
    m_spriteRenderer = std::make_unique<R_Sprites>();
    m_beamRenderer = std::make_unique<R_Beams>();
    m_cableRenderer = std::make_unique<R_Cables>();
    m_videoRenderer = std::make_unique<R_Video>();
    m_monitorRenderer = std::make_unique<R_Monitors>();
    m_overlayRenderer = std::make_unique<R_Overlay>();
    m_interiorRenderer = std::make_unique<R_InteriorParallax>();
    m_glassRenderer = std::make_unique<R_Glass>();
    m_waterRenderer = std::make_unique<R_Waters>();
    m_uiRenderer = std::make_unique<R_UI>();
    m_decalRenderer = std::make_unique<R_Decals>();

    m_glassRenderer->Init(ww, wh);
    m_waterRenderer->Init(ww, wh);
    m_uiRenderer->Init(m_windowRef);
    m_particleRenderer->Init();
    m_lightRenderer->Init();
    m_spriteRenderer->Init();
    m_beamRenderer->Init();
    m_cableRenderer->Init();
    m_videoRenderer->Init();
    m_monitorRenderer->Init();
    m_overlayRenderer->Init();
    m_interiorRenderer->Init();
    m_decalRenderer->Init();

    return true;
}

bool Renderer::LoadMap(const std::string& path)
{
    BSP::MapData map = BSP::Load(path);
    if (!map.loaded)
    {
        return false;
    }

    Cubemap::LoadForMap(path.substr(5, path.find_last_of('.') - 5));

    m_bspRenderer->Init(map);
    m_modelRenderer->Init(map);
    m_skyRenderer->Init(map.skyName);

    m_waterRenderer->ClearSurfaces();
    for (const auto& s : map.waterSurfaces)
    {
        m_waterRenderer->AddSurface({ s.start, s.count, s.height, s.textureName });
    }

    Physics::AddBSPCollision(map.collision.vertices, map.collision.indices);

    for (const auto& entData : map.entities)
    {
        EntityManager::SpawnEntity(entData.className, entData);
    }

    return true;
}

void Renderer::DrawSceneDepth(R_Shader& shader, const Frustum& frustum)
{
    m_bspRenderer->Draw(shader, frustum, true);
    m_modelRenderer->Draw(shader, frustum, true);
}

void Renderer::RenderWorld(Camera& camera, GLuint cubemapToExclude, bool drawWater)
{
    GLint targetFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &targetFBO);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int renderW = viewport[2];
    int renderH = viewport[3];

    int w, h;
    SDL_GetWindowSize(m_windowRef->Get(), &w, &h);

    GeometryPass(camera, renderW, renderH, drawWater);
    LightingPass(camera, cubemapToExclude, targetFBO, renderW, renderH, w, h);
    if (targetFBO == m_postProcess->GetActiveFBO())
    {
        glNamedFramebufferTexture(targetFBO, GL_DEPTH_ATTACHMENT, m_gbuffer->GetDepthTex(), 0);
    }
    else
    {
        DepthBlit(targetFBO, renderW, renderH);
    }
    ForwardPass(camera, targetFBO, renderW, renderH);
}

void Renderer::GeometryPass(Camera& camera, int renderW, int renderH, bool drawWater)
{
    m_gbuffer->Bind();
    glViewport(0, 0, renderW, renderH);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_gbufferShader.Bind();
    m_gbufferShader.SetMat4("u_projection", camera.GetProjectionMatrix());
    m_gbufferShader.SetMat4("u_prevViewProj", camera.GetPrevViewProj());
    m_gbufferShader.SetMat4("u_view", camera.GetViewMatrix());
    m_gbufferShader.SetMat4("u_model", glm::mat4(1.0f));
    m_gbufferShader.SetVec3("u_viewPos", camera.position);
    m_gbufferShader.SetInt("u_mat_parallax", mat_parallax.GetInt());
    m_gbufferShader.SetFloat("u_pomMinSteps", mat_parallax_min_steps.GetFloat());
    m_gbufferShader.SetFloat("u_pomMaxSteps", mat_parallax_max_steps.GetFloat());
    m_gbufferShader.SetInt("u_pomRefineSteps", mat_parallax_refine.GetInt());

    Frustum frustum = camera.GetFrustum();

    m_bspRenderer->Draw(m_gbufferShader, frustum);
    m_modelRenderer->Draw(m_gbufferShader, frustum);
    m_decalRenderer->Draw(camera, frustum, Decals::GetActiveDecals());

    if (drawWater && r_water.GetInt() > 0)
        m_waterRenderer->Draw(camera, m_bspRenderer->GetVAO(), m_bspRenderer->GetLightmapTexture());

    m_gbuffer->Unbind();
}

void Renderer::LightingPass(Camera& camera, GLuint cubemapToExclude, GLint targetFBO, int renderW, int renderH, int w, int h)
{
    glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
    glViewport(0, 0, renderW, renderH);

    m_resolveShader.Bind();
    m_resolveShader.SetMat4("u_view", camera.GetViewMatrix());
    m_resolveShader.SetMat4("u_projection", camera.GetProjectionMatrix());
    m_resolveShader.SetVec3("u_viewPos", camera.position);

    m_resolveShader.SetMat4("u_invProjection", glm::inverse(camera.GetProjectionMatrix()));
    m_resolveShader.SetMat4("u_invView", glm::inverse(camera.GetViewMatrix()));

    m_resolveShader.SetInt("r_lightmap_bicubic", r_lightmap_bicubic.GetInt());

    glBindTextureUnit(0, m_gbuffer->GetDepthTex());
    glBindTextureUnit(1, m_gbuffer->GetNormalTex());
    glBindTextureUnit(2, m_gbuffer->GetAlbedoTex());
    glBindTextureUnit(3, m_gbuffer->GetMRAOTex());
    glBindTextureUnit(6, m_gbuffer->GetLightmapUVTex());
    glBindTextureUnit(5, m_bspRenderer->GetLightmapTexture());

    m_resolveShader.SetInt("u_useCubemap", 0);
    glBindTextureUnit(4, 0);

    const Cubemap::CubemapProbe* probe = Cubemap::FindClosest(camera.position);
    if (probe && probe->textureID != 0 && probe->textureID != cubemapToExclude)
    {
        m_resolveShader.SetInt("u_useCubemap", 1);
        m_resolveShader.SetVec3("u_cubemapOrigin", probe->origin);
        m_resolveShader.SetVec3("u_cubemapMins", probe->mins);
        m_resolveShader.SetVec3("u_cubemapMaxs", probe->maxs);
        glBindTextureUnit(4, probe->textureID);
    }

    m_lightRenderer->Bind(m_resolveShader);

    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Renderer::DepthBlit(GLint targetFBO, int renderW, int renderH)
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_gbuffer->GetFBO());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, targetFBO);
    glBlitFramebuffer(0, 0, renderW, renderH, 0, 0, renderW, renderH, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
}

void Renderer::ForwardPass(Camera& camera, GLint targetFBO, int renderW, int renderH)
{
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    if (r_skybox.GetInt() > 0)
        m_skyRenderer->Draw(camera);

    if (r_particles.GetInt() > 0)
        m_particleRenderer->Draw(camera, m_gbuffer->GetDepthTex());

    if (r_sprites.GetInt() > 0)
        m_spriteRenderer->Draw(camera, Sprites::GetActiveSprites());

    m_beamRenderer->Draw(camera, Beams::GetActiveBeams());
    m_cableRenderer->Draw(camera, Cables::GetActiveCables());
    m_videoRenderer->Draw(camera, m_bspRenderer.get());
    m_monitorRenderer->Draw(camera, m_bspRenderer.get());
    m_interiorRenderer->Draw(camera, m_bspRenderer.get());

    glDepthMask(GL_TRUE);
}

void Renderer::Render(Camera& camera)
{
    m_lightRenderer->RenderShadowMaps(camera, this);
    m_monitorRenderer->RenderTextures(this);

    int w, h;
    SDL_GetWindowSize(m_windowRef->Get(), &w, &h);

    camera.SetAspectRatio((float)w / (float)h);

    glPolygonMode(GL_FRONT_AND_BACK, r_wireframe.GetInt() > 0 ? GL_LINE : GL_FILL);

    if (r_water.GetInt() == 1)
        m_waterRenderer->RenderReflection(this, camera);

    m_postProcess->Begin();
    glViewport(0, 0, w, h);

    RenderWorld(camera, 0);

    m_glassRenderer->CaptureScreen(m_postProcess->GetActiveFBO(), w, h);
    m_glassRenderer->Draw(camera, m_bspRenderer.get());

    m_postProcess->End();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Draw postprocessing
    glDisable(GL_DEPTH_TEST);
    m_postProcess->Draw(camera, m_lightRenderer.get(), m_gbuffer.get());
    m_overlayRenderer->Draw();

    if (r_debug_gbuffer.GetInt())
    {
        m_gbuffer->DrawDebug(w, h);

        int dw = w / 9;
        int dh = h / 8;

        m_uiRenderer->DrawText("DEPTH", 10.0f, (float)(dh + 20), { 0.0f, 1.0f, 0.0f, 1.0f });
        m_uiRenderer->DrawText("WORLD N", (float)(dw + 10), (float)(dh + 20), { 0.0f, 1.0f, 0.0f, 1.0f });
        m_uiRenderer->DrawText("TANGENT N", (float)(dw * 2 + 10), (float)(dh + 20), { 0.0f, 1.0f, 0.0f, 1.0f });
        m_uiRenderer->DrawText("ALBEDO", (float)(dw * 3 + 10), (float)(dh + 20), { 0.0f, 1.0f, 0.0f, 1.0f });
        m_uiRenderer->DrawText("METALLIC", (float)(dw * 4 + 10), (float)(dh + 20), { 0.0f, 1.0f, 0.0f, 1.0f });
        m_uiRenderer->DrawText("ROUGHNESS", (float)(dw * 5 + 10), (float)(dh + 20), { 0.0f, 1.0f, 0.0f, 1.0f });
        m_uiRenderer->DrawText("AO", (float)(dw * 6 + 10), (float)(dh + 20), { 0.0f, 1.0f, 0.0f, 1.0f });
        m_uiRenderer->DrawText("VELOCITY", (float)(dw * 7 + 10), (float)(dh + 20), { 0.0f, 1.0f, 0.0f, 1.0f });
        m_uiRenderer->DrawText("LM UV", (float)(dw * 8 + 10), (float)(dh + 20), { 0.0f, 1.0f, 0.0f, 1.0f });
    }

    glEnable(GL_DEPTH_TEST);

    // Handle env_fade
    glm::vec4 fade = Fade::GetCurrentFade();
    if (fade.a > 0.001f)
    {
        int w, h;
        SDL_GetWindowSize(m_windowRef->Get(), &w, &h);
        m_uiRenderer->DrawRect(0, 0, (float)w, (float)h, fade);
    }

    m_uiRenderer->Render();
    camera.UpdatePreviousState();
    m_windowRef->Swap();
}

void Renderer::OnWindowResize(int w, int h)
{
    if (m_gbuffer)
    {
        m_gbuffer->Rescale(w, h);
    }
    if (m_uiRenderer)
    {
        m_uiRenderer->OnWindowResize(w, h);
    }
    if (m_postProcess)
    {
        m_postProcess->Rescale(w, h);
    }
    if (m_glassRenderer)
    {
        m_glassRenderer->Rescale(w, h);
    }
}

void Renderer::Shutdown()
{
    if (m_gbuffer)
    {
        m_gbuffer->Shutdown();
        m_gbuffer.reset();
    }
    if (m_quadVAO != 0)
    {
        glDeleteVertexArrays(1, &m_quadVAO);
        m_quadVAO = 0;
    }
    if (m_quadVBO != 0)
    {
        glDeleteBuffers(1, &m_quadVBO);
        m_quadVBO = 0;
    }
    if (m_postProcess)
    {
        m_postProcess->Shutdown();
        m_postProcess.reset();
    }

    if (m_bspRenderer)
    {
        m_bspRenderer->Shutdown();
        m_bspRenderer.reset();
    }

    if (m_modelRenderer)
    {
        m_modelRenderer->Shutdown();
        m_modelRenderer.reset();
    }

    if (m_skyRenderer)
    {
        m_skyRenderer->Shutdown();
        m_skyRenderer.reset();
    }

    if (m_particleRenderer)
    {
        m_particleRenderer->Shutdown();
        m_particleRenderer.reset();
    }

    if (m_lightRenderer)
    {
        m_lightRenderer->Shutdown();
        m_lightRenderer.reset();
    }

    if (m_spriteRenderer)
    {
        m_spriteRenderer->Shutdown();
        m_spriteRenderer.reset();
    }

    if (m_beamRenderer)
    {
        m_beamRenderer->Shutdown();
        m_beamRenderer.reset();
    }

    if (m_cableRenderer)
    {
        m_cableRenderer->Shutdown();
        m_cableRenderer.reset();
    }

    if (m_videoRenderer)
    {
        m_videoRenderer->Shutdown();
        m_videoRenderer.reset();
    }

    if (m_monitorRenderer)
    {
        m_monitorRenderer->Shutdown();
        m_monitorRenderer.reset();
    }

    if (m_overlayRenderer)
    {
        m_overlayRenderer->Shutdown();
        m_overlayRenderer.reset();
    }

    if (m_glassRenderer)
    {
        m_glassRenderer->Shutdown();
        m_glassRenderer.reset();
    }

    if (m_interiorRenderer)
    {
        m_interiorRenderer->Shutdown();
        m_interiorRenderer.reset();
    }

    if (m_waterRenderer)
    {
        m_waterRenderer->Shutdown();
        m_waterRenderer.reset();
    }

    if (m_uiRenderer)
    {
        m_uiRenderer->Shutdown();
        m_uiRenderer.reset();
    }

    if (m_decalRenderer)
    {
        m_decalRenderer->Shutdown();
        m_decalRenderer.reset();
    }
}