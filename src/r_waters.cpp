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
#include "r_waters.h"
#include "renderer.h"
#include "timing.h"
#include "resources.h"
#include "cvar.h"
#include <algorithm>

void R_Waters::Init(int width, int height, int downsample)
{
    m_width = width; m_height = height;
    int ds = std::max(1, downsample);
    int fboW = width / ds;
    int fboH = height / ds;

    m_shader.Load("shaders/water.vert", "shaders/water.frag");
    m_normalMap = Resources::LoadTexture("textures/water_normal.png", false);
    m_dudvMap = Resources::LoadTexture("textures/water_dudv.png", false);

    glGenFramebuffers(1, &m_reflectFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_reflectFBO);

    glGenTextures(1, &m_reflectTex);
    glBindTexture(GL_TEXTURE_2D, m_reflectTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, fboW, fboH, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_reflectTex, 0);

    glGenRenderbuffers(1, &m_reflectRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_reflectRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, fboW, fboH);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_reflectRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void R_Waters::AddSurface(const WaterSurface& surface)
{
    m_surfaces.push_back(surface);
    m_starts.push_back((GLint)surface.start);
    m_counts.push_back((GLsizei)surface.count);
}

void R_Waters::ClearSurfaces()
{
    m_surfaces.clear();
    m_starts.clear();
    m_counts.clear();
}

void R_Waters::RenderReflection(Renderer* renderer, const Camera& mainCam)
{
    if (m_surfaces.empty())
        return;

    // Planar reflection algorithm
    float h = m_surfaces[0].height;
    Camera reflectCam = mainCam;
    float dist = 2.0f * (mainCam.position.y - h);
    reflectCam.position.y -= dist;
    reflectCam.pitch = -mainCam.pitch;

    m_reflectView = reflectCam.GetViewMatrix();
    m_reflectProj = reflectCam.GetProjectionMatrix();

    glBindFramebuffer(GL_FRAMEBUFFER, m_reflectFBO);
    int ds = std::max(1, CVar::Find("r_water_downsample")->GetInt());
    glViewport(0, 0, m_width / ds, m_height / ds);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderer->RenderWorld(reflectCam, 0, false);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void R_Waters::Draw(const Camera& camera, GLuint vao)
{
    if (m_surfaces.empty() || m_starts.empty())
        return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_shader.Bind();
    m_shader.SetMat4("u_projection", camera.GetProjectionMatrix());
    m_shader.SetMat4("u_view", camera.GetViewMatrix());
    m_shader.SetMat4("u_model", glm::mat4(1.0f));
    m_shader.SetMat4("u_reflectView", m_reflectView);
    m_shader.SetMat4("u_reflectProj", m_reflectProj);
    m_shader.SetVec3("u_viewPos", camera.position);
    m_shader.SetFloat("u_time", (float)Time::TotalTime());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_reflectTex);
    m_shader.SetInt("u_reflectionTexture", 0);

    if (m_dudvMap)
        m_dudvMap->Bind(1);
    m_shader.SetInt("u_dudvMap", 1);

    if (m_normalMap)
        m_normalMap->Bind(2);
    m_shader.SetInt("u_normalMap", 2);

    glBindVertexArray(vao);
    for (const auto& s : m_surfaces)
    {
        glDrawArrays(GL_TRIANGLES, s.start, s.count);
    }

    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

void R_Waters::Shutdown()
{
    if (m_reflectFBO)
        glDeleteFramebuffers(1, &m_reflectFBO);
    if (m_reflectTex)
        glDeleteTextures(1, &m_reflectTex);
    if (m_reflectRBO)
        glDeleteRenderbuffers(1, &m_reflectRBO);

    m_surfaces.clear();
    m_starts.clear();
    m_counts.clear();
}