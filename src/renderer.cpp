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
#include <glm/glm.hpp>

CVar r_vsync("r_vsync", "1", CVAR_SAVE);
CVar r_multisample("r_multisample", "1", CVAR_SAVE);
CVar r_multisample_samples("r_multisample_samples", "4", CVAR_SAVE);

CVar r_debug_lightmaps("r_debug_lightmaps", "0", CVAR_NONE);
CVar r_debug_lightmaps_directional("r_debug_lightmaps_directional", "0", CVAR_NONE);
CVar r_debug_vertexlight("r_debug_vertexlight", "0", CVAR_NONE);
CVar r_debug_vertexlight_directional("r_debug_vertexlight_directional", "0", CVAR_NONE);

CVar r_fov("fov", "75.0", CVAR_SAVE);
CVar r_skybox("r_skybox", "1", CVAR_SAVE);
CVar r_wireframe("r_wireframe", "0", CVAR_NONE);
CVar r_fullbright("r_fullbright", "0", CVAR_NONE);

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

    if (!m_worldShader.Load("shaders/world.vert", "shaders/world.frag"))
    {
        Console::Error("World: Failed to load shaders");
        return false;
    }

    // Global GL State
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    m_postProcess = std::make_unique<R_PostProcess>();
    m_postProcess->Init(1280, 720);

    m_bspRenderer = std::make_unique<R_BSP>();
    m_modelRenderer = std::make_unique<R_Models>();
    m_skyRenderer = std::make_unique<R_Sky>();
    m_particleRenderer = std::make_unique<R_Particles>();

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

    if (!m_bspRenderer->Init(map))
    {
        Console::Error("BSP renderer failed to initialize.");
        return false;
    }

    if (!m_modelRenderer->Init(map))
    {
        Console::Warn("Static Prop renderer failed to initialize.");
    }

    if (!m_skyRenderer->Init(map.skyName))
    {
        Console::Warn("Skybox renderer failed to initialize.");
    }

    if (!m_particleRenderer->Init())
    {
        Console::Error("Particle renderer failed to initialize.");
    }

    Physics::AddBSPCollision(map.collision.vertices, map.collision.indices);

    for (const auto& entData : map.entities)
    {
        EntityManager::SpawnEntity(entData.className, entData);
    }

    return true;
}

void Renderer::DrawWorld(Camera& camera, GLuint cubemapToExclude)
{
    m_worldShader.Bind();
    m_worldShader.SetMat4("u_projection", camera.GetProjectionMatrix());
    m_worldShader.SetMat4("u_view", camera.GetViewMatrix());
    m_worldShader.SetMat4("u_model", glm::mat4(1.0f));
    m_worldShader.SetVec3("u_viewPos", camera.position);
    m_worldShader.SetInt("u_fullbright", r_fullbright.GetInt());

    m_worldShader.SetInt("u_diffuse", 0);
    m_worldShader.SetInt("u_lightmap", 1);
    m_worldShader.SetInt("u_normal", 2);
    m_worldShader.SetInt("u_specular", 3);
    m_worldShader.SetInt("u_cubemap", 4);

    m_worldShader.SetBool("u_useCubemap", false);
    const Cubemap::CubemapProbe* probe = Cubemap::FindClosest(camera.position);
    if (probe && probe->textureID != 0 && probe->textureID != cubemapToExclude)
    {
        m_worldShader.SetBool("u_useCubemap", true);
        m_worldShader.SetVec3("u_cubemapOrigin", probe->origin);
        m_worldShader.SetVec3("u_cubemapMins", probe->mins);
        m_worldShader.SetVec3("u_cubemapMaxs", probe->maxs);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_CUBE_MAP, probe->textureID);
    }

    // Determine debug mode
    int debugMode = 0;
    if (r_debug_lightmaps.GetInt())
        debugMode = 1;
    else if (r_debug_lightmaps_directional.GetInt())
        debugMode = 2;
    else if (r_debug_vertexlight.GetInt())
        debugMode = 3;
    else if (r_debug_vertexlight_directional.GetInt())
        debugMode = 4;
    m_worldShader.SetInt("u_debugMode", debugMode);

    Frustum frustum = camera.GetFrustum();

    // Draw BSP
    if (m_bspRenderer)
    {
        m_bspRenderer->Draw(m_worldShader, frustum);
    }

    // Draw models
    if (m_modelRenderer)
    {
        m_modelRenderer->Draw(m_worldShader, frustum);
    }

    m_worldShader.SetBool("u_useCubemap", false);

    // Draw sky
    if (m_skyRenderer && r_skybox.GetInt() > 0)
    {
        m_skyRenderer->Draw(camera);
    }

    // Draw particles
    if (m_particleRenderer)
    {
        m_particleRenderer->Draw(camera);
    }
}

void Renderer::Render(Camera& camera)
{
    int w, h;
    SDL_GetWindowSize(m_windowRef->Get(), &w, &h);

    camera.SetFOV(r_fov.GetFloat());
    camera.SetAspectRatio((float)w / (float)h);

    if (r_wireframe.GetInt() > 0)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    m_postProcess->Begin();
    glViewport(0, 0, w, h);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    DrawWorld(camera, 0);

    m_postProcess->End();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Draw postprocessing
    glDisable(GL_DEPTH_TEST);
    m_postProcess->Draw();
    glEnable(GL_DEPTH_TEST);

    m_windowRef->Swap();
}

void Renderer::RenderWorld(Camera& camera, GLuint cubemapToExclude)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    DrawWorld(camera, cubemapToExclude);
}

void Renderer::OnWindowResize(int w, int h)
{
    if (m_postProcess)
    {
        m_postProcess->Rescale(w, h);
    }
}

void Renderer::Shutdown()
{
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
}