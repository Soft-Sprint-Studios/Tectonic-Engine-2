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
#include "r_monitors.h"
#include "renderer.h"
#include "entities.h"
#include "r_bsp.h"
#include "func_monitor.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

R_Monitors::R_Monitors()
{
}

R_Monitors::~R_Monitors()
{
    Shutdown();
}

void R_Monitors::Init()
{
    m_shader.Load("shaders/monitor.vert", "shaders/monitor.frag");
    m_sTexture = bgfx::createUniform("s_texture", bgfx::UniformType::Sampler);
    m_uMonitorParams = bgfx::createUniform("u_monitorParams", bgfx::UniformType::Vec4);
}

void R_Monitors::RecreateFBO(int resolution)
{
    if (m_currentResolution == resolution && bgfx::isValid(m_fbo))
    {
        return;
    }

    if (bgfx::isValid(m_fbo))
    {
        bgfx::destroy(m_fbo);
        bgfx::destroy(m_colorTexture);
        bgfx::destroy(m_depthTexture);
    }

    m_currentResolution = resolution;

    uint64_t flags = BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
    m_colorTexture = bgfx::createTexture2D((uint16_t)resolution, (uint16_t)resolution, false, 1, bgfx::TextureFormat::RGBA16F, flags);
    m_depthTexture = bgfx::createTexture2D((uint16_t)resolution, (uint16_t)resolution, false, 1, bgfx::TextureFormat::D24S8, flags);

    bgfx::TextureHandle attachments[] = { m_colorTexture, m_depthTexture };
    m_fbo = bgfx::createFrameBuffer(2, attachments, false);
}

void R_Monitors::RenderTextures(Renderer* renderer)
{
    auto monitors = Monitors::GetActiveMonitors();
    if (monitors.empty())
    {
        return;
    }

    auto m = monitors[0];

    if (!m->IsActive())
    {
        return;
    }

    if (m->GetDef().isStatic && m->HasRenderedOnce())
    {
        return;
    }

    auto camEnt = EntityManager::FindEntityByName(m->GetDef().cameraName);
    if (!camEnt)
    {
        return;
    }

    RecreateFBO(m->GetDef().resolution);

    float fov = camEnt->GetFloat("fov", 75.0f);
    float radius = camEnt->GetFloat("radius", 2000.0f);

    Camera cam(fov, 1.0f, 0.1f, radius);
    cam.position = camEnt->GetOrigin();
    glm::vec3 entAngles = camEnt->GetAngles();
    glm::mat4 rotation = glm::mat4(1.0f);

    rotation = glm::rotate(rotation, glm::radians(entAngles.y + 90.0f), glm::vec3(0, 1, 0));
    rotation = glm::rotate(rotation, glm::radians(-entAngles.x), glm::vec3(1, 0, 0));
    rotation = glm::rotate(rotation, glm::radians(entAngles.z), glm::vec3(0, 0, 1));

    glm::vec3 forward = glm::vec3(rotation * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));

    cam.pitch = glm::degrees(asin(forward.y));
    cam.yaw = glm::degrees(atan2(forward.z, forward.x));

    renderer->RenderWorld(cam, 0, true, RenderView::MonitorGBuffer, RenderView::MonitorResolve, m_fbo);

    if (m->GetDef().isStatic)
    {
        m->SetRenderedOnce(true);
    }
}

void R_Monitors::Draw(bgfx::ViewId viewId, const Camera& camera, R_BSP* bsp)
{
    if (!bgfx::isValid(m_colorTexture))
    {
        return;
    }

    for (const auto& ent : EntityManager::GetEntities())
    {
        if (ent->GetClassName() != "func_monitor" || !ent->IsEnabled())
        {
            continue;
        }

        auto monitor_ent = std::dynamic_pointer_cast<FuncMonitor>(ent);
        if (!monitor_ent) 
            continue;

        auto monitor_handle = monitor_ent->GetHandle();
        if (!monitor_handle) 
            continue;

        auto& def = monitor_handle->GetDef();

        btVector3 bmin, bmax;
        float monitorW = 512.0f;
        float monitorH = 512.0f;
        if (ent->GetPhysObject())
        {
            ent->GetPhysObject()->getCollisionShape()->getAabb(ent->GetPhysObject()->getWorldTransform(), bmin, bmax);
            glm::vec3 size(bmax.x() - bmin.x(), bmax.y() - bmin.y(), bmax.z() - bmin.z());
            size *= 32.0f;

            float dims[3] = { size.x, size.y, size.z };
            std::sort(dims, dims + 3);
            monitorW = dims[2];
            monitorH = dims[1];
        }

        float params[4] = { def.grayscale ? 1.0f : 0.0f, monitorW, monitorH, (float)def.resolution };
        bgfx::setUniform(m_uMonitorParams, params);

        bgfx::setTexture(10, m_sTexture, m_colorTexture);

        glm::mat4 model = glm::translate(glm::mat4(1.0f), ent->GetOrigin());
        glm::vec3 ang = ent->GetAngles();
        model = glm::rotate(model, glm::radians(ang.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(ang.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(ang.z), glm::vec3(0, 0, 1));

        bsp->DrawBModel(ent->GetBModelIndex(), m_shader, viewId, model, camera.position, true);
    }
}

void R_Monitors::Shutdown()
{
    if (bgfx::isValid(m_fbo))
    {
        bgfx::destroy(m_fbo);
        m_fbo = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_colorTexture))
    {
        bgfx::destroy(m_colorTexture);
        m_colorTexture = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_depthTexture))
    {
        bgfx::destroy(m_depthTexture);
        m_depthTexture = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_sTexture))
    {
        bgfx::destroy(m_sTexture);
        bgfx::destroy(m_uMonitorParams);
        m_sTexture = BGFX_INVALID_HANDLE;
        m_uMonitorParams = BGFX_INVALID_HANDLE;
    }

    m_currentResolution = 0;
}