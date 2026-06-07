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
#include "r_models.h"
#include "filesystem.h"
#include "materials.h"
#include "console.h"
#include "physics.h"
#include "cubemap.h"
#include "entities.h"
#include "prop_animation.h"
#include "animation.h"
#include "gltf.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include <algorithm>
#include <filesystem>

R_Models::R_Models()
{
    m_blueprints.clear();
}

R_Models::~R_Models()
{
    Shutdown();
}

bool R_Models::Init(const BSP::MapData& mapData)
{
    Shutdown();

    std::unordered_map<std::string, std::vector<const BSP::StaticPropInstance*>> groupedProps;
    for (const auto& prop : mapData.staticProps)
    {
        groupedProps[prop.modelPath].push_back(&prop);
    }

    for (const auto& [path, props] : groupedProps)
    {
        LoadModel(path);

        auto& group = m_propGroups[path];
        group.instanceCount = (uint32_t)props.size();
        group.worldMins = glm::vec3(FLT_MAX);
        group.worldMaxs = glm::vec3(-FLT_MAX);

        if (!props.empty())
        {
            group.hasBumpedLighting = props[0]->hasBumpedLighting || !props[0]->lmDirData[0].empty();
        }

        std::vector<glm::mat4> transforms;
        transforms.reserve(group.instanceCount);

        std::vector<glm::vec4> allUVTransforms;
        for (const auto* prop : props) 
        {
            group.hasLightmap = !prop->lmData.empty();
            for (int i = 0; i < 4; i++) 
                allUVTransforms.push_back(prop->lmUVTransform[i]);
        }

        if (group.hasLightmap) 
        {
            glGenBuffers(1, &group.lmUVSSBO);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, group.lmUVSSBO);
            glBufferData(GL_SHADER_STORAGE_BUFFER, allUVTransforms.size() * sizeof(glm::vec4), allUVTransforms.data(), GL_STATIC_DRAW);
        }

        glm::vec3 corners[8] =
        {
            group.localMins,
            { group.localMaxs.x, group.localMins.y, group.localMins.z },
            { group.localMins.x, group.localMaxs.y, group.localMins.z },
            { group.localMins.x, group.localMins.y, group.localMaxs.z },
            { group.localMaxs.x, group.localMaxs.y, group.localMins.z },
            { group.localMins.x, group.localMaxs.y, group.localMaxs.z },
            { group.localMaxs.x, group.localMins.y, group.localMaxs.z },
            group.localMaxs
        };

        for (size_t i = 0; i < props.size(); ++i)
        {
            const auto* prop = props[i];

            glm::mat4 m_visual = glm::translate(glm::mat4(1.0f), prop->position);
            m_visual = glm::rotate(m_visual, glm::radians(prop->angles.y - 90.0f), glm::vec3(0, 1, 0));
            m_visual = glm::rotate(m_visual, glm::radians(-prop->angles.x), glm::vec3(1, 0, 0));
            m_visual = glm::rotate(m_visual, glm::radians(prop->angles.z), glm::vec3(0, 0, 1));

            glm::mat4 m_physics = m_visual;
            m_visual = glm::scale(m_visual, glm::vec3(prop->scale));

            transforms.push_back(m_visual);

            for (int c = 0; c < 8; c++)
            {
                glm::vec3 worldCorner = glm::vec3(m_visual * glm::vec4(corners[c], 1.0f));
                group.worldMins = glm::min(group.worldMins, worldCorner);
                group.worldMaxs = glm::max(group.worldMaxs, worldCorner);
            }

            if (group.physicsShape)
            {
                Physics::AddStaticBody(group.physicsShape, m_physics, glm::vec3(prop->scale));
            }
        }

        glGenBuffers(1, &group.transformSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, group.transformSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, transforms.size() * sizeof(glm::mat4), transforms.data(), GL_STATIC_DRAW);
    }

    return true;
}

