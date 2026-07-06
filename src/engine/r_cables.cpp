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
#include <glm/gtc/type_ptr.hpp>

void R_Cables::Init()
{
    m_shader.Load("shaders/cable.vert", "shaders/cable.frag");
    
    m_layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .end();
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

void R_Cables::Draw(bgfx::ViewId viewId, const Camera& camera, const std::vector<std::shared_ptr<Cable>>& cables)
{
    if (cables.empty()) 
        return;

    uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW | BGFX_STATE_PT_TRISTRIP;

    glm::mat4 identity(1.0f);

    for (auto& cable : cables)
    {
        if (!cable->IsActive()) 
            continue;

        auto& d = cable->GetDef();

        glm::vec3 mid = (d.startPos + d.endPos) * 0.5f;
        mid.y -= d.slack;

        std::vector<glm::vec3> vertices;
        vertices.reserve((d.segments + 1) * 2);

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

        uint32_t numVerts = (uint32_t)vertices.size();
        if (numVerts == bgfx::getAvailTransientVertexBuffer(numVerts, m_layout))
        {
            bgfx::TransientVertexBuffer tvb;
            bgfx::allocTransientVertexBuffer(&tvb, numVerts, m_layout);
            std::memcpy(tvb.data, vertices.data(), numVerts * sizeof(glm::vec3));

            bgfx::setTransform(glm::value_ptr(identity));
            bgfx::setVertexBuffer(0, &tvb);
            bgfx::setState(state);
            bgfx::submit(viewId, m_shader.GetProgram());
        }
    }
}

void R_Cables::Shutdown()
{
}