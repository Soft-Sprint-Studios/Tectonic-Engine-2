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
#include "timing.h"
#include <glm/gtc/type_ptr.hpp>

void R_Beams::Init()
{
    m_shader.Load("shaders/beam.vert", "shaders/beam.frag");

    m_layout.begin()
        .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
        .end();

    float verts[] = 
    {
        0, -1,  1, -1,  1, 1,
        0, -1,  1,  1,  0, 1
    };

    m_vbo = bgfx::createVertexBuffer(bgfx::copy(verts, sizeof(verts)), m_layout);

    m_uBeamParams = bgfx::createUniform("u_beamParams", bgfx::UniformType::Vec4);
    m_uStartPosLocal = bgfx::createUniform("u_startPosLocal", bgfx::UniformType::Vec4);
    m_uEndPosLocal = bgfx::createUniform("u_endPosLocal", bgfx::UniformType::Vec4);
    m_uViewPosLocal = bgfx::createUniform("u_viewPosLocal", bgfx::UniformType::Vec4);
    m_uBeamColor = bgfx::createUniform("u_beamColor", bgfx::UniformType::Vec4);
    m_uBeamTime = bgfx::createUniform("u_beamTime", bgfx::UniformType::Vec4);
}

void R_Beams::Draw(bgfx::ViewId viewId, const Camera& camera, const std::vector<std::shared_ptr<Beam>>& beams)
{
    if (beams.empty()) 
        return;

    uint64_t state = BGFX_STATE_WRITE_RGB  | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE);

    float viewPos[4] = { camera.position.x, camera.position.y, camera.position.z, 0.0f };
    bgfx::setUniform(m_uViewPosLocal, viewPos);

    float timeParam[4] = { (float)Time::TotalTime(), 0.0f, 0.0f, 0.0f };
    bgfx::setUniform(m_uBeamTime, timeParam);

    glm::mat4 identity(1.0f);

    for (auto& b : beams)
    {
        if (!b->IsActive()) 
            continue;

        auto& d = b->GetDef();

        bgfx::setTransform(glm::value_ptr(identity));

        float startPos[4] = { d.startPos.x, d.startPos.y, d.startPos.z, 0.0f };
        float endPos[4] = { d.endPos.x, d.endPos.y, d.endPos.z, 0.0f };
        float color[4] = { d.color.x, d.color.y, d.color.z, 0.0f };
        float beamParams[4] = { d.width, 0.0f, 0.0f, 0.0f };

        bgfx::setUniform(m_uStartPosLocal, startPos);
        bgfx::setUniform(m_uEndPosLocal, endPos);
        bgfx::setUniform(m_uBeamColor, color);
        bgfx::setUniform(m_uBeamParams, beamParams);

        bgfx::setVertexBuffer(0, m_vbo);
        bgfx::setState(state);
        bgfx::submit(viewId, m_shader.GetProgram());
    }
}

void R_Beams::Shutdown()
{
    if (bgfx::isValid(m_vbo)) 
        bgfx::destroy(m_vbo);
    if (bgfx::isValid(m_uBeamParams))
    {
        bgfx::destroy(m_uBeamParams);
        bgfx::destroy(m_uStartPosLocal);
        bgfx::destroy(m_uEndPosLocal);
        bgfx::destroy(m_uViewPosLocal);
        bgfx::destroy(m_uBeamColor);
        bgfx::destroy(m_uBeamTime);
    }
    m_vbo = BGFX_INVALID_HANDLE;
    m_uBeamParams = BGFX_INVALID_HANDLE;
}