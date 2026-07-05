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
#include "console.h"
#include "cvar.h"
#include <glm/gtc/type_ptr.hpp>

R_BSP::R_BSP()
{
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

    m_layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Tangent, 4, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Half)
        .add(bgfx::Attrib::TexCoord1, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord2, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord3, 1, bgfx::AttribType::Float)
        .end();

    const bgfx::Memory* vboMem = bgfx::copy(map.renderVertices.data(), (uint32_t)(map.renderVertices.size() * sizeof(BSP::Vertex)));
    m_vbo = bgfx::createVertexBuffer(vboMem, m_layout);

    if (!map.lightmapAtlas.empty())
    {
        std::vector<uint16_t> halfAtlas(map.lightmapAtlas.size());

        for (size_t i = 0; i < map.lightmapAtlas.size(); ++i)
        {
            halfAtlas[i] = glm::packHalf1x16(map.lightmapAtlas[i]);
        }

        const bgfx::Memory* lmMem = bgfx::copy(halfAtlas.data(), uint32_t(halfAtlas.size() * sizeof(uint16_t)));

        m_lightmapTexture = bgfx::createTexture2D((uint16_t)map.lightmapAtlasWidth, (uint16_t)map.lightmapAtlasHeight, false, 1, bgfx::TextureFormat::RGBA16F, BGFX_TEXTURE_NONE, lmMem);
    }

    m_sDiffuse = bgfx::createUniform("s_diffuse", bgfx::UniformType::Sampler);
    m_sNormal = bgfx::createUniform("s_normal", bgfx::UniformType::Sampler);
    m_sMRAO = bgfx::createUniform("s_mraohMap", bgfx::UniformType::Sampler);
    m_sDiffuse2 = bgfx::createUniform("s_diffuse2", bgfx::UniformType::Sampler);
    m_sNormal2 = bgfx::createUniform("s_normal2", bgfx::UniformType::Sampler);
    m_sMRAO2 = bgfx::createUniform("s_mraohMap2", bgfx::UniformType::Sampler);
    m_sLightmap = bgfx::createUniform("s_lightmap", bgfx::UniformType::Sampler);
    m_uBumpAndHeights = bgfx::createUniform("u_bumpAndHeights", bgfx::UniformType::Vec4);
    m_uParallaxParams = bgfx::createUniform("u_parallaxParams", bgfx::UniformType::Vec4);
    m_uViewPos = bgfx::createUniform("u_viewPos", bgfx::UniformType::Vec4);
    m_uLightmapParams = bgfx::createUniform("u_lightmapParams", bgfx::UniformType::Vec4);
    m_uModelParams = bgfx::createUniform("u_modelParams", bgfx::UniformType::Vec4);

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

    m_opaqueVertexCount = map.opaqueVertexCount;

    m_subModels.clear();
    for (const auto& ent : map.entities)
    {
        if (ent.modelIndex < 0 || ent.renderVertices.empty())
        {
            continue;
        }

        if (m_subModels.count(ent.modelIndex))
        {
            continue;
        }

        BrushModel bm;
        const bgfx::Memory* subMem = bgfx::copy(ent.renderVertices.data(), (uint32_t)(ent.renderVertices.size() * sizeof(BSP::Vertex)));
        bm.vbo = bgfx::createVertexBuffer(subMem, m_layout);

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

void R_BSP::Draw(const R_Shader& shader, bgfx::ViewId viewId, const Frustum& frustum, const glm::vec3& viewPos, bool depthOnly)
{
    if (!bgfx::isValid(m_vbo) || !bgfx::isValid(shader.GetProgram()))
    {
        return;
    }

    glm::mat4 identity(1.0f);
    uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW;

    for (auto& dc : m_drawCalls)
    {
        if (frustum.valid && !frustum.IsBoxVisible(dc.mins, dc.maxs))
        {
            continue;
        }

        bgfx::setTransform(glm::value_ptr(identity));
        bgfx::setVertexBuffer(0, m_vbo, dc.start, dc.count);

        float modelParams[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
        bgfx::setUniform(m_uModelParams, modelParams);

        if (!depthOnly)
        {
            float bumpHeights[4] = { dc.isBumped ? 1.0f : 0.0f, dc.heightScale1, dc.heightScale2, 0.0f };
            bgfx::setUniform(m_uBumpAndHeights, bumpHeights);

            float vp[4] = { viewPos.x, viewPos.y, viewPos.z, 0.0f };
            bgfx::setUniform(m_uViewPos, vp);

            float par[4] = 
            {
                (float)CVar::GetFloat("mat_parallax_min_steps", 8.0f),
                (float)CVar::GetFloat("mat_parallax_max_steps", 32.0f),
                (float)CVar::GetInt("mat_parallax_refine", 8),
                (float)CVar::GetInt("mat_parallax", 1)
            };
            bgfx::setUniform(m_uParallaxParams, par);

            float lm[4] = { (float)CVar::GetInt("r_lightmap_bicubic", 1), 0.0f, 0.0f, 0.0f };
            bgfx::setUniform(m_uLightmapParams, lm);

            if (bgfx::isValid(m_lightmapTexture))
            {
                bgfx::setTexture(6, m_sLightmap, m_lightmapTexture);
            }

            bgfx::setTexture(0, m_sDiffuse, (dc.texture ? dc.texture : Materials::GetTexture(""))->GetHandle());
            bgfx::setTexture(1, m_sNormal, (dc.normalMap ? dc.normalMap : Materials::GetNormalMap(""))->GetHandle());
            bgfx::setTexture(2, m_sMRAO, (dc.mraohMap ? dc.mraohMap : Materials::GetMRAOMap(""))->GetHandle());
            bgfx::setTexture(3, m_sDiffuse2, (dc.texture2 ? dc.texture2 : Materials::GetTexture(""))->GetHandle());
            bgfx::setTexture(4, m_sNormal2, (dc.normalMap2 ? dc.normalMap2 : Materials::GetNormalMap2(""))->GetHandle());
            bgfx::setTexture(5, m_sMRAO2, (dc.mraohMap2 ? dc.mraohMap2 : Materials::GetMRAOMap2(""))->GetHandle());
        }

        bgfx::setState(state);
        bgfx::submit(viewId, shader.GetProgram());
    }

    for (auto& ent : EntityManager::GetEntities())
    {
        if (ent->IsRenderable() && ent->GetBModelIndex() >= 0)
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), ent->GetOrigin());
            glm::vec3 ang = ent->GetAngles();
            model = glm::rotate(model, glm::radians(ang.y), glm::vec3(0, 1, 0));
            model = glm::rotate(model, glm::radians(ang.x), glm::vec3(1, 0, 0));
            model = glm::rotate(model, glm::radians(ang.z), glm::vec3(0, 0, 1));
            DrawBModel(ent->GetBModelIndex(), shader, viewId, model, viewPos, depthOnly);
        }
    }
}

