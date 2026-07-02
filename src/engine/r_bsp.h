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
#pragma once
#include "bsploader.h"
#include "r_shader.h"
#include "r_texture.h"
#include <bgfx/bgfx.h>
#include <memory>
#include <vector>

struct BSPDrawCall
{
    std::shared_ptr<R_Texture> texture;
    std::shared_ptr<R_Texture> normalMap;
    std::shared_ptr<R_Texture> mraohMap;
    std::shared_ptr<R_Texture> texture2;
    std::shared_ptr<R_Texture> normalMap2;
    std::shared_ptr<R_Texture> mraohMap2;
    float heightScale1;
    float heightScale2;
    bool isBumped;

    uint32_t start;
    uint32_t count;
    glm::vec3 mins;
    glm::vec3 maxs;
};

struct BrushModel
{
    bgfx::VertexBufferHandle vbo = BGFX_INVALID_HANDLE;
    std::vector<BSPDrawCall> drawCalls;
};

class R_BSP
{
public:
    R_BSP();
    ~R_BSP();

    bool Init(const BSP::MapData& mapData);
    void Draw(const R_Shader& shader, bgfx::ViewId viewId, const Frustum& frustum, const glm::vec3& viewPos, bool depthOnly = false);
    void DrawBModel(int index, const R_Shader& shader, bgfx::ViewId viewId, const glm::mat4& transform, const glm::vec3& viewPos, bool depthOnly = false);
    void Shutdown();

private:
    bgfx::VertexLayout m_layout;
    bgfx::VertexBufferHandle m_vbo = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_lightmapTexture = BGFX_INVALID_HANDLE;

    bgfx::UniformHandle m_sDiffuse;
    bgfx::UniformHandle m_sNormal;
    bgfx::UniformHandle m_sMRAO;
    bgfx::UniformHandle m_sDiffuse2;
    bgfx::UniformHandle m_sNormal2;
    bgfx::UniformHandle m_sMRAO2;
    bgfx::UniformHandle m_sLightmap;
    bgfx::UniformHandle m_uBumpAndHeights;
    bgfx::UniformHandle m_uParallaxParams;
    bgfx::UniformHandle m_uViewPos;
    bgfx::UniformHandle m_uLightmapParams;

    std::vector<BSPDrawCall> m_drawCalls;
    std::unordered_map<int, BrushModel> m_subModels;
    uint32_t m_opaqueVertexCount = 0;
};