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

void R_Overlay::Init()
{
    m_shader.Load("shaders/overlay.vert", "shaders/overlay.frag");

    float quad[] = 
    { 
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f 
    };

    glCreateVertexArrays(1, &m_vao);
    glCreateBuffers(1, &m_vbo);
    glNamedBufferData(m_vbo, sizeof(quad), quad, GL_STATIC_DRAW);

    glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, 4 * sizeof(float));

    glEnableVertexArrayAttrib(m_vao, 0);
    glVertexArrayAttribFormat(m_vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(m_vao, 0, 0);

    glEnableVertexArrayAttrib(m_vao, 1);
    glVertexArrayAttribFormat(m_vao, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
    glVertexArrayAttribBinding(m_vao, 1, 0);
}

void R_Overlay::Draw()
{
    const auto& s = ScreenOverlay::GetSettings();
    if (!s.active || s.textureName.empty())
    {
        return;
    }

    auto tex = Materials::GetTexture(s.textureName);
    if (!tex)
    {
        return;
    }

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);

    switch (s.renderMode)
    {
    case 0:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        break;

    case 1:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;

    case 2:
        glBlendFunc(GL_DST_COLOR, GL_ZERO);
        break;

    case 3:
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
        break;

    default:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    }

    m_shader.Bind();
    tex->Bind(0);
    m_shader.SetFloat("u_alpha", s.alpha);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void R_Overlay::Shutdown()
{
    if (m_vao) 
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) 
        glDeleteBuffers(1, &m_vbo);
}