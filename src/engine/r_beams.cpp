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
#include "r_beams.h"
#include "r_state.h"
#include "timing.h"

void R_Beams::Init()
{
    m_shader.Load("shaders/beam.vert", "shaders/beam.frag");

    float verts[] = 
    {
        0, -1,  1, -1,  1, 1,
        0, -1,  1,  1,  0, 1
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
}

void R_Beams::Draw(const Camera& camera, const std::vector<std::shared_ptr<Beam>>& beams)
{
    if (beams.empty()) 
        return;

    R_State::SetBlending(true);
    R_State::SetBlendFunc(GL_SRC_ALPHA, GL_ONE);
    R_State::SetDepthMask(false);
    R_State::SetCullFace(GL_FRONT);

    m_shader.Bind();
    m_shader.SetMat4("u_view", camera.GetViewMatrix());
    m_shader.SetMat4("u_projection", camera.GetProjectionMatrix());
    m_shader.SetVec3("u_viewPos", camera.position);
    m_shader.SetFloat("u_time", (float)Time::TotalTime());

    glBindVertexArray(m_vao);
    for (auto& b : beams)
    {
        if (!b->IsActive()) 
            continue;

        auto& d = b->GetDef();
        m_shader.SetVec3("u_startPos", d.startPos);
        m_shader.SetVec3("u_endPos", d.endPos);
        m_shader.SetVec3("u_color", d.color);
        m_shader.SetFloat("u_width", d.width);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    R_State::SetCullFace(GL_BACK);
    R_State::SetDepthMask(true);
    R_State::SetBlending(false);
}

void R_Beams::Shutdown()
{
    if (m_vao) 
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) 
        glDeleteBuffers(1, &m_vbo);
}