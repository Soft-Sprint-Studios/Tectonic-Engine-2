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
#include "cvar.h"

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

    glGenBuffers(1, &m_instanceSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_instanceSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 1024 * sizeof(glm::mat4), NULL, GL_DYNAMIC_DRAW);
}

void R_Decals::Draw(const Camera& camera, const std::vector<std::shared_ptr<Decal>>& decals)
{
    if (decals.empty())
    {
        return;
    }

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);

    m_shader.Bind();
    m_shader.SetMat4("u_view", camera.GetViewMatrix());
    m_shader.SetMat4("u_projection", camera.GetProjectionMatrix());

    m_shader.SetVec3("u_viewPos", camera.position);
    m_shader.SetInt("u_mat_parallax", CVar::GetInt("mat_parallax"));
    m_shader.SetFloat("u_pomMinSteps", CVar::GetFloat("mat_parallax_min_steps"));
    m_shader.SetFloat("u_pomMaxSteps", CVar::GetFloat("mat_parallax_max_steps"));
    m_shader.SetInt("u_pomRefineSteps", CVar::GetInt("mat_parallax_refine"));

    std::unordered_map<std::string, std::vector<glm::mat4>> groups;
    for (const auto& d : decals)
    {
        groups[d->GetDef().textureName].push_back(d->GetModelMatrix());
    }

    glBindVertexArray(m_vao);
    for (auto& [texName, matrices] : groups)
    {
        auto diff = Materials::GetTexture(texName);
        auto norm = Materials::GetNormalMap(texName);
        auto mraoh = Materials::GetMRAOMap(texName);

        diff->Bind(0);
        norm->Bind(1);
        mraoh->Bind(2);
        m_shader.SetFloat("u_heightScale", Materials::GetHeightScale(texName));

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_instanceSSBO);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, matrices.size() * sizeof(glm::mat4), matrices.data());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, m_instanceSSBO);

        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, (GLsizei)matrices.size());
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
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
    if (m_instanceSSBO)
    {
        glDeleteBuffers(1, &m_instanceSSBO);
    }
    m_vao = m_vbo = m_ebo = 0;
}