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
#include "r_overlay.h"
#include "materials.h"
#include "screen_overlay.h"
#include <cstring>
#include <glm/gtc/type_ptr.hpp>

void R_Overlay::Init()
{
    m_shader.Load("shaders/overlay.vert", "shaders/overlay.frag");
    
    m_sTexture = bgfx::createUniform("s_texture", bgfx::UniformType::Sampler);
    m_uOverlayParams = bgfx::createUniform("u_overlayParams", bgfx::UniformType::Vec4);

    m_layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
}

void R_Overlay::Draw(bgfx::ViewId viewId)
{
    const auto& s = ScreenOverlay::GetSettings();
    if (!s.active || s.textureName.empty()) 
        return;

    auto tex = Materials::GetTexture(s.textureName);
    if (!tex) 
        return;

    const uint32_t numVertices = 6;
    if (bgfx::getAvailTransientVertexBuffer(numVertices, m_layout) >= numVertices)
    {
        bgfx::TransientVertexBuffer tvb;
        bgfx::allocTransientVertexBuffer(&tvb, numVertices, m_layout);

        struct Vertex
        { 
            float x, y, z; 
            float u, v; 
        };
        Vertex* v = (Vertex*)tvb.data;
        v[0] = { -1.0f,  1.0f, 0.0f, 0.0f, 0.0f };
        v[1] = { -1.0f, -1.0f, 0.0f, 0.0f, 1.0f };
        v[2] = {  1.0f, -1.0f, 0.0f, 1.0f, 1.0f };
        v[3] = { -1.0f,  1.0f, 0.0f, 0.0f, 0.0f };
        v[4] = {  1.0f, -1.0f, 0.0f, 1.0f, 1.0f };
        v[5] = {  1.0f,  1.0f, 0.0f, 1.0f, 0.0f };

        uint64_t blend = BGFX_STATE_BLEND_ALPHA;
        if (s.renderMode == 0) 
            blend = BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE);
        else if (s.renderMode == 2) 
            blend = BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_DST_COLOR, BGFX_STATE_BLEND_ZERO);
        else if (s.renderMode == 3) 
            blend = BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_COLOR);

        uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | blend;
        
        float params[4] = { s.alpha, 0.0f, 0.0f, 0.0f };
        bgfx::setUniform(m_uOverlayParams, params);

        bgfx::setTexture(0, m_sTexture, tex->GetHandle());
        bgfx::setVertexBuffer(0, &tvb);
        bgfx::setState(state);
        
        glm::mat4 identity(1.0f);
        bgfx::setTransform(glm::value_ptr(identity));
        bgfx::submit(viewId, m_shader.GetProgram());
    }
}

void R_Overlay::Shutdown()
{
    if (bgfx::isValid(m_sTexture))
    {
        bgfx::destroy(m_sTexture);
        bgfx::destroy(m_uOverlayParams);
        m_sTexture = m_uOverlayParams = BGFX_INVALID_HANDLE;
    }
}