void R_Models::LoadModel(const std::string& path)
{
    if (m_propGroups.count(path) && m_propGroups[path].totalVertices > 0)
        return;

    if (m_blueprints.find(path) == m_blueprints.end())
    {
        m_blueprints[path] = GLTF::Load(path);
    }

    GLTF::ModelData& modelData = m_blueprints[path];

    std::filesystem::path p(path);
    std::string modelName = p.stem().string();
    if (!modelData.valid)
    {
        if (path != "models/error.glb")
        {
            LoadModel("models/error.glb");
            m_propGroups[path] = m_propGroups["models/error.glb"];
            m_propGroups[path].instanceCount = 0;
        }
        return;
    }

    PropGroup group;
    uint32_t currentVertexOffset = 0;

    group.localMins = modelData.localMins;
    group.localMaxs = modelData.localMaxs;

    for (const auto& prim : modelData.primitives)
    {
        ModelMesh m = {};
        m.vertexOffset = currentVertexOffset;
        m.vertexCount = (uint32_t)prim.positions.size();

        glGenVertexArrays(1, &m.vao);
        glBindVertexArray(m.vao);

        if (!prim.positions.empty())
        {
            GLuint vbo;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, prim.positions.size() * sizeof(glm::vec3), prim.positions.data(), GL_STATIC_DRAW);
            m.vbos.push_back(vbo);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
        }

        if (!prim.uvs.empty())
        {
            GLuint vbo;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, prim.uvs.size() * sizeof(glm::vec2), prim.uvs.data(), GL_STATIC_DRAW);
            m.vbos.push_back(vbo);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), 0);
        }

        if (!prim.normals.empty())
        {
            GLuint vbo;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, prim.normals.size() * sizeof(glm::vec3), prim.normals.data(), GL_STATIC_DRAW);
            m.vbos.push_back(vbo);
            glEnableVertexAttribArray(5);
            glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
        }

        if (!prim.tangents.empty())
        {
            GLuint vbo;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, prim.tangents.size() * sizeof(glm::vec4), prim.tangents.data(), GL_STATIC_DRAW);
            m.vbos.push_back(vbo);
            glEnableVertexAttribArray(6);
            glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), 0);
        }

        if (!prim.joints.empty())
        {
            GLuint vbo;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, prim.joints.size() * sizeof(glm::uvec4), prim.joints.data(), GL_STATIC_DRAW);
            m.vbos.push_back(vbo);
            glEnableVertexAttribArray(7);
            glVertexAttribIPointer(7, 4, GL_UNSIGNED_INT, sizeof(glm::uvec4), 0);
        }

        if (!prim.weights.empty())
        {
            GLuint vbo;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, prim.weights.size() * sizeof(glm::vec4), prim.weights.data(), GL_STATIC_DRAW);
            m.vbos.push_back(vbo);
            glEnableVertexAttribArray(8);
            glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), 0);
        }

        if (!prim.indices.empty())
        {
            glGenBuffers(1, &m.ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, prim.indices.size() * sizeof(uint32_t), prim.indices.data(), GL_STATIC_DRAW);
            m.indexCount = (uint32_t)prim.indices.size();
            m.indexType = GL_UNSIGNED_INT;
        }

        std::string lookupName = modelName + "/" + prim.materialName;

        m.texture = Materials::GetTexture(lookupName);
        m.normalMap = Materials::GetNormalMap(lookupName);
        m.mraohMap = Materials::GetMRAOMap(lookupName);
        m.heightScale = Materials::GetHeightScale(lookupName);

        currentVertexOffset += m.vertexCount;
        group.meshes.push_back(m);
    }

    group.totalVertices = currentVertexOffset;
    group.physicsShape = Physics::CreateStaticMeshShape(modelData.physicsPositions, modelData.physicsIndices);

    m_propGroups[path] = group;
}

