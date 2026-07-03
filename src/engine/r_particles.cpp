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
#include <SDL3/SDL.h>
#include <glm/gtc/type_ptr.hpp>

R_Particles::R_Particles()
{
}

R_Particles::~R_Particles() 
{
    Shutdown(); 
}

bool R_Particles::Init()
{
    m_shader.Load("shaders/particle.vert", "shaders/particle.frag");

    m_layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

    m_sTexture = bgfx::createUniform("s_texture", bgfx::UniformType::Sampler);
    m_uParticleParams = bgfx::createUniform("u_particleParams", bgfx::UniformType::Vec4);

    return true;
}

void R_Particles::Draw(bgfx::ViewId viewId, const Camera& camera)
{
    auto& systems = Particles::GetActiveSystems();
    if (systems.empty()) 
        return;

    uint64_t baseState = BGFX_STATE_WRITE_RGB 
                       | BGFX_STATE_WRITE_A 
                       | BGFX_STATE_DEPTH_TEST_LESS;

    int w, h;
    SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &w, &h);
    float particleParams[4] = { (float)w, (float)h, 0.0f, 0.0f };
    bgfx::setUniform(m_uParticleParams, particleParams);

    glm::mat4 vMat = camera.GetViewMatrix();
    glm::vec3 right = { vMat[0][0], vMat[1][0], vMat[2][0] };
    glm::vec3 up = { vMat[0][1], vMat[1][1], vMat[2][1] };

    glm::mat4 identity(1.0f);

    for (auto& s : systems)
    {
        auto& pts = s->GetParticles();
        if (pts.empty()) 
            continue;

        uint64_t blendState = s->GetDef().additive
            ? BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE)
            : BGFX_STATE_BLEND_ALPHA;

        uint32_t numVerts = (uint32_t)pts.size() * 4;
        uint32_t numIndices = (uint32_t)pts.size() * 6;

        if (numVerts == bgfx::getAvailTransientVertexBuffer(numVerts, m_layout) &&
            numIndices == bgfx::getAvailTransientIndexBuffer(numIndices))
        {
            bgfx::TransientVertexBuffer tvb;
            bgfx::TransientIndexBuffer tib;
            bgfx::allocTransientVertexBuffer(&tvb, numVerts, m_layout);
            bgfx::allocTransientIndexBuffer(&tib, numIndices);

            ParticleVertex* verts = (ParticleVertex*)tvb.data;
            uint16_t* indices = (uint16_t*)tib.data;

            for (size_t i = 0; i < pts.size(); ++i)
            {
                const auto& p = pts[i];
                float sSize = p.size;
                glm::vec3 corners[4] = 
                {
                    p.pos + (-right + up) * sSize,
                    p.pos + (-right - up) * sSize,
                    p.pos + (right - up) * sSize,
                    p.pos + (right + up) * sSize
                };

                verts[i * 4 + 0] = { corners[0].x, corners[0].y, corners[0].z, p.col.r, p.col.g, p.col.b, p.col.a, 0.0f, 1.0f };
                verts[i * 4 + 1] = { corners[1].x, corners[1].y, corners[1].z, p.col.r, p.col.g, p.col.b, p.col.a, 0.0f, 0.0f };
                verts[i * 4 + 2] = { corners[2].x, corners[2].y, corners[2].z, p.col.r, p.col.g, p.col.b, p.col.a, 1.0f, 0.0f };
                verts[i * 4 + 3] = { corners[3].x, corners[3].y, corners[3].z, p.col.r, p.col.g, p.col.b, p.col.a, 1.0f, 1.0f };

                uint16_t base = (uint16_t)(i * 4);
                indices[i * 6 + 0] = base + 0;
                indices[i * 6 + 1] = base + 1;
                indices[i * 6 + 2] = base + 2;
                indices[i * 6 + 3] = base + 0;
                indices[i * 6 + 4] = base + 2;
                indices[i * 6 + 5] = base + 3;
            }

            bgfx::setTransform(glm::value_ptr(identity));
            bgfx::setTexture(0, m_sTexture, Materials::GetTexture(s->GetDef().textureName)->GetHandle());
            bgfx::setVertexBuffer(0, &tvb);
            bgfx::setIndexBuffer(&tib);
            bgfx::setState(baseState | blendState);
            bgfx::submit(viewId, m_shader.GetProgram());
        }
    }
}

void R_Particles::Shutdown()
{
    if (bgfx::isValid(m_sTexture))
    {
        bgfx::destroy(m_sTexture);
        bgfx::destroy(m_uParticleParams);
        m_sTexture = m_uParticleParams = BGFX_INVALID_HANDLE;
    }
}