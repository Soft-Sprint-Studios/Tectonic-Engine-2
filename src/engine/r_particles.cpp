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

    glCreateVertexArrays(1, &m_vao);
    glCreateBuffers(1, &m_vbo);
    glNamedBufferData(m_vbo, 10000 * sizeof(PVertex), NULL, GL_DYNAMIC_DRAW);

    glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, sizeof(PVertex));

    glEnableVertexArrayAttrib(m_vao, 0);
    glVertexArrayAttribFormat(m_vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(m_vao, 0, 0);

    glEnableVertexArrayAttrib(m_vao, 1);
    glVertexArrayAttribFormat(m_vao, 1, 4, GL_FLOAT, GL_FALSE, offsetof(PVertex, col));
    glVertexArrayAttribBinding(m_vao, 1, 0);

    glEnableVertexArrayAttrib(m_vao, 2);
    glVertexArrayAttribFormat(m_vao, 2, 1, GL_FLOAT, GL_FALSE, offsetof(PVertex, size));
    glVertexArrayAttribBinding(m_vao, 2, 0);
    
    return true;
}

void R_Particles::Draw(const Camera& camera, uint32_t depthTex)
{
    auto& systems = Particles::GetActiveSystems();
    if (systems.empty()) 
        return;

    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    m_shader.Bind();
    m_shader.SetMat4("u_proj", camera.GetProjectionMatrix());
    m_shader.SetMat4("u_view", camera.GetViewMatrix());
    m_shader.SetMat4("u_invProj", glm::inverse(camera.GetProjectionMatrix()));
    glBindTextureUnit(1, depthTex);

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
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        }
        else
        {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        Materials::GetTexture(s->GetDef().textureName)->Bind(0);

        std::vector<PVertex> vts;
        for (auto& p : pts) 
            vts.push_back({p.pos, p.col, p.size});

        glNamedBufferSubData(m_vbo, 0, vts.size() * sizeof(PVertex), vts.data());
        glDrawArrays(GL_POINTS, 0, (GLsizei)vts.size());
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void R_Particles::Shutdown() 
{ 
    glDeleteVertexArrays(1, &m_vao); 
    glDeleteBuffers(1, &m_vbo); 
}