void R_Models::Draw(const R_Shader& shader, const Frustum& frustum, bool depthOnly)
{
    shader.SetInt("u_isInstanced", 1);

    for (auto& [path, group] : m_propGroups)
    {
        if (group.instanceCount == 0)
        {
            continue;
        }

        if (frustum.valid && !frustum.IsBoxVisible(group.worldMins, group.worldMaxs))
        {
            continue;
        }

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, group.transformSSBO);

        if (!depthOnly)
        {
            shader.SetInt("u_totalVertices", group.totalVertices);
        }

        for (auto& mesh : group.meshes)
        {
            if (mesh.indexCount == 0)
            {
                continue;
            }

            glBindVertexArray(mesh.vao);

            if (depthOnly)
            {
                for (auto& mesh : group.meshes)
                {
                    glBindVertexArray(mesh.vao);
                    glDrawElementsInstanced(GL_TRIANGLES, mesh.indexCount, mesh.indexType, 0, group.instanceCount);
                }
            }
            else
            {
                shader.SetInt("u_totalVertices", group.totalVertices);

                shader.SetInt("u_hasLM", group.hasLightmap ? 1 : 0);
                if (group.hasLightmap)
                {
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, group.lmUVSSBO);
                }

                for (auto& mesh : group.meshes)
                {
                    if (mesh.indexCount == 0) 
                        continue;

                    glBindVertexArray(mesh.vao);
                    shader.SetInt("u_vertexOffset", mesh.vertexOffset);
                    shader.SetFloat("u_heightScale1", mesh.heightScale);
                    shader.SetInt("u_useBump", group.hasBumpedLighting ? 1 : 0);

                    (mesh.texture ? mesh.texture : Materials::GetTexture(""))->Bind(0);
                    (mesh.normalMap ? mesh.normalMap : Materials::GetNormalMap(""))->Bind(1);
                    (mesh.mraohMap ? mesh.mraohMap : Materials::GetMRAOMap(""))->Bind(2);

                    glDrawElementsInstanced(GL_TRIANGLES, mesh.indexCount, mesh.indexType, 0, group.instanceCount);
                }
            }
        }
    }

    shader.SetInt("u_isInstanced", 0);

    // Render Animated Props
    for (auto& ent : EntityManager::GetEntities())
    {
        if (ent->GetClassName() != "prop_animation" || !ent->IsRenderable())
            continue;

        auto* p = static_cast<PropAnimation*>(ent.get());
        LoadModel(p->m_modelPath);

        auto* data = GetModelData(p->m_modelPath);
        if (!data) 
            continue;

        if (!ent->GetPhysObject())
        {
            btCollisionShape* shape = GetPhysicsShape(p->m_modelPath);
            if (shape)
            {
                glm::vec3 a = ent->GetAngles();
                glm::mat4 mat = glm::translate(glm::mat4(1.0f), ent->GetOrigin());
                mat = glm::rotate(mat, glm::radians(a.y), { 0, 1, 0 });
                mat = glm::rotate(mat, glm::radians(a.x), { 1, 0, 0 });
                mat = glm::rotate(mat, glm::radians(a.z), { 0, 0, 1 });
                mat = glm::scale(mat, glm::vec3(p->m_scale));
                btRigidBody* body = Physics::CreateRigidBody(0.0f, mat, shape, ent.get(), p->m_scale);
                ent->SetPhysObject(body);
            }
        }

        if (p->m_nodeStates.empty())
        {
            for (const auto& n : data->nodes)
                p->m_nodeStates.push_back({ n.translation, n.rotation, n.scale });
        }

        int idx = -1;
        if (!p->m_animName.empty())
        {
            for (int i = 0; i < (int)data->animations.size(); ++i)
                if (data->animations[i].name == p->m_animName) 
                    idx = i;

            if (idx != -1 && !data->animations.empty())
            {
                float duration = data->animations[idx].duration;
                if (!p->m_looping && p->m_animTime > duration)
                {
                    p->m_animTime = duration;
                    if (p->m_playing) 
                    { 
                        p->m_playing = false; 
                        p->FireOutput("OnAnimationDone"); 
                    }
                }
            }
        }

        if (!depthOnly)
            Animation::UpdateHierarchy(*data, p->m_nodeStates, idx, p->m_animTime);

        glm::vec3 a = ent->GetAngles();
        glm::mat4 mat = glm::translate(glm::mat4(1.0f), ent->GetOrigin());
        mat = glm::rotate(mat, glm::radians(a.y), { 0, 1, 0 });
        mat = glm::rotate(mat, glm::radians(a.x), { 1, 0, 0 });
        mat = glm::rotate(mat, glm::radians(a.z), { 0, 0, 1 });
        mat = glm::scale(mat, glm::vec3(p->m_scale));

        if (!data->skin.joints.empty())
        {
            std::vector<glm::mat4> bones;
            Animation::GetSkinMatrices(*data, p->m_nodeStates, bones);
            DrawSkinned(shader, p->m_modelPath, mat, bones);
        }
        else
        {
            DrawSkinned(shader, p->m_modelPath, mat * p->m_nodeStates[0].globalMatrix, {});
        }
    }
}

void R_Models::DrawSkinned(const R_Shader& shader, const std::string& modelPath, const glm::mat4& transform, const std::vector<glm::mat4>& boneMatrices)
{
    if (m_propGroups.find(modelPath) == m_propGroups.end())
    {
        return;
    }

    auto& group = m_propGroups[modelPath];

    shader.SetMat4("u_model", transform);
    shader.SetInt("u_isInstanced", 0);
    shader.SetInt("u_isAnimated", boneMatrices.empty() ? 0 : 1);

    // For now animated models dont have baked lighting
    shader.SetInt("u_hasLM", 0);

    for (size_t i = 0; i < boneMatrices.size() && i < 128; ++i)
    {
        shader.SetMat4("u_bones[" + std::to_string(i) + "]", boneMatrices[i]);
    }

    for (auto& mesh : group.meshes)
    {
        glBindVertexArray(mesh.vao);
        (mesh.texture ? mesh.texture : Materials::GetTexture(""))->Bind(0);
        (mesh.normalMap ? mesh.normalMap : Materials::GetNormalMap(""))->Bind(1);
        (mesh.mraohMap ? mesh.mraohMap : Materials::GetMRAOMap(""))->Bind(2);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, mesh.indexType, 0);
    }

    shader.SetInt("u_isAnimated", 0);
}

GLTF::ModelData* R_Models::GetModelData(const std::string& path)
{
    if (m_blueprints.find(path) != m_blueprints.end())
    {
        return &m_blueprints[path];
    }
    return nullptr;
}

btCollisionShape* R_Models::GetPhysicsShape(const std::string& path)
{ 
    return m_propGroups.count(path) ? m_propGroups[path].physicsShape : nullptr; 
}

void R_Models::Shutdown()
{
    for (auto& [path, group] : m_propGroups)
    {
        for (auto& mesh : group.meshes)
        {
            if (mesh.vao != 0)
                glDeleteVertexArrays(1, &mesh.vao);
            if (!mesh.vbos.empty())
                glDeleteBuffers((GLsizei)mesh.vbos.size(), mesh.vbos.data());
            if (mesh.ebo != 0)
                glDeleteBuffers(1, &mesh.ebo);
        }

        if (group.transformSSBO != 0)
            glDeleteBuffers(1, &group.transformSSBO);
        if (group.lmUVSSBO != 0)
            glDeleteBuffers(1, &group.lmUVSSBO);

        group.physicsShape = nullptr;
    }

    m_propGroups.clear();
    m_blueprints.clear();
}