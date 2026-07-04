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
#include "renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <filesystem>
#include <cstring>

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

    m_layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Tangent, 4, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Indices, 4, bgfx::AttribType::Int16, false, false)
        .add(bgfx::Attrib::Weight, 4, bgfx::AttribType::Float)
        .end();

    m_uModelParams = bgfx::createUniform("u_modelParams", bgfx::UniformType::Vec4);
    m_uBones = bgfx::createUniform("u_bones", bgfx::UniformType::Mat4, 128);
    m_sDiffuse = bgfx::createUniform("s_diffuse", bgfx::UniformType::Sampler);
    m_sNormal = bgfx::createUniform("s_normal", bgfx::UniformType::Sampler);
    m_sMRAO = bgfx::createUniform("s_mraohMap", bgfx::UniformType::Sampler);

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

        group.transforms.reserve(group.instanceCount);

        std::vector<glm::vec4> allUVTransforms;
        for (const auto* prop : props) 
        {
            group.hasLightmap = !prop->lmData.empty();
            for (int i = 0; i < 4; i++) 
                allUVTransforms.push_back(prop->lmUVTransform[i]);
        }

        if (group.hasLightmap && !allUVTransforms.empty()) 
        {
            bgfx::VertexLayout uvLayout;
            uvLayout.begin()
                .add(bgfx::Attrib::Position, 4, bgfx::AttribType::Float)
                .end();

            const bgfx::Memory* mem = bgfx::copy(allUVTransforms.data(), (uint32_t)(allUVTransforms.size() * sizeof(glm::vec4)));
            group.lmUVSSBO = bgfx::createVertexBuffer(mem, uvLayout);
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

            group.transforms.push_back(m_visual);

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

        std::vector<ModelVertex> vertices(prim.positions.size());
        for (size_t v = 0; v < prim.positions.size(); ++v)
        {
            vertices[v].pos = prim.positions[v];
            vertices[v].normal = !prim.normals.empty() ? prim.normals[v] : glm::vec3(0, 1, 0);
            vertices[v].tangent = !prim.tangents.empty() ? prim.tangents[v] : glm::vec4(1, 0, 0, 1);
            vertices[v].uv = !prim.uvs.empty() ? prim.uvs[v] : glm::vec2(0, 0);

            if (!prim.joints.empty())
            {
                vertices[v].joints[0] = (int16_t)prim.joints[v].x;
                vertices[v].joints[1] = (int16_t)prim.joints[v].y;
                vertices[v].joints[2] = (int16_t)prim.joints[v].z;
                vertices[v].joints[3] = (int16_t)prim.joints[v].w;
            }
            else
            {
                std::memset(vertices[v].joints, 0, sizeof(vertices[v].joints));
            }

            vertices[v].weights = !prim.weights.empty() ? prim.weights[v] : glm::vec4(0.0f);
        }

        m.vbo = bgfx::createVertexBuffer(bgfx::copy(vertices.data(), (uint32_t)(vertices.size() * sizeof(ModelVertex))), m_layout);

        if (!prim.indices.empty())
        {
            m.ebo = bgfx::createIndexBuffer(bgfx::copy(prim.indices.data(), (uint32_t)(prim.indices.size() * sizeof(uint32_t))), BGFX_BUFFER_INDEX32);
            m.indexCount = (uint32_t)prim.indices.size();
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

void R_Models::Draw(bgfx::ViewId viewId, const R_Shader& shader, const Frustum& frustum, bool depthOnly)
{
    // 1. Render static props (Instanced)
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

        for (auto& mesh : group.meshes)
        {
            if (mesh.indexCount == 0)
            {
                continue;
            }

            uint16_t numInstances = (uint16_t)group.instanceCount;
            uint16_t instanceStride = 64;

            bgfx::InstanceDataBuffer idb;
            if (numInstances == bgfx::getAvailInstanceDataBuffer(numInstances, instanceStride))
            {
                bgfx::allocInstanceDataBuffer(&idb, numInstances, instanceStride);
                std::memcpy(idb.data, group.transforms.data(), numInstances * instanceStride);
                bgfx::setInstanceDataBuffer(&idb, 0, numInstances);
            }
            else
            {
                continue;
            }

            if (!depthOnly)
            {
                float modelParams[4] = { 1.0f, 0.0f, group.hasLightmap ? 1.0f : 0.0f, 0.0f };
                bgfx::setUniform(m_uModelParams, modelParams);

                if (group.hasLightmap && bgfx::isValid(group.lmUVSSBO))
                {
                    bgfx::setBuffer(7, group.lmUVSSBO, bgfx::Access::Read);
                }

                bgfx::setTexture(0, m_sDiffuse, (mesh.texture ? mesh.texture : Materials::GetTexture(""))->GetHandle());
                bgfx::setTexture(1, m_sNormal, (mesh.normalMap ? mesh.normalMap : Materials::GetNormalMap(""))->GetHandle());
                bgfx::setTexture(2, m_sMRAO, (mesh.mraohMap ? mesh.mraohMap : Materials::GetMRAOMap(""))->GetHandle());
            }

            bgfx::setVertexBuffer(0, mesh.vbo);
            bgfx::setIndexBuffer(mesh.ebo);

            bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW);
            bgfx::submit(viewId, shader.GetProgram());
        }
    }

    // 2. Render Animated Props
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
            DrawSkinned(viewId, shader, p->m_modelPath, mat, bones, depthOnly);
        }
        else
        {
            DrawSkinned(viewId, shader, p->m_modelPath, mat * p->m_nodeStates[0].globalMatrix, {}, depthOnly);
        }
    }

    // 3. Render Player Model
    for (auto& ent : EntityManager::GetEntities())
    {
        if (ent->IsPlayer() && ent->IsRenderable())
        {
            std::string modelPath = "models/error.glb";
            LoadModel(modelPath);

            auto* data = GetModelData(modelPath);
            if (data)
            {
                glm::vec3 a = ent->GetAngles();
                glm::mat4 mat = glm::translate(glm::mat4(1.0f), ent->GetOrigin());
                mat = glm::rotate(mat, glm::radians(a.y), { 0, 1, 0 });
                mat = glm::scale(mat, glm::vec3(0.025f));

                DrawSkinned(viewId, shader, modelPath, mat, {}, depthOnly);
            }
        }
    }
}

