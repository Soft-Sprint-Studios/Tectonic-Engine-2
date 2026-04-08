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
    if (!map.loaded)
    {
        return false;
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, map.renderVertices.size() * sizeof(BSP::Vertex), map.renderVertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BSP::Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(BSP::Vertex), (void*)offsetof(BSP::Vertex, uv));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(BSP::Vertex), (void*)offsetof(BSP::Vertex, lm_uv));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(BSP::Vertex), (void*)offsetof(BSP::Vertex, lm_uv2));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(BSP::Vertex), (void*)offsetof(BSP::Vertex, lm_uv3));
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, sizeof(BSP::Vertex), (void*)offsetof(BSP::Vertex, lm_uv4));
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(BSP::Vertex), (void*)offsetof(BSP::Vertex, color));
    glEnableVertexAttribArray(9);
    glVertexAttribPointer(9, 3, GL_FLOAT, GL_FALSE, sizeof(BSP::Vertex), (void*)offsetof(BSP::Vertex, normal));

    glBindVertexArray(0);

    if (!map.lightmapAtlas.empty())
    {
        glGenTextures(1, &m_lightmapTexture);
        glBindTexture(GL_TEXTURE_2D, m_lightmapTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, map.lightmapAtlasWidth, map.lightmapAtlasHeight, 0, GL_RGB, GL_FLOAT, map.lightmapAtlas.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    m_drawCalls.clear();
    for (auto& dc : map.drawCalls)
    {
        BSPDrawCall draw;
        draw.texture = Materials::GetTexture(dc.textureName);
        draw.normalMap = Materials::GetNormalMap(dc.textureName);
        draw.specularMap = Materials::GetSpecularMap(dc.textureName);
        draw.isBumped = dc.isBumped;
        draw.start = dc.start;
        draw.count = dc.count;
        draw.mins = dc.mins;
        draw.maxs = dc.maxs;
        m_drawCalls.push_back(draw);
    }

    return true;
}

void R_BSP::Draw(const Shader& shader, const Frustum& frustum, bool depthOnly)
{
    if (m_vao == 0)
    {
        return;
    }

    glBindVertexArray(m_vao);
    if (m_lightmapTexture != 0)
    {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_lightmapTexture);
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
            shader.SetInt("u_isModel", 0);
            (dc.texture ? dc.texture : Materials::GetTexture(""))->Bind(0);
            (dc.isBumped && dc.normalMap ? dc.normalMap : Materials::GetFlatNormal())->Bind(2);
            (dc.isBumped && dc.specularMap ? dc.specularMap : Materials::GetWhiteTexture())->Bind(3);
        }

        glDrawArrays(GL_TRIANGLES, dc.start, dc.count);
    }
    glBindVertexArray(0);
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
}