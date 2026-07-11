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
#include "gltf.h"
#include "bsploader.h"
#include "r_shader.h"
#include "r_texture.h"
#include <bgfx/bgfx.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include <btBulletDynamicsCommon.h>

struct ModelVertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec4 tangent;
    glm::vec2 uv;
    int16_t joints[4];
    glm::vec4 weights;
};

struct ModelMesh
{
    bgfx::VertexBufferHandle vbo = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle ebo = BGFX_INVALID_HANDLE;
    uint32_t indexCount = 0;
    std::shared_ptr<R_Texture> texture;
    std::shared_ptr<R_Texture> normalMap;
    std::shared_ptr<R_Texture> mraohMap;
    float heightScale;
    uint32_t vertexOffset;
    uint32_t vertexCount;
};

class R_Models
{
public:
    R_Models();
    ~R_Models();

    bool Init(const BSP::MapData& mapData);
    void LoadModel(const std::string& path);
    void Draw(bgfx::ViewId viewId, const R_Shader& shader, const Frustum& frustum, bool depthOnly = false);
    void DrawSkinned(bgfx::ViewId viewId, const R_Shader& shader, const std::string& modelPath, const glm::mat4& transform, const std::vector<glm::mat4>& boneMatrices, bool depthOnly = false);

    GLTF::ModelData* GetModelData(const std::string& path);
    btCollisionShape* GetPhysicsShape(const std::string& path);
    void Shutdown();

private:
    std::unordered_map<std::string, GLTF::ModelData> m_blueprints;

    struct PropGroup
    {
        std::vector<ModelMesh> meshes;
        uint32_t instanceCount = 0;
        uint32_t totalVertices = 0;

        std::vector<glm::mat4> transforms;
        bgfx::VertexBufferHandle lmUVSSBO = BGFX_INVALID_HANDLE;
        bool hasLightmap = false;
        bool hasBumpedLighting = false;

        btCollisionShape* physicsShape = nullptr;
        glm::vec3 localMins = glm::vec3(1e10f);
        glm::vec3 localMaxs = glm::vec3(-1e10f);
        glm::vec3 worldMins = glm::vec3(1e10f);
        glm::vec3 worldMaxs = glm::vec3(-1e10f);
    };

    std::unordered_map<std::string, PropGroup> m_propGroups;
    bgfx::VertexLayout m_layout;

    bgfx::UniformHandle m_uModelParams = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uBones = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sDiffuse = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sNormal = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sMRAO = BGFX_INVALID_HANDLE;

    // Merged models for depth only
    bgfx::VertexBufferHandle m_mergedDepthVbo = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle m_mergedDepthEbo = BGFX_INVALID_HANDLE;
    uint32_t m_mergedDepthIndexCount = 0;
};