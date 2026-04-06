#include "r_bsp.h"
#include "material_manager.h"
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
        draw.texture = MaterialManager::GetTexture(dc.textureName);
        draw.normalMap = MaterialManager::GetNormalMap(dc.textureName);
        draw.specularMap = MaterialManager::GetSpecularMap(dc.textureName);
        draw.isBumped = dc.isBumped;
        draw.start = dc.start;
        draw.count = dc.count;
        m_drawCalls.push_back(draw);
    }

    return true;
}

void R_BSP::Draw(const Shader& shader)
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
        shader.SetInt("u_useBump", dc.isBumped);
        shader.SetInt("u_isModel", 0);

        if (dc.texture)
        {
            dc.texture->Bind(0);
        }

        if (dc.isBumped && dc.normalMap)
        {
            dc.normalMap->Bind(2);
        }

        if (dc.isBumped && dc.specularMap)
        {
            dc.specularMap->Bind(3);
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