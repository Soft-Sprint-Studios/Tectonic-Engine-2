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
#include "camera.h"
#include "r_bsp.h"
#include "materials.h"
#include "entities.h"
#include <glad/glad.h>

R_BSP::R_BSP()
{
    m_vao = 0;
    m_vbo = 0;
    m_lightmapTexture = 0;
}

R_BSP::~R_BSP()
{
    Shutdown();
}

bool R_BSP::Init(const BSP::MapData& map)
{
    Shutdown();
    if (!map.loaded)
    {
        return false;
    }

    glCreateVertexArrays(1, &m_vao);
    glCreateBuffers(1, &m_vbo);
    glNamedBufferData(m_vbo, map.renderVertices.size() * sizeof(BSP::Vertex), map.renderVertices.data(), GL_STATIC_DRAW);

    glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, sizeof(BSP::Vertex));

    glEnableVertexArrayAttrib(m_vao, 0);
    glVertexArrayAttribFormat(m_vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(m_vao, 0, 0);

    glEnableVertexArrayAttrib(m_vao, 1);
    glVertexArrayAttribFormat(m_vao, 1, 2, GL_HALF_FLOAT, GL_FALSE, offsetof(BSP::Vertex, uv));
    glVertexArrayAttribBinding(m_vao, 1, 0);

    glEnableVertexArrayAttrib(m_vao, 2);
    glVertexArrayAttribFormat(m_vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(BSP::Vertex, lm_uv));
    glVertexArrayAttribBinding(m_vao, 2, 0);

    glEnableVertexArrayAttrib(m_vao, 3);
    glVertexArrayAttribFormat(m_vao, 3, 2, GL_FLOAT, GL_FALSE, offsetof(BSP::Vertex, lm_size));
    glVertexArrayAttribBinding(m_vao, 3, 0);

    glEnableVertexArrayAttrib(m_vao, 4);
    glVertexArrayAttribFormat(m_vao, 4, 1, GL_FLOAT, GL_FALSE, offsetof(BSP::Vertex, alpha));
    glVertexArrayAttribBinding(m_vao, 4, 0);

    glEnableVertexArrayAttrib(m_vao, 5);
    glVertexArrayAttribFormat(m_vao, 5, 3, GL_FLOAT, GL_FALSE, offsetof(BSP::Vertex, normal));
    glVertexArrayAttribBinding(m_vao, 5, 0);

    glEnableVertexArrayAttrib(m_vao, 6);
    glVertexArrayAttribFormat(m_vao, 6, 4, GL_FLOAT, GL_FALSE, offsetof(BSP::Vertex, tangent));
    glVertexArrayAttribBinding(m_vao, 6, 0);

    if (!map.lightmapAtlas.empty())
    {
        glCreateTextures(GL_TEXTURE_2D, 1, &m_lightmapTexture);
        glTextureStorage2D(m_lightmapTexture, 1, GL_RGBA16F, map.lightmapAtlasWidth, map.lightmapAtlasHeight);
        glTextureSubImage2D(m_lightmapTexture, 0, 0, 0, map.lightmapAtlasWidth, map.lightmapAtlasHeight, GL_RGBA, GL_FLOAT, map.lightmapAtlas.data());
        glTextureParameteri(m_lightmapTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_lightmapTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(m_lightmapTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_lightmapTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    m_drawCalls.clear();
    for (auto& dc : map.drawCalls)
    {
        BSPDrawCall draw;
        draw.texture = Materials::GetTexture(dc.textureName);
        draw.normalMap = Materials::GetNormalMap(dc.textureName);
        draw.mraohMap = Materials::GetMRAOMap(dc.textureName);
        draw.texture2 = Materials::GetTexture2(dc.textureName) ? Materials::GetTexture2(dc.textureName) : draw.texture;
        draw.normalMap2 = (Materials::GetTexture2(dc.textureName) && Materials::GetNormalMap2(dc.textureName)) ? Materials::GetNormalMap2(dc.textureName) : draw.normalMap;
        draw.mraohMap2 = Materials::GetMRAOMap2(dc.textureName);
        draw.heightScale1 = Materials::GetHeightScale(dc.textureName);
        draw.heightScale2 = Materials::GetHeightScale2(dc.textureName);
        draw.isBumped = dc.isBumped;
        draw.start = dc.start;
        draw.count = dc.count;
        draw.mins = dc.mins;
        draw.maxs = dc.maxs;
        m_drawCalls.push_back(draw);
    }

    m_totalVertexCount = (uint32_t)map.renderVertices.size();
    m_opaqueVertexCount = map.opaqueVertexCount;

    m_subModels.clear();
    for (const auto& ent : map.entities)
    {
        if (ent.modelIndex < 0 || ent.renderVertices.empty()) 
            continue;

        if (m_subModels.count(ent.modelIndex))
            continue;

        BrushModel bm;
        glCreateVertexArrays(1, &bm.vao);
        glCreateBuffers(1, &bm.vbo);
        glNamedBufferData(bm.vbo, ent.renderVertices.size() * sizeof(BSP::Vertex), ent.renderVertices.data(), GL_STATIC_DRAW);

        glVertexArrayVertexBuffer(bm.vao, 0, bm.vbo, 0, sizeof(BSP::Vertex));

        glEnableVertexArrayAttrib(bm.vao, 0);
        glVertexArrayAttribFormat(bm.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(bm.vao, 0, 0);

        glEnableVertexArrayAttrib(bm.vao, 1);
        glVertexArrayAttribFormat(bm.vao, 1, 2, GL_HALF_FLOAT, GL_FALSE, offsetof(BSP::Vertex, uv));
        glVertexArrayAttribBinding(bm.vao, 1, 0);

        glEnableVertexArrayAttrib(bm.vao, 2);
        glVertexArrayAttribFormat(bm.vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(BSP::Vertex, lm_uv));
        glVertexArrayAttribBinding(bm.vao, 2, 0);

        glEnableVertexArrayAttrib(bm.vao, 3);
        glVertexArrayAttribFormat(bm.vao, 3, 2, GL_FLOAT, GL_FALSE, offsetof(BSP::Vertex, lm_size));
        glVertexArrayAttribBinding(bm.vao, 3, 0);

        glEnableVertexArrayAttrib(bm.vao, 4);
        glVertexArrayAttribFormat(bm.vao, 4, 1, GL_FLOAT, GL_FALSE, offsetof(BSP::Vertex, alpha));
        glVertexArrayAttribBinding(bm.vao, 4, 0);

        glEnableVertexArrayAttrib(bm.vao, 5);
        glVertexArrayAttribFormat(bm.vao, 5, 3, GL_FLOAT, GL_FALSE, offsetof(BSP::Vertex, normal));
        glVertexArrayAttribBinding(bm.vao, 5, 0);

        glEnableVertexArrayAttrib(bm.vao, 6);
        glVertexArrayAttribFormat(bm.vao, 6, 4, GL_FLOAT, GL_FALSE, offsetof(BSP::Vertex, tangent));
        glVertexArrayAttribBinding(bm.vao, 6, 0);

        for (auto& dc : ent.drawCalls)
        {
            BSPDrawCall draw;
            draw.texture = Materials::GetTexture(dc.textureName);
            draw.normalMap = Materials::GetNormalMap(dc.textureName);
            draw.mraohMap = Materials::GetMRAOMap(dc.textureName);
            draw.texture2 = Materials::GetTexture2(dc.textureName) ? Materials::GetTexture2(dc.textureName) : draw.texture;
            draw.normalMap2 = (Materials::GetTexture2(dc.textureName) && Materials::GetNormalMap2(dc.textureName)) ? Materials::GetNormalMap2(dc.textureName) : draw.normalMap;
            draw.mraohMap2 = Materials::GetMRAOMap2(dc.textureName);
            draw.heightScale1 = Materials::GetHeightScale(dc.textureName);
            draw.heightScale2 = Materials::GetHeightScale2(dc.textureName);
            draw.isBumped = dc.isBumped;
            draw.start = dc.start;
            draw.count = dc.count;
            bm.drawCalls.push_back(draw);
        }
        m_subModels[ent.modelIndex] = bm;
    }

    return true;
}

void R_BSP::Draw(const R_Shader& shader, const Frustum& frustum, bool depthOnly)
{
    if (m_vao == 0)
    {
        return;
    }

    glBindVertexArray(m_vao);

    if (depthOnly)
    {
        glDrawArrays(GL_TRIANGLES, 0, m_opaqueVertexCount);
        return;
    }

    if (m_lightmapTexture != 0)
    {
        glBindTextureUnit(1, m_lightmapTexture);
    }

    for (auto& dc : m_drawCalls)
    {
        if (frustum.valid && !frustum.IsBoxVisible(dc.mins, dc.maxs))
        {
            continue;
        }

        if (!depthOnly)
        {
            shader.SetInt("u_useBump", dc.isBumped ? 1 : 0);
            (dc.texture ? dc.texture : Materials::GetTexture(""))->Bind(0);
            (dc.normalMap ? dc.normalMap : Materials::GetNormalMap(""))->Bind(1);
            (dc.mraohMap ? dc.mraohMap : Materials::GetMRAOMap(""))->Bind(2);
            (dc.texture2 ? dc.texture2 : Materials::GetTexture(""))->Bind(3);
            (dc.normalMap2 ? dc.normalMap2 : Materials::GetNormalMap2(""))->Bind(4);
            (dc.mraohMap2 ? dc.mraohMap2 : Materials::GetMRAOMap2(""))->Bind(5);
            shader.SetFloat("u_heightScale1", dc.heightScale1);
            shader.SetFloat("u_heightScale2", dc.heightScale2);
        }

        glDrawArrays(GL_TRIANGLES, dc.start, dc.count);
    }

    // Render Brush Entities
    for (auto& ent : EntityManager::GetEntities())
    {
        if (ent->IsRenderable() && ent->GetBModelIndex() >= 0)
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), ent->GetOrigin());
            glm::vec3 ang = ent->GetAngles();
            model = glm::rotate(model, glm::radians(ang.y), glm::vec3(0, 1, 0));
            model = glm::rotate(model, glm::radians(ang.x), glm::vec3(1, 0, 0));
            model = glm::rotate(model, glm::radians(ang.z), glm::vec3(0, 0, 1));
            DrawBModel(ent->GetBModelIndex(), shader, model, depthOnly);
        }
    }
}

void R_BSP::DrawBModel(int index, const R_Shader& shader, const glm::mat4& transform, bool depthOnly)
{
    if (m_subModels.find(index) == m_subModels.end()) 
        return;

    const auto& bm = m_subModels[index];

    if (bm.vao == 0)
    {
        return;
    }

    shader.SetMat4("u_model", transform);
    shader.SetInt("u_isInstanced", 0);

    if (!depthOnly)
    {
        if (m_lightmapTexture != 0)
        {
            glBindTextureUnit(1, m_lightmapTexture);
        }
    }

    glBindVertexArray(bm.vao);
    for (const auto& dc : bm.drawCalls)
    {
        if (!depthOnly)
        {
            shader.SetInt("u_useBump", dc.isBumped ? 1 : 0);
            (dc.texture ? dc.texture : Materials::GetTexture(""))->Bind(0);
            (dc.normalMap ? dc.normalMap : Materials::GetNormalMap(""))->Bind(1);
            (dc.mraohMap ? dc.mraohMap : Materials::GetMRAOMap(""))->Bind(2);
            (dc.texture2 ? dc.texture2 : Materials::GetTexture(""))->Bind(3);
            (dc.normalMap2 ? dc.normalMap2 : Materials::GetNormalMap(""))->Bind(4);
            (dc.mraohMap2 ? dc.mraohMap2 : Materials::GetMRAOMap2(""))->Bind(5);
            shader.SetFloat("u_heightScale1", dc.heightScale1);
            shader.SetFloat("u_heightScale2", dc.heightScale2);
        }

        glDrawArrays(GL_TRIANGLES, dc.start, dc.count);
    }
}

void R_BSP::Shutdown()
{
    if (m_vao != 0) 
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != 0) 
        glDeleteBuffers(1, &m_vbo);
    if (m_lightmapTexture != 0) 
        glDeleteTextures(1, &m_lightmapTexture);
    m_vao = 0; 
    m_vbo = 0; 
    m_lightmapTexture = 0;
    m_drawCalls.clear();
    for (auto& [idx, bm] : m_subModels)
    {
        if (bm.vao) 
            glDeleteVertexArrays(1, &bm.vao);
        if (bm.vbo) 
            glDeleteBuffers(1, &bm.vbo);
    }
    m_subModels.clear();
}