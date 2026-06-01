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
#include "r_decals.h"
#include "materials.h"

void R_Decals::Init()
{
    m_shader.Load("shaders/decal.vert", "shaders/decal.frag");

    DecalVertex vertices[] =
    {
        { glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec2(0.0f, 0.0f) },
        { glm::vec3(0.5f, -0.5f, 0.0f), glm::vec2(1.0f, 0.0f) },
        { glm::vec3(0.5f,  0.5f, 0.0f), glm::vec2(1.0f, 1.0f) },
        { glm::vec3(-0.5f,  0.5f, 0.0f), glm::vec2(0.0f, 1.0f) }
    };

    unsigned int indices[] =
    {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DecalVertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(DecalVertex), (void*)offsetof(DecalVertex, uv));
}

void R_Decals::Draw(const Camera& camera, const std::vector<std::shared_ptr<Decal>>& decals, GLuint lightmapTex, int screenW, int screenH)
{
    if (decals.empty())
    {
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);

    m_shader.Bind();
    m_shader.SetMat4("u_view", camera.GetViewMatrix());
    m_shader.SetMat4("u_projection", camera.GetProjectionMatrix());
    m_shader.SetInt("u_texture", 0);
    m_shader.SetInt("u_lightmap", 1);
    m_shader.SetVec2("u_screenSize", glm::vec2((float)screenW, (float)screenH));

    if (lightmapTex != 0)
    {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, lightmapTex);
    }

    glBindVertexArray(m_vao);
    for (const auto& d : decals)
    {
        const auto& def = d->GetDef();
        auto tex = Materials::GetTexture(def.textureName);
        if (tex)
        {
            tex->Bind(0);

            m_shader.SetMat4("u_model", d->GetModelMatrix());
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    glDepthMask(GL_TRUE);
}

void R_Decals::Shutdown()
{
    if (m_vao)
    {
        glDeleteVertexArrays(1, &m_vao);
    }
    if (m_vbo)
    {
        glDeleteBuffers(1, &m_vbo);
    }
    if (m_ebo)
    {
        glDeleteBuffers(1, &m_ebo);
    }
    m_vao = m_vbo = m_ebo = 0;
}