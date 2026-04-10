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
#include "r_sprites.h"
#include "materials.h"

void R_Sprites::Init()
{
    m_shader.Load("shaders/sprite.vert", "shaders/sprite.frag");

    float quad[] = {
        -0.5f,  0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f, 0.0f,
         0.5f, -0.5f, 1.0f, 0.0f,
        -0.5f,  0.5f, 0.0f, 1.0f,
         0.5f, -0.5f, 1.0f, 0.0f,
         0.5f,  0.5f, 1.0f, 1.0f
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void R_Sprites::Draw(const Camera& camera, const std::vector<std::shared_ptr<Sprite>>& sprites)
{
    if (sprites.empty()) 
        return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    m_shader.Bind();
    m_shader.SetMat4("u_view", camera.GetViewMatrix());
    m_shader.SetMat4("u_projection", camera.GetProjectionMatrix());

    glm::mat4 v = camera.GetViewMatrix();
    m_shader.SetVec3("u_viewRight", { v[0][0], v[1][0], v[2][0] });
    m_shader.SetVec3("u_viewUp", { v[0][1], v[1][1], v[2][1] });

    glBindVertexArray(m_vao);
    for (const auto& sprite : sprites)
    {
        if (!sprite->IsActive()) 
            continue;

        const auto& def = sprite->GetDef();
        auto tex = Materials::GetTexture(def.textureName);
        if (tex)
        {
            tex->Bind(0);
            m_shader.SetVec3("u_worldPos", sprite->GetPosition());
            m_shader.SetVec2("u_scale", def.scale);
            m_shader.SetVec4("u_color", def.color);
            m_shader.SetInt("u_cylindrical", def.cylindrical ? 1 : 0);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }
    glDisable(GL_BLEND);
}

void R_Sprites::Shutdown()
{
    if (m_vao) 
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) 
        glDeleteBuffers(1, &m_vbo);
}