void R_BSP::DrawBModel(int index, const R_Shader& shader, bgfx::ViewId viewId, const glm::mat4& transform, const glm::vec3& viewPos, bool depthOnly)
{
    if (m_subModels.find(index) == m_subModels.end())
    {
        return;
    }

    const auto& bm = m_subModels[index];
    if (!bgfx::isValid(bm.vbo))
    {
        return;
    }

    uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW;

    for (const auto& dc : bm.drawCalls)
    {
        bgfx::setTransform(glm::value_ptr(transform));
        bgfx::setVertexBuffer(0, bm.vbo, dc.start, dc.count);

        float modelParams[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
        bgfx::setUniform(m_uModelParams, modelParams);

        if (!depthOnly)
        {
            float bumpHeights[4] = { dc.isBumped ? 1.0f : 0.0f, dc.heightScale1, dc.heightScale2, 0.0f };
            bgfx::setUniform(m_uBumpAndHeights, bumpHeights);

            float vp[4] = { viewPos.x, viewPos.y, viewPos.z, 0.0f };
            bgfx::setUniform(m_uViewPos, vp);

            float par[4] = 
            {
                (float)CVar::GetFloat("mat_parallax_min_steps", 8.0f),
                (float)CVar::GetFloat("mat_parallax_max_steps", 32.0f),
                (float)CVar::GetInt("mat_parallax_refine", 8),
                (float)CVar::GetInt("mat_parallax", 1)
            };
            bgfx::setUniform(m_uParallaxParams, par);

            float lm[4] = { (float)CVar::GetInt("r_lightmap_bicubic", 1), 0.0f, 0.0f, 0.0f };
            bgfx::setUniform(m_uLightmapParams, lm);

            if (bgfx::isValid(m_lightmapTexture))
            {
                bgfx::setTexture(6, m_sLightmap, m_lightmapTexture);
            }

            bgfx::setTexture(0, m_sDiffuse, (dc.texture ? dc.texture : Materials::GetTexture(""))->GetHandle());
            bgfx::setTexture(1, m_sNormal, (dc.normalMap ? dc.normalMap : Materials::GetNormalMap(""))->GetHandle());
            bgfx::setTexture(2, m_sMRAO, (dc.mraohMap ? dc.mraohMap : Materials::GetMRAOMap(""))->GetHandle());
            bgfx::setTexture(3, m_sDiffuse2, (dc.texture2 ? dc.texture2 : Materials::GetTexture(""))->GetHandle());
            bgfx::setTexture(4, m_sNormal2, (dc.normalMap2 ? dc.normalMap2 : Materials::GetNormalMap2(""))->GetHandle());
            bgfx::setTexture(5, m_sMRAO2, (dc.mraohMap2 ? dc.mraohMap2 : Materials::GetMRAOMap2(""))->GetHandle());
        }

        bgfx::setState(state);
        bgfx::submit(viewId, shader.GetProgram());
    }
}

void R_BSP::Shutdown()
{
    if (bgfx::isValid(m_vbo))
    {
        bgfx::destroy(m_vbo);
        m_vbo = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_lightmapTexture))
    {
        bgfx::destroy(m_lightmapTexture);
        m_lightmapTexture = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_sDiffuse))
    {
        bgfx::destroy(m_sDiffuse);
        bgfx::destroy(m_sNormal);
        bgfx::destroy(m_sMRAO);
        bgfx::destroy(m_sDiffuse2);
        bgfx::destroy(m_sNormal2);
        bgfx::destroy(m_sMRAO2);
        bgfx::destroy(m_sLightmap);
        bgfx::destroy(m_uBumpAndHeights);
        bgfx::destroy(m_uParallaxParams);
        bgfx::destroy(m_uViewPos);
        bgfx::destroy(m_uLightmapParams);
        bgfx::destroy(m_uModelParams);

        m_sDiffuse = BGFX_INVALID_HANDLE;
    }

    for (auto& [idx, bm] : m_subModels)
    {
        if (bgfx::isValid(bm.vbo))
        {
            bgfx::destroy(bm.vbo);
        }
    }

    m_subModels.clear();
    m_drawCalls.clear();
}