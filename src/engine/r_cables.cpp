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
#include "r_cables.h"

void R_Cables::Init()
{
    m_shader.Load("shaders/cable.vert", "shaders/cable.frag");
    
    glCreateVertexArrays(1, &m_vao);
    glCreateBuffers(1, &m_vbo);
}

static glm::vec3 GetBezierPoint(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2) 
{
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    glm::vec3 p = p0 * uu;
    p += p1 * (2.0f * u * t);
    p += p2 * tt;
    return p;
}

void R_Cables::Draw(const Camera& camera, const std::vector<std::shared_ptr<Cable>>& cables)
{
    if (cables.empty()) 
        return;

    m_shader.Bind();
    m_shader.SetMat4("u_view", camera.GetViewMatrix());
    m_shader.SetMat4("u_projection", camera.GetProjectionMatrix());
    m_shader.SetVec3("u_viewPos", camera.position);

    glBindVertexArray(m_vao);

    for (auto& cable : cables)
    {
        if (!cable->IsActive()) 
            continue;

        auto& d = cable->GetDef();

        glm::vec3 mid = (d.startPos + d.endPos) * 0.5f;
        mid.y -= d.slack;

        std::vector<glm::vec3> vertices;
        for (int i = 0; i <= d.segments; i++)
        {
            float t = (float)i / (float)d.segments;
            glm::vec3 p = GetBezierPoint(t, d.startPos, mid, d.endPos);

            glm::vec3 nextP = (i < d.segments) ? GetBezierPoint((float)(i+1)/d.segments, d.startPos, mid, d.endPos) : p;
            glm::vec3 dir = glm::normalize(nextP - p);
            if (i == d.segments) 
                dir = glm::normalize(p - GetBezierPoint((float)(i-1)/d.segments, d.startPos, mid, d.endPos));
            
            glm::vec3 viewDir = glm::normalize(camera.position - p);
            glm::vec3 side = glm::normalize(glm::cross(dir, viewDir)) * (d.width * 0.5f);
            
            vertices.push_back(p - side);
            vertices.push_back(p + side);
        }

        glNamedBufferData(m_vbo, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STREAM_DRAW);
        glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, sizeof(glm::vec3));
        glEnableVertexArrayAttrib(m_vao, 0);
        glVertexArrayAttribFormat(m_vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(m_vao, 0, 0);
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, (GLsizei)vertices.size());
    }
}

void R_Cables::Shutdown()
{
    if (m_vao) 
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) 
        glDeleteBuffers(1, &m_vbo);
}