void R_Models::DrawSkinned(bgfx::ViewId viewId, const R_Shader& shader, const std::string& modelPath, const glm::mat4& transform, const std::vector<glm::mat4>& boneMatrices, bool depthOnly)
{
    if (m_propGroups.find(modelPath) == m_propGroups.end())
    {
        return;
    }

    auto& group = m_propGroups[modelPath];

    for (auto& mesh : group.meshes)
    {
        bgfx::setTransform(glm::value_ptr(transform));

        if (!depthOnly)
        {
            float modelParams[4] = { 0.0f, boneMatrices.empty() ? 0.0f : 1.0f, 0.0f, 0.0f };
            bgfx::setUniform(m_uModelParams, modelParams);

            if (!boneMatrices.empty())
            {
                bgfx::setUniform(m_uBones, glm::value_ptr(boneMatrices[0]), (uint16_t)std::min(boneMatrices.size(), (size_t)128));
            }
        }

        bgfx::setVertexBuffer(0, mesh.vbo);
        bgfx::setIndexBuffer(mesh.ebo);

        if (!depthOnly)
        {
            bgfx::setTexture(0, m_sDiffuse, (mesh.texture ? mesh.texture : Materials::GetTexture(""))->GetHandle());
            bgfx::setTexture(1, m_sNormal, (mesh.normalMap ? mesh.normalMap : Materials::GetNormalMap(""))->GetHandle());
            bgfx::setTexture(2, m_sMRAO, (mesh.mraohMap ? mesh.mraohMap : Materials::GetMRAOMap(""))->GetHandle());
        }

        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW);
        bgfx::submit(viewId, shader.GetProgram());
    }
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
            if (bgfx::isValid(mesh.vbo))
                bgfx::destroy(mesh.vbo);
            if (bgfx::isValid(mesh.ebo))
                bgfx::destroy(mesh.ebo);
        }

        if (bgfx::isValid(group.lmUVSSBO))
            bgfx::destroy(group.lmUVSSBO);

        group.physicsShape = nullptr;
    }

    if (bgfx::isValid(m_uModelParams))
    {
        bgfx::destroy(m_uModelParams);
        bgfx::destroy(m_uBones);
        bgfx::destroy(m_sDiffuse);
        bgfx::destroy(m_sNormal);
        bgfx::destroy(m_sMRAO);
        m_uModelParams = m_uBones = BGFX_INVALID_HANDLE;
    }

    m_propGroups.clear();
    m_blueprints.clear();
}