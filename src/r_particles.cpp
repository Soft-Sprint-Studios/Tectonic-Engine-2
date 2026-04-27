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
#include "r_particles.h"
#include "r_state.h"
#include "particles.h"
#include "materials.h"
#include "console.h"
#include <r_ui.h>

R_Particles::R_Particles() : m_vao(0), m_vbo(0) 
{
}

R_Particles::~R_Particles() 
{ 
    Shutdown(); 
}

bool R_Particles::Init()
{
    m_shader.Load("shaders/particle.vert", "shaders/particle.frag", "shaders/particle.geom");

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, 10000 * sizeof(PVertex), NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PVertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(PVertex), (void*)offsetof(PVertex, col));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(PVertex), (void*)offsetof(PVertex, size));
    
    return true;
}

void R_Particles::Draw(const Camera& camera, uint32_t depthTex)
{
    auto& systems = Particles::GetActiveSystems();
    if (systems.empty()) 
        return;

    R_State::SetBlending(true);
    R_State::SetDepthMask(false);
    m_shader.Bind();
    m_shader.SetMat4("u_proj", camera.GetProjectionMatrix());
    m_shader.SetMat4("u_view", camera.GetViewMatrix());
    m_shader.SetMat4("u_invProj", glm::inverse(camera.GetProjectionMatrix()));
    m_shader.SetInt("u_depthTexture", 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthTex);

    int w, h;
    SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &w, &h);
    m_shader.SetVec2("u_screenSize", { (float)w, (float)h });

    glm::mat4 v = camera.GetViewMatrix();
    m_shader.SetVec3("u_right", glm::vec3(v[0][0], v[1][0], v[2][0]));
    m_shader.SetVec3("u_up", glm::vec3(v[0][1], v[1][1], v[2][1]));

    glBindVertexArray(m_vao);
    for (auto& s : systems)
    {
        auto& pts = s->GetParticles();
        if (pts.empty()) 
            continue;

        if (s->GetDef().additive)
        {
            R_State::SetBlendFunc(GL_SRC_ALPHA, GL_ONE);
        }
        else
        {
            R_State::SetBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        Materials::GetTexture(s->GetDef().textureName)->Bind(0);

        std::vector<PVertex> vts;
        for (auto& p : pts) 
            vts.push_back({p.pos, p.col, p.size});

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vts.size() * sizeof(PVertex), vts.data());
        glDrawArrays(GL_POINTS, 0, (GLsizei)vts.size());
    }
    R_State::SetDepthMask(true);
    R_State::SetBlending(false);
}

void R_Particles::Shutdown() 
{ 
    glDeleteVertexArrays(1, &m_vao); 
    glDeleteBuffers(1, &m_vbo); 
}