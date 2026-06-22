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
#include <glm/gtc/matrix_transform.hpp>
#include "r_bsp.h"
#include "func_monitor.h"

void R_Monitors::Init()
{
    m_shader.Load("shaders/monitor.vert", "shaders/monitor.frag");
}

R_Monitors::RenderTarget& R_Monitors::GetTarget(Monitor* m)
{
    auto& def = m->GetDef();

    // Create new FBO if it doesnt exist or resolution changed
    if (m_targets.find(m) == m_targets.end() || m_targets[m].res != def.resolution)
    {
        RenderTarget& rt = m_targets[m];
        if (rt.fbo)
        {
            glDeleteFramebuffers(1, &rt.fbo);
            glDeleteTextures(1, &rt.texture);
            glDeleteRenderbuffers(1, &rt.rbo);
        }

        rt.res = def.resolution;
        glCreateFramebuffers(1, &rt.fbo);
        glCreateTextures(GL_TEXTURE_2D, 1, &rt.texture);
        glTextureStorage2D(rt.texture, 1, GL_RGB16F, rt.res, rt.res);
        glTextureParameteri(rt.texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(rt.texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glNamedFramebufferTexture(rt.fbo, GL_COLOR_ATTACHMENT0, rt.texture, 0);

        glCreateRenderbuffers(1, &rt.rbo);
        glNamedRenderbufferStorage(rt.rbo, GL_DEPTH_COMPONENT24, rt.res, rt.res);
        glNamedFramebufferRenderbuffer(rt.fbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rt.rbo);
    }
    return m_targets[m];
}

void R_Monitors::RenderTextures(Renderer* renderer)
{
    auto monitors = Monitors::GetActiveMonitors();
    for (auto& m : monitors)
    {
        if (!m->IsActive())
        {
            continue;
        }

        if (m->GetDef().isStatic && m->HasRenderedOnce())
        {
            continue;
        }

        auto camEnt = EntityManager::FindEntityByName(m->GetDef().cameraName);
        if (!camEnt)
        {
            continue;
        }

        RenderTarget& rt = GetTarget(m.get());

        // Setup a temporary camera
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

        glViewport(0, 0, rt.res, rt.res);
        glBindFramebuffer(GL_FRAMEBUFFER, rt.fbo);

        renderer->RenderWorld(cam, 0, true);

        if (m->GetDef().isStatic)
        {
            m->SetRenderedOnce(true);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void R_Monitors::Draw(const Camera& camera, R_BSP* bsp)
{
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    m_shader.Bind();
    m_shader.SetMat4("u_view", camera.GetViewMatrix());
    m_shader.SetMat4("u_projection", camera.GetProjectionMatrix());

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

        auto it = m_targets.find(monitor_handle.get());
        if (it == m_targets.end()) 
            continue;

        auto& def = monitor_handle->GetDef();
        m_shader.SetInt("u_grayscale", def.grayscale ? 1 : 0);

        glBindTextureUnit(0, it->second.texture);

        glm::mat4 model = glm::translate(glm::mat4(1.0f), ent->GetOrigin());
        glm::vec3 ang = ent->GetAngles();
        model = glm::rotate(model, glm::radians(ang.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(ang.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(ang.z), glm::vec3(0, 0, 1));

        bsp->DrawBModel(ent->GetBModelIndex(), m_shader, model, true);
    }
}

void R_Monitors::Shutdown()
{
    for (auto& pair : m_targets)
    {
        glDeleteFramebuffers(1, &pair.second.fbo);
        glDeleteTextures(1, &pair.second.texture);
        glDeleteRenderbuffers(1, &pair.second.rbo);
    }
    m_targets.clear();
}